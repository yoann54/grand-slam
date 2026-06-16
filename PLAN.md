# 🎾 Grand Slam — Watchface Pebble Time 2

Watchface dédiée au tennis : **4 zones = 4 tournois du Grand Chelem**, chacune affichant
l'**heure locale** et la **météo** du lieu, sur un design de **court de tennis vu du dessus**.

---

## 1. Cible technique

| Élément | Choix |
|---|---|
| Appareil | Pebble Time 2 → plateforme SDK **`emery`** |
| Écran | 200 × 228 px, 64 couleurs |
| Langage | **C** (rendu/watchface) + **PebbleKit JS** (météo + fuseaux côté téléphone) |
| Config utilisateur | **Clay** (unités °C/°F, thème, clé API) |
| Météo | **OpenWeatherMap** (current weather par lat/lon — renvoie aussi le décalage horaire → gère le DST gratuitement) |
| Rafraîchissement | Météo toutes les ~30 min + au lancement ; tick à la minute (pas de secondes → batterie) |

---

## 2. Les 4 zones (quadrants)

```
+---------------------+---------------------+
|   AUSTRALIAN OPEN   |    ROLAND-GARROS    |
|   Melbourne 🇦🇺      |    Paris 🇫🇷         |
|   dur (bleu foncé)  |    terre battue     |
+---------------------+---------------------+   <- "filet" (lignes blanches)
|     WIMBLEDON       |      US OPEN        |
|     Londres 🇬🇧      |   New York 🇺🇸       |
|   gazon (vert)      |   dur (bleu vif)    |
+---------------------+---------------------+
```

Chaque zone (≈100 × 114 px) affiche :
- **Abréviation / ville** du tournoi
- **Heure locale** (HH:MM, police large = élément principal)
- **Météo** : icône condition + température
- **Couleur de fond = couleur de la surface** du tournoi (identité visuelle forte)

### Données par tournoi

| Tournoi | Ville | Coordonnées | Surface / couleur | Période |
|---|---|---|---|---|
| Australian Open | Melbourne | -37.84, 144.98 | Dur — bleu foncé | mi-fin janvier |
| Roland-Garros | Paris | 48.85, 2.25 | Terre battue — orange | fin mai → début juin |
| Wimbledon | Londres | 51.43, -0.21 | Gazon — vert | fin juin → mi-juillet |
| US Open | New York | 40.75, -73.85 | Dur — bleu vif | fin août → début sept. |

---

## 3. Design "court de tennis"

- **Filet central** : lignes blanches en croix divisant les 4 zones (la barre horizontale = filet).
- **Lignes de court** : bordure + ligne de service stylisée dans chaque quadrant (sobre, sans surcharger).
- **Balle de tennis** 🎾 au centre du filet comme accent jaune.

---

## 4. Mes idées en plus 💡

1. **Tournoi actif mis en avant** — selon la date du jour, la zone du Grand Chelem en cours
   (ou le prochain) est mise en valeur : bordure lumineuse / balle qui s'y déplace.
2. **Jour/nuit par zone** — les 4 villes sont sur des fuseaux très différents. Petit icône
   soleil/lune (ou assombrissement) si c'est la nuit là-bas → Melbourne en pleine nuit
   pendant que Paris est en plein jour. Joli ET utile.
3. **Compte à rebours** vers le prochain Grand Chelem (ex : "RG dans 12 j").
4. **Décalage horaire correct (DST inclus)** : on récupère l'offset UTC de chaque ville
   directement depuis la réponse météo → pas de bug d'heure d'été à coder à la main.

---

## 5. Architecture du code

```
grand-slam/
  package.json            # appinfo : capabilities ["location","configurable"], targetPlatforms ["emery"]
  src/
    c/
      grand_slam.c         # entrée watchface, fenêtre, tick handler
      court.c / .h         # rendu du court (lignes, filet, balle)
      zone.c / .h          # rendu d'une zone (heure locale + météo)
      weather.c / .h       # état météo + AppMessage (réception)
      slam.c / .h          # table des 4 tournois (coords, couleurs, dates)
    pkjs/
      index.js             # PebbleKit JS : fetch météo 4 villes, envoi AppMessage
    common/
      message_keys.json    # clés AppMessage partagées C <-> JS
  resources/
    images/                # icônes météo, balle de tennis, drapeaux
```

---

## 6. Plan d'exécution par phases

- **Phase 0 — Scaffolding** : `pebble new-project`, cible `emery`, build + emulator OK ("court vide").
- **Phase 1 — Design statique** : court, 4 quadrants, couleurs de surface, filet, balle, labels.
- **Phase 2 — Heures locales** : table des tournois + offsets (en dur d'abord, puis via JS) ;
  affichage HH:MM par zone.
- **Phase 3 — Météo** : PebbleKit JS → OpenWeatherMap → AppMessage → temp + icône par zone.
- **Phase 4 — Polish** : tournoi actif, jour/nuit, compte à rebours, balle accent.
- **Phase 5 — Config Clay** : unités °C/°F, thème, clé API.
- **Phase 6 — Tests** : emulator `emery` + device réel, vérif batterie.

---

## 7. Décisions à caler avant de coder

1. **Clé API OpenWeatherMap** — gratuite (60 appels/min). À créer sur openweathermap.org,
   ou on en embarque une le temps du dev.
2. **Unité par défaut** — °C (recommandé) ou °F.
3. **Plateformes de build** — `emery` seul, ou aussi `basalt`/`chalk` pour tester sur l'émulateur classique.
