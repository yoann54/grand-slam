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

// Mot de condition météo (ex: "Clouds", "Rain") reçu de l'API.
static char s_cond[SLAM_COUNT][16];

// Thème clair (true) ou sombre (false, défaut), reçu de la config.
static bool s_theme_light = false;

// Date de début (mois*100+jour) de la prochaine édition de chaque tournoi,
// reçue du winners.json (0 = utiliser la date par défaut de SLAMS).
static uint16_t s_start_md[SLAM_COUNT];

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
  if (clock_is_24h_style()) {
    strftime(buf, len, "%H:%M", &t);
  } else {
    // 12h compact avec indicateur am/pm (ex: "6:47p"), pour tenir dans la zone.
    int h = t.tm_hour % 12;
    if (h == 0) { h = 12; }
    snprintf(buf, len, "%d:%02d%c", h, t.tm_min, t.tm_hour < 12 ? 'a' : 'p');
  }
}

// Jour entre 6h et 20h (heure locale du tournoi).
static bool slam_is_day(SlamId id) {
  int h = slam_local_tm(id).tm_hour;
  return h >= 6 && h < 20;
}

// Index de langue selon la locale de la montre (colonnes de la table ci-dessous).
// 0=EN, 1=FR, 2=ES, 3=DE, 4=IT. Toute autre langue retombe sur l'anglais.
static int lang_index(void) {
  const char *l = i18n_get_system_locale();
  if (!l) { return 0; }
  if (l[0] == 'f' && l[1] == 'r') { return 1; }
  if (l[0] == 'e' && l[1] == 's') { return 2; }
  if (l[0] == 'd' && l[1] == 'e') { return 3; }
  if (l[0] == 'i' && l[1] == 't') { return 4; }
  return 0;
}

// Traduit le mot de condition OpenWeatherMap (anglais) selon la langue de la
// montre. Mots courts (sans accents) ; condition inconnue → laissée en anglais.
static const char *localize_cond(const char *en) {
  int lg = lang_index();
  if (en[0] == '\0' || lg == 0) {
    return en;
  }
  //                         EN            FR           ES           DE           IT
  static const char *const map[][5] = {
    {"Clear",        "Clair",      "Despejado", "Klar",      "Sereno"},
    {"Clouds",       "Nuageux",    "Nubes",     "Bewolkt",   "Nuvoloso"},
    {"Rain",         "Pluie",      "Lluvia",    "Regen",     "Pioggia"},
    {"Drizzle",      "Bruine",     "Llovizna",  "Niesel",    "Pioggerella"},
    {"Thunderstorm", "Orage",      "Tormenta",  "Gewitter",  "Temporale"},
    {"Snow",         "Neige",      "Nieve",     "Schnee",    "Neve"},
    {"Mist",         "Brume",      "Niebla",    "Nebel",     "Foschia"},
    {"Fog",          "Brouillard", "Niebla",    "Nebel",     "Nebbia"},
    {"Haze",         "Brume",      "Bruma",     "Dunst",     "Foschia"},
    {"Smoke",        "Fumee",      "Humo",      "Rauch",     "Fumo"},
    {"Dust",         "Poussiere",  "Polvo",     "Staub",     "Polvere"},
    {"Sand",         "Sable",      "Arena",     "Sand",      "Sabbia"},
    {"Tornado",      "Tornade",    "Tornado",   "Tornado",   "Tornado"},
    {"Squall",       "Rafales",    "Rafaga",    "Boe",       "Raffica"},
    {"Ash",          "Cendres",    "Ceniza",    "Asche",     "Cenere"},
  };
  for (unsigned i = 0; i < ARRAY_LENGTH(map); i++) {
    if (strcmp(en, map[i][0]) == 0) {
      return map[i][lg];
    }
  }
  return en;
}

// Ligne météo : "Nuageux 24°" si dispo (condition traduite + température),
// sinon juste "24°", sinon le nom de la ville (avant la 1re synchro).
static void slam_temp_string(SlamId id, char *buf, size_t len) {
  if (s_weather[id].valid) {
    if (s_cond[id][0] != '\0') {
      snprintf(buf, len, "%s %d°", localize_cond(s_cond[id]), s_weather[id].temp);
    } else {
      snprintf(buf, len, "%d°", s_weather[id].temp);
    }
  } else {
    snprintf(buf, len, "%s", SLAMS[id].city);
  }
}

// Compte à rebours du tournoi focus : "LIVE" ou "J-12".
static void slam_countdown_string(const SlamFocus *focus, char *buf, size_t len) {
  if (focus->live) {
    snprintf(buf, len, "LIVE");
  } else {
    snprintf(buf, len, "J-%d", focus->days_until);
  }
}

// ---------------------------------------------------------------------------
// Icône jour/nuit
// ---------------------------------------------------------------------------
static void draw_day_night(GContext *ctx, GPoint c, bool day, GColor surface, GColor icon) {
  if (day) {
    // Soleil : disque + rayons (légèrement agrandi)
    graphics_context_set_fill_color(ctx, icon);
    graphics_fill_circle(ctx, c, 5);
    graphics_context_set_stroke_color(ctx, icon);
    graphics_context_set_stroke_width(ctx, 2);
    for (int i = 0; i < 8; i++) {
      int32_t a = TRIG_MAX_ANGLE * i / 8;
      GPoint p1 = { c.x + (7 * cos_lookup(a) / TRIG_MAX_RATIO),
                    c.y + (7 * sin_lookup(a) / TRIG_MAX_RATIO) };
      GPoint p2 = { c.x + (10 * cos_lookup(a) / TRIG_MAX_RATIO),
                    c.y + (10 * sin_lookup(a) / TRIG_MAX_RATIO) };
      graphics_draw_line(ctx, p1, p2);
    }
  } else {
    // Lune : disque évidé par un disque couleur surface (croissant, agrandi)
    graphics_context_set_fill_color(ctx, icon);
    graphics_fill_circle(ctx, c, 6);
    graphics_context_set_fill_color(ctx, surface);
    graphics_fill_circle(ctx, GPoint(c.x + 4, c.y - 2), 6);
  }
}

// ---------------------------------------------------------------------------
// Rendu d'une zone (un quadrant = un Grand Chelem)
// ---------------------------------------------------------------------------
static void draw_zone(GContext *ctx, GRect q, SlamId id, const SlamFocus *focus) {
  Theme th = slam_theme(s_theme_light);
  GColor surface = slam_surface(id, s_theme_light);

  // Fond = couleur de la surface
  graphics_context_set_fill_color(ctx, surface);
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
  graphics_context_set_stroke_color(ctx, th.ink);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_rect(ctx, grect_inset(q, GEdgeInsets(6)));

  // Surlignage du tournoi en cours / prochain : cadre accent
  if (focus->id == id) {
    graphics_context_set_stroke_color(ctx, th.accent);
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_rect(ctx, grect_inset(q, GEdgeInsets(2)));
  }
#endif

  const int x0 = content.origin.x, y0 = content.origin.y, w = content.size.w;
  char buf[24];
  bool is_focus = (focus->id == id);

  // Icône jour/nuit (coin haut-droit, alignée verticalement sur l'en-tête).
  // Donnée réelle de l'API si dispo, sinon heuristique 6h-20h.
  bool day = s_weather[id].valid ? s_weather[id].is_day : slam_is_day(id);
  draw_day_night(ctx, GPoint(x0 + w - PBL_IF_ROUND_ELSE(13, 19),
                             y0 + PBL_IF_ROUND_ELSE(11, 18)),
                 day, surface, th.icon);

  // En-tête : abréviation centrée (accent si tournoi focus)
  graphics_context_set_text_color(ctx, is_focus ? th.accent : th.ink);
  graphics_draw_text(ctx, SLAMS[id].abbr, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(x0, y0 + PBL_IF_ROUND_ELSE(0, 8), w, 22),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Compte à rebours plus petit, au coin haut-gauche (tournoi focus)
  if (is_focus) {
    slam_countdown_string(focus, buf, sizeof(buf));
    graphics_context_set_text_color(ctx, th.accent);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(x0 + 2, y0 + PBL_IF_ROUND_ELSE(3, 10), w - 4, 16),
                       GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }

  // Heure locale (élément principal)
  slam_time_string(id, buf, sizeof(buf));
  graphics_context_set_text_color(ctx, th.ink);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(x0, y0 + PBL_IF_ROUND_ELSE(18, 34), w, 30),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Champion de la dernière édition (en jaune, masqué tant que non reçu)
  if (s_winner[id][0] != '\0') {
    graphics_context_set_text_color(ctx, th.accent);
    graphics_draw_text(ctx, s_winner[id], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(x0, y0 + PBL_IF_ROUND_ELSE(40, 64), w, 18),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  // Météo (température, ou ville avant la 1re synchro)
  slam_temp_string(id, buf, sizeof(buf));
  graphics_context_set_text_color(ctx, th.ink);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(x0, y0 + PBL_IF_ROUND_ELSE(52, 84), w, 18),
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
  SlamFocus focus = slam_focus_ex(&today, s_start_md);

  GRect tl = GRect(0, 0, midx, midy);
  GRect tr = GRect(midx, 0, b.size.w - midx, midy);
  GRect bl = GRect(0, midy, midx, b.size.h - midy);
  GRect br = GRect(midx, midy, b.size.w - midx, b.size.h - midy);

  draw_zone(ctx, tl, SLAM_AO, &focus);
  draw_zone(ctx, tr, SLAM_RG, &focus);
  draw_zone(ctx, bl, SLAM_WIM, &focus);
  draw_zone(ctx, br, SLAM_US, &focus);

  Theme th = slam_theme(s_theme_light);

  // Le "filet" : barre horizontale épaisse + ligne centrale verticale
  graphics_context_set_fill_color(ctx, th.ink);
  graphics_fill_rect(ctx, GRect(0, midy - 2, b.size.w, 4), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(midx - 1, 0, 2, b.size.h), 0, GCornerNone);

#if defined(PBL_ROUND)
  // Camembert : anneau (encre) sur le pourtour, et arc accent sur la portion
  // du tournoi en cours / prochain. Angles (0° = haut, sens horaire) :
  //   AO=270-360 (haut-gauche), RG=0-90 (haut-droit),
  //   W=180-270 (bas-gauche),   US=90-180 (bas-droit).
  graphics_context_set_fill_color(ctx, th.ink);
  graphics_fill_radial(ctx, b, GOvalScaleModeFitCircle, 3, 0, TRIG_MAX_ANGLE);

  static const int focus_start[SLAM_COUNT] = { 270, 0, 180, 90 };
  static const int focus_end[SLAM_COUNT]   = { 360, 90, 270, 180 };
  graphics_context_set_fill_color(ctx, th.accent);
  graphics_fill_radial(ctx, b, GOvalScaleModeFitCircle, 6,
                       DEG_TO_TRIGANGLE(focus_start[focus.id]),
                       DEG_TO_TRIGANGLE(focus_end[focus.id]));
#endif

  // Balle de tennis au centre du filet
  GPoint c = GPoint(midx, midy);
  graphics_context_set_fill_color(ctx, th.ball);
  graphics_fill_circle(ctx, c, 8);
  graphics_context_set_stroke_color(ctx, th.ink);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, c, 8);
}

// ---------------------------------------------------------------------------
// Réception météo (PebbleKit JS)
// ---------------------------------------------------------------------------
static void inbox_received(DictionaryIterator *iter, void *context) {
  // Clés individuelles par zone (objet plat envoyé par le JS).
  const uint32_t temp_k[4] = { MESSAGE_KEY_TEMP_0, MESSAGE_KEY_TEMP_1, MESSAGE_KEY_TEMP_2, MESSAGE_KEY_TEMP_3 };
  const uint32_t tz_k[4]   = { MESSAGE_KEY_TZ_0,   MESSAGE_KEY_TZ_1,   MESSAGE_KEY_TZ_2,   MESSAGE_KEY_TZ_3 };
  const uint32_t day_k[4]  = { MESSAGE_KEY_DAY_0,  MESSAGE_KEY_DAY_1,  MESSAGE_KEY_DAY_2,  MESSAGE_KEY_DAY_3 };
  const uint32_t win_k[4]  = { MESSAGE_KEY_WIN_0,  MESSAGE_KEY_WIN_1,  MESSAGE_KEY_WIN_2,  MESSAGE_KEY_WIN_3 };
  const uint32_t cond_k[4] = { MESSAGE_KEY_COND_0, MESSAGE_KEY_COND_1, MESSAGE_KEY_COND_2, MESSAGE_KEY_COND_3 };
  const uint32_t dstart_k[4] = { MESSAGE_KEY_DSTART_0, MESSAGE_KEY_DSTART_1, MESSAGE_KEY_DSTART_2, MESSAGE_KEY_DSTART_3 };

  for (int i = 0; i < SLAM_COUNT; i++) {
    Tuple *t;
    if ((t = dict_find(iter, temp_k[i]))) {
      s_weather[i].temp = t->value->int32;
      s_weather[i].valid = true;
    }
    if ((t = dict_find(iter, tz_k[i]))) {
      s_tz_offset[i] = t->value->int32;
    }
    if ((t = dict_find(iter, day_k[i]))) {
      s_weather[i].is_day = (t->value->int32 != 0);
    }
    if ((t = dict_find(iter, win_k[i]))) {
      strncpy(s_winner[i], t->value->cstring, sizeof(s_winner[i]) - 1);
      s_winner[i][sizeof(s_winner[i]) - 1] = '\0';
    }
    if ((t = dict_find(iter, cond_k[i]))) {
      strncpy(s_cond[i], t->value->cstring, sizeof(s_cond[i]) - 1);
      s_cond[i][sizeof(s_cond[i]) - 1] = '\0';
    }
    if ((t = dict_find(iter, dstart_k[i]))) {
      s_start_md[i] = t->value->int32;
    }
  }

  Tuple *theme_t = dict_find(iter, MESSAGE_KEY_THEME);
  if (theme_t) {
    s_theme_light = (theme_t->value->int32 != 0);
    if (s_window) {
      window_set_background_color(s_window, slam_theme(s_theme_light).bg);
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
