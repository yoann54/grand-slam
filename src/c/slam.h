#pragma once
#include <pebble.h>

// Les 4 tournois du Grand Chelem, dans l'ordre des quadrants :
//   0 = haut-gauche, 1 = haut-droit, 2 = bas-gauche, 3 = bas-droit
typedef enum {
  SLAM_AO = 0,   // Australian Open  - Melbourne
  SLAM_RG = 1,   // Roland-Garros    - Paris
  SLAM_WIM = 2,  // Wimbledon        - Londres
  SLAM_US = 3,   // US Open          - New York
  SLAM_COUNT = 4
} SlamId;

typedef struct {
  const char *abbr;        // "AO", "RG", "W", "US"
  const char *city;        // "Melbourne", ...
  GColor surface;          // couleur de la surface du court
  GColor text;             // couleur du texte/lignes lisible sur la surface
  int32_t tz_offset_sec;   // décalage UTC provisoire (sera remplacé par le JS météo)
  uint8_t start_month;     // mois de début du tournoi (1-12)
  uint8_t start_day;       // jour de début
  uint8_t duration_days;   // durée approximative du tournoi
} Slam;

// Tournoi mis en avant (en cours ou prochain) + compte à rebours.
typedef struct {
  SlamId id;        // tournoi à surligner
  bool live;        // true si le tournoi est en cours aujourd'hui
  int days_until;   // jours avant le début (0 si live)
} SlamFocus;

SlamFocus slam_focus(struct tm *today);

// Données météo reçues du téléphone (PebbleKit JS). Remplies en Phase 3.
typedef struct {
  int16_t temp;            // température (unité selon config)
  uint16_t cond;           // code condition OpenWeatherMap (0 = inconnu)
  bool is_day;             // jour/nuit au lieu du tournoi (depuis l'icône API)
  bool valid;              // true une fois la 1re donnée reçue
} SlamWeather;

extern const Slam SLAMS[SLAM_COUNT];
