#include "slam.h"

// Couleurs choisies dans la palette 64 couleurs de Pebble pour évoquer
// la surface réelle de chaque tournoi. Lignes/texte en blanc (lignes de court).
//
// Offsets UTC : valeurs de juin 2026 (provisoires). Le PebbleKit JS enverra
// l'offset réel de chaque ville (DST inclus) en Phase 3.
//
// Dates : fenêtres approximatives des tournois (servent au surlignage du
// tournoi en cours / prochain et au compte à rebours).
const Slam SLAMS[SLAM_COUNT] = {
  // Australian Open — Melbourne — dur bleu — AEST (UTC+10) — mi-janvier
  [SLAM_AO] = {
    .abbr = "AO", .city = "Melbourne",
    .surface_dark = { .argb = GColorCobaltBlueARGB8 },
    .surface_light = { .argb = GColorBabyBlueEyesARGB8 },
    .tz_offset_sec = 10 * 3600,
    .start_month = 1, .start_day = 12, .duration_days = 14,
  },
  // Roland-Garros — Paris — terre battue — CEST (UTC+2) — fin mai
  [SLAM_RG] = {
    .abbr = "RG", .city = "Paris",
    .surface_dark = { .argb = GColorOrangeARGB8 },
    .surface_light = { .argb = GColorMelonARGB8 },
    .tz_offset_sec = 2 * 3600,
    .start_month = 5, .start_day = 25, .duration_days = 15,
  },
  // Wimbledon — Londres — gazon — BST (UTC+1) — fin juin / début juillet
  [SLAM_WIM] = {
    .abbr = "W", .city = "London",
    .surface_dark = { .argb = GColorIslamicGreenARGB8 },
    .surface_light = { .argb = GColorMintGreenARGB8 },
    .tz_offset_sec = 1 * 3600,
    .start_month = 6, .start_day = 30, .duration_days = 14,
  },
  // US Open — New York — dur bleu vif — EDT (UTC-4) — fin août
  [SLAM_US] = {
    .abbr = "US", .city = "New York",
    .surface_dark = { .argb = GColorVividCeruleanARGB8 },
    .surface_light = { .argb = GColorCelesteARGB8 },
    .tz_offset_sec = -4 * 3600,
    .start_month = 8, .start_day = 25, .duration_days = 15,
  },
};

GColor slam_surface(SlamId id, bool light) {
  return light ? SLAMS[id].surface_light : SLAMS[id].surface_dark;
}

Theme slam_theme(bool light) {
  if (light) {
    // Thème clair : surfaces pastel, encre noire, accent rouge profond.
    return (Theme){
      .ink    = { .argb = GColorBlackARGB8 },
      .accent = { .argb = GColorDarkCandyAppleRedARGB8 },
      .bg     = { .argb = GColorWhiteARGB8 },
      .ball   = { .argb = GColorYellowARGB8 },
      .icon   = { .argb = GColorWindsorTanARGB8 },
    };
  }
  // Thème sombre (par défaut) : surfaces vives, encre blanche, accent jaune.
  return (Theme){
    .ink    = { .argb = GColorWhiteARGB8 },
    .accent = { .argb = GColorYellowARGB8 },
    .bg     = { .argb = GColorBlackARGB8 },
    .ball   = { .argb = GColorYellowARGB8 },
    .icon   = { .argb = GColorYellowARGB8 },
  };
}

// Jour de l'année (0-364) approximatif pour une date (mois 1-12, jour 1-31).
// Approximation non bissextile : suffisant pour ordonner les tournois.
static int day_of_year(int month, int day) {
  static const int cum[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  return cum[month - 1] + (day - 1);
}

// Détermine le tournoi à mettre en avant : celui en cours aujourd'hui, sinon
// le prochain à venir (avec le nombre de jours avant son début).
SlamFocus slam_focus(struct tm *today) {
  return slam_focus_ex(today, NULL);
}

SlamFocus slam_focus_ex(struct tm *today, const uint16_t *override_md) {
  int today_yd = day_of_year(today->tm_mon + 1, today->tm_mday);

  SlamFocus focus = { .id = SLAM_AO, .live = false, .days_until = 9999 };

  for (int i = 0; i < SLAM_COUNT; i++) {
    int month = SLAMS[i].start_month, day = SLAMS[i].start_day;
    if (override_md && override_md[i] != 0) {
      month = override_md[i] / 100;
      day = override_md[i] % 100;
    }
    int start = day_of_year(month, day);
    int end = start + SLAMS[i].duration_days;

    // En cours ?
    if (today_yd >= start && today_yd <= end) {
      focus.id = (SlamId)i;
      focus.live = true;
      focus.days_until = 0;
      return focus;
    }

    // Sinon, jours avant le prochain départ (avec passage à l'année suivante)
    int delta = start - today_yd;
    if (delta < 0) {
      delta += 365;
    }
    if (delta < focus.days_until) {
      focus.id = (SlamId)i;
      focus.live = false;
      focus.days_until = delta;
    }
  }
  return focus;
}
