#include <pebble.h>
#include "slam.h"

static Window *s_window;
static Layer *s_court_layer;

// État météo par tournoi (rempli via PebbleKit JS).
static SlamWeather s_weather[SLAM_COUNT];

// Décalage UTC par tournoi (secondes). Initialisé avec les valeurs en dur de
// SLAMS, puis remplacé par les vraies valeurs (DST inclus) reçues du JS météo.
static int32_t s_tz_offset[SLAM_COUNT];

// Vainqueur de la dernière édition (selon le tour ATP/WTA choisi en config).
static char s_winner[SLAM_COUNT][20];

// ---------------------------------------------------------------------------
// Helpers temps
// ---------------------------------------------------------------------------

// struct tm de l'heure locale d'un tournoi (UTC + offset), via gmtime.
static struct tm slam_local_tm(SlamId id) {
  time_t local = time(NULL) + s_tz_offset[id];
  return *gmtime(&local);
}

static void slam_time_string(SlamId id, char *buf, size_t len) {
  struct tm t = slam_local_tm(id);
  strftime(buf, len, clock_is_24h_style() ? "%H:%M" : "%I:%M", &t);
}

// Jour entre 6h et 20h (heure locale du tournoi).
static bool slam_is_day(SlamId id) {
  int h = slam_local_tm(id).tm_hour;
  return h >= 6 && h < 20;
}

// Ligne température : "24°" si météo dispo, sinon nom de la ville (avant 1re synchro).
static void slam_temp_string(SlamId id, char *buf, size_t len) {
  if (s_weather[id].valid) {
    snprintf(buf, len, "%d°", s_weather[id].temp);
  } else {
    snprintf(buf, len, "%s", SLAMS[id].city);
  }
}

// En-tête de zone : abréviation seule, ou "<abbr>  J-12" / "<abbr>  LIVE" si focus.
static void slam_header_string(SlamId id, const SlamFocus *focus, char *buf, size_t len) {
  if (focus->id == id) {
    if (focus->live) {
      snprintf(buf, len, "%s LIVE", SLAMS[id].abbr);
    } else {
      snprintf(buf, len, "%s J-%d", SLAMS[id].abbr, focus->days_until);
    }
  } else {
    snprintf(buf, len, "%s", SLAMS[id].abbr);
  }
}

// ---------------------------------------------------------------------------
// Icône jour/nuit
// ---------------------------------------------------------------------------
static void draw_day_night(GContext *ctx, GPoint c, bool day, GColor surface) {
  if (day) {
    // Soleil : disque + rayons
    graphics_context_set_fill_color(ctx, GColorYellow);
    graphics_fill_circle(ctx, c, 4);
    graphics_context_set_stroke_color(ctx, GColorYellow);
    graphics_context_set_stroke_width(ctx, 1);
    for (int i = 0; i < 8; i++) {
      int32_t a = TRIG_MAX_ANGLE * i / 8;
      GPoint p1 = { c.x + (6 * cos_lookup(a) / TRIG_MAX_RATIO),
                    c.y + (6 * sin_lookup(a) / TRIG_MAX_RATIO) };
      GPoint p2 = { c.x + (8 * cos_lookup(a) / TRIG_MAX_RATIO),
                    c.y + (8 * sin_lookup(a) / TRIG_MAX_RATIO) };
      graphics_draw_line(ctx, p1, p2);
    }
  } else {
    // Lune : disque pâle évidé par un disque couleur surface (croissant)
    graphics_context_set_fill_color(ctx, GColorPastelYellow);
    graphics_fill_circle(ctx, c, 5);
    graphics_context_set_fill_color(ctx, surface);
    graphics_fill_circle(ctx, GPoint(c.x + 3, c.y - 2), 5);
  }
}

// ---------------------------------------------------------------------------
// Rendu d'une zone (un quadrant = un Grand Chelem)
// ---------------------------------------------------------------------------
static void draw_zone(GContext *ctx, GRect q, SlamId id, const SlamFocus *focus) {
  const Slam *s = &SLAMS[id];

  // Fond = couleur de la surface
  graphics_context_set_fill_color(ctx, s->surface);
  graphics_fill_rect(ctx, q, 0, GCornerNone);

  // Sur écran rond (chalk), le contenu est resserré vers le centre (le filet),
  // là où le cercle est le plus large, pour éviter que les coins ne rognent.
  // Sur écran rectangulaire, le contenu occupe tout le quadrant.
#if defined(PBL_ROUND)
  bool is_top = (q.origin.y == 0);
  bool is_left = (q.origin.x == 0);
  const int INNER_H = 70, SIDE_IN = 8, SIDE_OUT = 18;
  GRect content = GRect(
    q.origin.x + (is_left ? SIDE_OUT : SIDE_IN),
    is_top ? (q.origin.y + q.size.h - INNER_H) : q.origin.y,
    q.size.w - SIDE_IN - SIDE_OUT,
    INNER_H);
#else
  GRect content = q;
#endif

  // Lignes de court : bordure blanche inset (rectangulaire uniquement ;
  // sur écran rond l'anneau "camembert" est dessiné globalement, voir court_update_proc).
#if !defined(PBL_ROUND)
  graphics_context_set_stroke_color(ctx, s->text);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_rect(ctx, grect_inset(q, GEdgeInsets(6)));

  // Surlignage du tournoi en cours / prochain : cadre jaune
  if (focus->id == id) {
    graphics_context_set_stroke_color(ctx, GColorYellow);
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_rect(ctx, grect_inset(q, GEdgeInsets(2)));
  }
#endif

  // Icône jour/nuit (coin haut-droit de la zone de contenu).
  // Donnée réelle de l'API si dispo, sinon heuristique 6h-20h.
  bool day = s_weather[id].valid ? s_weather[id].is_day : slam_is_day(id);
  draw_day_night(ctx, GPoint(content.origin.x + content.size.w - 12, content.origin.y + 12),
                 day, s->surface);

  // 4 lignes par zone, sans rien sacrifier : en-tête, heure, champion, météo.
  const int x0 = content.origin.x, y0 = content.origin.y, w = content.size.w;
  char buf[24];

  // En-tête : abréviation (jaune + compte à rebours si tournoi focus)
  bool is_focus = (focus->id == id);
  slam_header_string(id, focus, buf, sizeof(buf));
  graphics_context_set_text_color(ctx, is_focus ? GColorYellow : s->text);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(x0, y0 + PBL_IF_ROUND_ELSE(-1, 2), w, 22),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Heure locale (élément principal)
  slam_time_string(id, buf, sizeof(buf));
  graphics_context_set_text_color(ctx, s->text);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(x0, y0 + PBL_IF_ROUND_ELSE(18, 26), w, 30),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Champion de la dernière édition (en jaune, masqué tant que non reçu)
  if (s_winner[id][0] != '\0') {
    graphics_context_set_text_color(ctx, GColorYellow);
    graphics_draw_text(ctx, s_winner[id], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(x0, y0 + PBL_IF_ROUND_ELSE(40, 58), w, 18),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  // Météo (température, ou ville avant la 1re synchro)
  slam_temp_string(id, buf, sizeof(buf));
  graphics_context_set_text_color(ctx, s->text);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(x0, y0 + PBL_IF_ROUND_ELSE(54, 84), w, 18),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// ---------------------------------------------------------------------------
// Rendu du court complet
// ---------------------------------------------------------------------------
static void court_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int midx = b.size.w / 2;
  int midy = b.size.h / 2;

  time_t now = time(NULL);
  struct tm today = *localtime(&now);
  SlamFocus focus = slam_focus(&today);

  GRect tl = GRect(0, 0, midx, midy);
  GRect tr = GRect(midx, 0, b.size.w - midx, midy);
  GRect bl = GRect(0, midy, midx, b.size.h - midy);
  GRect br = GRect(midx, midy, b.size.w - midx, b.size.h - midy);

  draw_zone(ctx, tl, SLAM_AO, &focus);
  draw_zone(ctx, tr, SLAM_RG, &focus);
  draw_zone(ctx, bl, SLAM_WIM, &focus);
  draw_zone(ctx, br, SLAM_US, &focus);

  // Le "filet" : barre horizontale épaisse + ligne centrale verticale
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, midy - 2, b.size.w, 4), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(midx - 1, 0, 2, b.size.h), 0, GCornerNone);

#if defined(PBL_ROUND)
  // Camembert : anneau blanc sur le pourtour, et arc jaune sur la portion
  // du tournoi en cours / prochain. Angles (0° = haut, sens horaire) :
  //   AO=270-360 (haut-gauche), RG=0-90 (haut-droit),
  //   W=180-270 (bas-gauche),   US=90-180 (bas-droit).
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, b, GOvalScaleModeFitCircle, 3, 0, TRIG_MAX_ANGLE);

  static const int focus_start[SLAM_COUNT] = { 270, 0, 180, 90 };
  static const int focus_end[SLAM_COUNT]   = { 360, 90, 270, 180 };
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_fill_radial(ctx, b, GOvalScaleModeFitCircle, 6,
                       DEG_TO_TRIGANGLE(focus_start[focus.id]),
                       DEG_TO_TRIGANGLE(focus_end[focus.id]));
#endif

  // Balle de tennis au centre du filet
  GPoint c = GPoint(midx, midy);
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_fill_circle(ctx, c, 8);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, c, 8);
}

// ---------------------------------------------------------------------------
// Réception météo (PebbleKit JS)
// ---------------------------------------------------------------------------
static void inbox_received(DictionaryIterator *iter, void *context) {
  for (int i = 0; i < SLAM_COUNT; i++) {
    Tuple *t;
    if ((t = dict_find(iter, MESSAGE_KEY_TEMP + i))) {
      s_weather[i].temp = t->value->int32;
      s_weather[i].valid = true;
    }
    if ((t = dict_find(iter, MESSAGE_KEY_COND + i))) {
      s_weather[i].cond = t->value->int32;
    }
    if ((t = dict_find(iter, MESSAGE_KEY_IS_DAY + i))) {
      s_weather[i].is_day = (t->value->int32 != 0);
    }
    if ((t = dict_find(iter, MESSAGE_KEY_TZ_OFFSET + i))) {
      s_tz_offset[i] = t->value->int32;
    }
    if ((t = dict_find(iter, MESSAGE_KEY_WINNER + i))) {
      strncpy(s_winner[i], t->value->cstring, sizeof(s_winner[i]) - 1);
      s_winner[i][sizeof(s_winner[i]) - 1] = '\0';
    }
  }
  if (s_court_layer) {
    layer_mark_dirty(s_court_layer);
  }
}

// ---------------------------------------------------------------------------
// Tick / Window
// ---------------------------------------------------------------------------
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_court_layer);
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_court_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_court_layer, court_update_proc);
  layer_add_child(root, s_court_layer);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_court_layer);
}

static void prv_init(void) {
  // Fuseaux initiaux (valeurs en dur) en attendant les vraies du JS météo.
  for (int i = 0; i < SLAM_COUNT; i++) {
    s_tz_offset[i] = SLAMS[i].tz_offset_sec;
  }

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_register_inbox_received(inbox_received);
  app_message_open(512, 64);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
