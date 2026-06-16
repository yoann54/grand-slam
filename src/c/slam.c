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
  // Australian Open — Melbourne — dur bleu foncé — AEST (UTC+10) — mi-janvier
  [SLAM_AO] = {
    .abbr = "AO", .city = "Melbourne",
    .surface = { .argb = GColorCobaltBlueARGB8 },
    .text = { .argb = GColorWhiteARGB8 },
    .tz_offset_sec = 10 * 3600,
    .start_month = 1, .start_day = 12, .duration_days = 14,
  },
  // Roland-Garros — Paris — terre battue (orange terracotta) — CEST (UTC+2) — fin mai
  [SLAM_RG] = {
    .abbr = "RG", .city = "Paris",
    .surface = { .argb = GColorOrangeARGB8 },
    .text = { .argb = GColorWhiteARGB8 },
    .tz_offset_sec = 2 * 3600,
    .start_month = 5, .start_day = 25, .duration_days = 15,
  },
  // Wimbledon — Londres — gazon — BST (UTC+1) — fin juin / début juillet
  [SLAM_WIM] = {
    .abbr = "W", .city = "London",
    .surface = { .argb = GColorIslamicGreenARGB8 },
    .text = { .argb = GColorWhiteARGB8 },
    .tz_offset_sec = 1 * 3600,
    .start_month = 6, .start_day = 30, .duration_days = 14,
  },
  // US Open — New York — dur bleu vif — EDT (UTC-4) — fin août
  [SLAM_US] = {
    .abbr = "US", .city = "New York",
    .surface = { .argb = GColorVividCeruleanARGB8 },
    .text = { .argb = GColorWhiteARGB8 },
    .tz_offset_sec = -4 * 3600,
    .start_month = 8, .start_day = 25, .duration_days = 15,
  },
};

// Jour de l'année (0-364) approximatif pour une date (mois 1-12, jour 1-31).
// Approximation non bissextile : suffisant pour ordonner les tournois.
static int day_of_year(int month, int day) {
  static const int cum[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  return cum[month - 1] + (day - 1);
}

// Détermine le tournoi à mettre en avant : celui en cours aujourd'hui, sinon
// le prochain à venir (avec le nombre de jours avant son début).
SlamFocus slam_focus(struct tm *today) {
  int today_yd = day_of_year(today->tm_mon + 1, today->tm_mday);

  SlamFocus focus = { .id = SLAM_AO, .live = false, .days_until = 9999 };

  for (int i = 0; i < SLAM_COUNT; i++) {
    int start = day_of_year(SLAMS[i].start_month, SLAMS[i].start_day);
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
