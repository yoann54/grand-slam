# Publier Grand Slam sur l'appstore Pebble

Le build est prêt. L'upload se fait manuellement (connecté) sur le portail — ce
fichier est la checklist + les textes/assets à coller.

## Où

- **Docs :** <https://developer.repebble.com/guides/appstore-publishing/publishing-an-app/>
- **Portail d'upload :** <https://dev-portal.rebble.io/> — les apps soumises ici
  apparaissent aussi dans l'appstore Pebble.

## Étapes

1. Se connecter → **Add a Watchface**.
2. Détails : titre, URL du code source, email de support (ci-dessous).
3. **Category** : Faces (watchface).
4. Uploader les icônes **large** et **small** (`store/icon-large-144.png`, `store/icon-small-48.png`).
5. **Create** → ouvre la page de listing.
6. **Add a release** → uploader `build/grand-slam.pbw` (+ notes de version).
7. **Save**, recharger, **Publish** à côté de la release.
8. **Manage Asset Collections** → une collection par plateforme supportée (**emery, basalt, chalk**).
9. Par collection : description + captures + bannière marketing (`store/banner-720x320.png`).
10. **Publish** (ou **Publish Privately** pour un lien privé ; le public est irréversible).
11. Recharger pour récupérer le lien appstore partageable + le deep link.

## Métadonnées à coller

| Champ | Valeur |
|---|---|
| Title | Grand Slam |
| Category | Faces |
| Support email | yoann.piconcely@skores.com |
| Source code URL | https://github.com/yoann54/grand-slam |
| Tagline | Tennis time & weather, the 4 majors on your wrist. |

**Short description**

> The four Grand Slams on your wrist: local time and weather for Melbourne,
> Paris, London and New York.

**Long description**

> Grand Slam turns your Pebble into a tennis court seen from above, split into
> four zones — one per major (Australian Open, Roland-Garros, Wimbledon, US Open),
> each in its surface colour.
>
> • Local time and live weather for each tournament's city.
> • Day/night indicator per zone (sun/moon) — see Melbourne at night while Paris is in daylight.
> • The current or upcoming Grand Slam is highlighted, with a countdown (e.g. "W J-14").
> • Defending champion of each major, with an ATP / WTA switch in the settings.
> • Time zones handle Daylight Saving automatically.
>
> Tap the gear icon in the Pebble phone app to set your free OpenWeatherMap API
> key, choose °C/°F, and pick ATP or WTA.

_Grand Slam est "configurable" : la roue dentée apparaît dans l'app Pebble et
ouvre les réglages (clé API, unités, ATP/WTA)._

## Assets (dossier `store/`)

| Fichier | Taille | Usage |
|---|---|---|
| `icon-large-144.png` | 144×144 | Grande icône appstore |
| `icon-small-48.png` | 48×48 | Petite icône appstore |
| `menu-icon-25.png` | 25×25 | Icône menu (optionnelle pour une watchface) |
| `banner-720x320.png` | 720×320 | Bannière marketing |
| `screenshots/emery_idle.png` | 200×228 | Collection emery (Pebble Time 2) |
| `screenshots/basalt_idle.png` | 144×168 | Collection basalt |
| `screenshots/chalk_idle.png` | 180×180 | Collection chalk (rond) |

⚠️ **Nom des captures = `<plateforme>_xxx.png`** (underscore obligatoire) : `pebble publish`
déduit la plateforme via `basename.split("_")`. `emery-idle.png` est refusé, `emery_idle.png` OK.

Régénérer icônes/bannière : `python3 tools/gen_assets.py`.
Recapturer une plateforme : `pebble install --emulator <p> && pebble screenshot --emulator <p> store/screenshots/<p>_idle.png`.

## Publier en une commande (CLI)

```sh
pebble publish --is-published --non-interactive --no-gif-all-platforms \
  --name "Grand Slam" --version "1.0.0" --category "Faces" \
  --source "https://github.com/yoann54/grand-slam" \
  --description "..." --release-notes "..." \
  --icon-small store/icon-small-48.png --icon-large store/icon-large-144.png \
  --screenshots store/screenshots/emery_idle.png store/screenshots/basalt_idle.png store/screenshots/chalk_idle.png
```

**Publié le 2026-06-16** → app : <https://apps.rePebble.com/1daaaed318be4c32b8033367> ·
dashboard (changelog) : <https://appstore-api.repebble.com/dashboard>.
Pour une mise à jour : bump `version` dans `package.json`, `pebble build`, puis relancer
`pebble publish` (mappé par UUID → met à jour l'app existante).

## Note : mise à jour des vainqueurs

Les champions ne nécessitent **pas** de republier l'app : ils sont lus depuis
`winners.json` (racine du repo) via GitHub. Après chaque tournoi, éditer ce
fichier et `git push` — toutes les montres se mettent à jour au prochain lancement.

## Pre-flight

- [x] Build propre emery / basalt / chalk
- [x] Page de config (HTML maison, pas Clay) — la clé API reste sur le téléphone
- [x] Vainqueurs auto-update via winners.json GitHub
- [ ] Test sur matériel réel (météo réelle + page de config)
- [x] URL du code source : https://github.com/yoann54/grand-slam
- [x] **Publié** sur l'appstore Rebble (2026-06-16, v1.0.0)
