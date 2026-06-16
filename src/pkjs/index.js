// PebbleKit JS — récupère la météo des 4 villes du Grand Chelem via
// OpenWeatherMap et l'envoie à la montre (heure locale + jour/nuit + température),
// envoie les vainqueurs de la dernière édition, et sert la page de réglages.

var config = require('./config');
var buildConfigPage = require('./config_page');
var FALLBACK_WINNERS = require('./winners'); // valeurs embarquées (offline / 1er lancement)

// winners.json hébergé sur GitHub : édité après chaque tournoi → toutes les
// montres se mettent à jour sans republier l'app. (winners.json à la racine du repo)
var WINNERS_URL = 'https://raw.githubusercontent.com/yoann54/grand-slam/main/winners.json';
var WINNERS_CACHE_KEY = 'grandslam.winners';

// La clé API est saisie par l'utilisateur dans la config (reste sur le téléphone).
// Vide dans le code source : aucune clé ne doit être commitée dans un repo public.
// (Pour tester à l'émulateur sans config, on peut injecter via `pebble send-app-message`.)
var DEFAULT_API_KEY = '';

function apiKey(cfg) { return (cfg.apiKey && cfg.apiKey.trim()) || DEFAULT_API_KEY; }

// Ordre = quadrants : 0 AO/Melbourne, 1 RG/Paris, 2 W/Londres, 3 US/New York
var CITIES = [
  { lat: -37.84, lon: 144.98 },
  { lat: 48.85,  lon: 2.25 },
  { lat: 51.43,  lon: -0.21 },
  { lat: 40.75,  lon: -73.85 }
];

function xhrJSON(url, cb) {
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function () {
    if (req.status >= 200 && req.status < 300) {
      try { cb(null, JSON.parse(req.responseText)); }
      catch (e) { cb(e); }
    } else {
      cb(new Error('HTTP ' + req.status));
    }
  };
  req.onerror = function () { cb(new Error('network error')); };
  req.send();
}

function fetchWeather() {
  var cfg = config.load();
  var key = apiKey(cfg);
  var units = cfg.units || 'metric';
  var temps = [0, 0, 0, 0];
  var conds = [0, 0, 0, 0];
  var tz = [0, 0, 0, 0];
  var isDay = [1, 1, 1, 1];
  var done = 0;

  CITIES.forEach(function (city, i) {
    var url = 'https://api.openweathermap.org/data/2.5/weather?lat=' + city.lat +
              '&lon=' + city.lon + '&units=' + units + '&appid=' + key;
    xhrJSON(url, function (err, data) {
      done++;
      if (!err && data && data.main) {
        temps[i] = Math.round(data.main.temp);
        conds[i] = (data.weather && data.weather[0]) ? data.weather[0].id : 0;
        tz[i] = data.timezone || 0; // décalage UTC en secondes (DST inclus)
        var icon = (data.weather && data.weather[0]) ? data.weather[0].icon : '';
        isDay[i] = (icon.charAt(icon.length - 1) === 'n') ? 0 : 1;
      } else {
        console.log('Weather error city ' + i + ': ' + (err ? err.message : 'no data'));
      }
      if (done === CITIES.length) {
        sendWeather(temps, conds, tz, isDay);
      }
    });
  });
}

function sendWeather(temps, conds, tz, isDay) {
  // Clés individuelles (objet plat) : pas de dépendance à l'expansion des clés-tableau.
  var msg = { 'READY': 1 };
  for (var i = 0; i < 4; i++) {
    msg['TEMP_' + i] = temps[i];
    msg['TZ_' + i] = tz[i];
    msg['DAY_' + i] = isDay[i];
  }
  Pebble.sendAppMessage(msg,
    function () { console.log('Weather sent: ' + JSON.stringify(temps)); },
    function (e) { console.log('Weather send failed: ' + JSON.stringify(e)); });
}

// Un jeu de vainqueurs valide = atp et wta, chacun un tableau de 4 noms.
function validWinners(o) {
  return o && Array.isArray(o.atp) && o.atp.length === 4 &&
         Array.isArray(o.wta) && o.wta.length === 4;
}

// Vainqueurs actifs : cache distant si dispo et valide, sinon valeurs embarquées.
function activeWinners() {
  try {
    var raw = localStorage.getItem(WINNERS_CACHE_KEY);
    if (raw) {
      var o = JSON.parse(raw);
      if (validWinners(o)) { return o; }
    }
  } catch (e) { /* cache illisible → fallback */ }
  return FALLBACK_WINNERS;
}

// Va chercher le winners.json distant et le met en cache (silencieux si échec).
function refreshWinners(cb) {
  xhrJSON(WINNERS_URL, function (err, data) {
    if (!err && validWinners(data)) {
      try {
        localStorage.setItem(WINNERS_CACHE_KEY, JSON.stringify({ atp: data.atp, wta: data.wta }));
        console.log('Winners updated from remote (' + (data.updated || '?') + ')');
      } catch (e) { /* storage plein → on garde l'ancien */ }
    } else {
      console.log('Winners remote fetch failed → cache/fallback');
    }
    if (cb) { cb(); }
  });
}

function sendWinners(cfg) {
  var w = activeWinners();
  var names = (cfg.tour === 'wta') ? w.wta : w.atp;
  var msg = {};
  for (var i = 0; i < 4; i++) { msg['WIN_' + i] = String(names[i] || ''); }
  Pebble.sendAppMessage(msg,
    function () { console.log('Winners sent (' + cfg.tour + '): ' + JSON.stringify(names)); },
    function (e) { console.log('Winners send failed: ' + JSON.stringify(e)); });
}

// ---- Page de réglages -------------------------------------------------------
Pebble.addEventListener('showConfiguration', function () {
  var cfg = config.load();
  try {
    var html = buildConfigPage(cfg);
    Pebble.openURL('data:text/html;charset=utf-8,' + encodeURIComponent(html));
  } catch (e) {
    console.log('config open FAILED: ' + (e && e.message));
  }
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) { return; } // annulé
  var s = null;
  try { s = JSON.parse(decodeURIComponent(e.response)); }
  catch (e1) { try { s = JSON.parse(e.response); } catch (e2) { s = null; } }
  if (!s) { return; }

  var cfg = config.load();
  if (typeof s.apiKey === 'string') { cfg.apiKey = s.apiKey; }
  if (s.units === 'metric' || s.units === 'imperial') { cfg.units = s.units; }
  if (s.tour === 'atp' || s.tour === 'wta') { cfg.tour = s.tour; }
  config.save(cfg);
  console.log('settings saved: units=' + cfg.units + ' tour=' + cfg.tour);

  fetchWeather();      // re-fetch avec les nouvelles unités / clé
  sendWinners(cfg);    // re-envoyer selon le tour choisi
});

// ---- Cycle de vie -----------------------------------------------------------
Pebble.addEventListener('ready', function () {
  var cfg = config.load();
  config.save(cfg); // persiste un blob de 1er lancement / migré
  console.log('PebbleKit JS ready');
  sendWinners(cfg);                                  // affichage instantané (cache/fallback)
  refreshWinners(function () { sendWinners(cfg); }); // puis MAJ depuis GitHub si dispo
  fetchWeather();
  setInterval(fetchWeather, 30 * 60 * 1000); // rafraîchissement météo (30 min)
});

// La montre peut demander un rafraîchissement (ex : reprise depuis veille)
Pebble.addEventListener('appmessage', function () {
  fetchWeather();
});
