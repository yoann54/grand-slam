/*
 * config.js — réglages persistés côté téléphone (jamais sur la montre pour la clé API).
 *
 * Même approche que Murmur : un schéma versionné avec une échelle de migration,
 * pour pouvoir ajouter des champs lors d'une mise à jour sans corrompre le blob
 * sauvegardé. Storage abstrait pour être testable sous Node (pas de localStorage).
 */

var SCHEMA_VERSION = 1;
var STORE_KEY = 'grandslam.config';

var memoryStore = {};
function storage() {
  if (typeof localStorage !== 'undefined' && localStorage) {
    return localStorage;
  }
  return {
    getItem: function (k) { return Object.prototype.hasOwnProperty.call(memoryStore, k) ? memoryStore[k] : null; },
    setItem: function (k, v) { memoryStore[k] = String(v); },
    removeItem: function (k) { delete memoryStore[k]; }
  };
}

function defaults() {
  return {
    version: SCHEMA_VERSION,
    apiKey: '',          // clé OpenWeatherMap (reste sur le téléphone)
    units: 'metric',     // 'metric' = °C, 'imperial' = °F
    tour: 'atp'          // 'atp' (messieurs) ou 'wta' (dames) pour le vainqueur affiché
  };
}

function migrate(cfg) {
  if (!cfg || typeof cfg !== 'object') {
    return defaults();
  }
  var base = defaults();
  Object.keys(base).forEach(function (k) {
    if (cfg[k] === undefined) { cfg[k] = base[k]; }
  });
  // Future : while (cfg.version < SCHEMA_VERSION) { ...; cfg.version++; }
  cfg.version = SCHEMA_VERSION;
  return cfg;
}

function load() {
  var raw = storage().getItem(STORE_KEY);
  if (!raw) { return defaults(); }
  try {
    return migrate(JSON.parse(raw));
  } catch (e) {
    return defaults();
  }
}

function save(cfg) {
  cfg.version = SCHEMA_VERSION;
  storage().setItem(STORE_KEY, JSON.stringify(cfg));
  return cfg;
}

module.exports = {
  SCHEMA_VERSION: SCHEMA_VERSION,
  defaults: defaults,
  migrate: migrate,
  load: load,
  save: save
};
