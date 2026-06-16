/*
 * config_page.js — page de réglages auto-contenue (HTML), servie en data: URI
 * depuis index.js. On n'utilise PAS Clay (ne s'affiche pas dans l'app Core).
 * Renvoie les réglages via `pebblejs://close#<json>`.
 *
 * Multi-langue : les libellés FR/EN sont embarqués ; la langue est choisie côté
 * webview selon navigator.language (fr → français, sinon anglais).
 *
 * La clé API OpenWeatherMap reste dans le storage du téléphone.
 */

var OWM_KEYS_URL = 'https://home.openweathermap.org/api_keys';

var I18N = {
  fr: {
    sub: 'Heure & météo des 4 tournois du Grand Chelem. Votre clé reste sur le téléphone.',
    apiKey: 'Clé API OpenWeatherMap',
    apiPlaceholder: 'collez votre clé',
    getKey: 'Obtenir une clé gratuite →',
    units: 'Unité de température',
    celsius: 'Celsius (°C)',
    fahrenheit: 'Fahrenheit (°F)',
    winner: 'Vainqueur affiché (dernière édition)',
    atp: 'ATP (messieurs)',
    wta: 'WTA (dames)',
    theme: 'Thème',
    dark: 'Sombre',
    light: 'Clair',
    save: 'Enregistrer',
    support: '☕ Soutenir le développeur'
  },
  en: {
    sub: 'Time & weather for the 4 Grand Slam tournaments. Your key stays on your phone.',
    apiKey: 'OpenWeatherMap API key',
    apiPlaceholder: 'paste your key',
    getKey: 'Get a free key →',
    units: 'Temperature unit',
    celsius: 'Celsius (°C)',
    fahrenheit: 'Fahrenheit (°F)',
    winner: 'Champion shown (latest edition)',
    atp: 'ATP (men)',
    wta: 'WTA (women)',
    theme: 'Theme',
    dark: 'Dark',
    light: 'Light',
    save: 'Save',
    support: '☕ Support the developer'
  }
};

var PAGE_JS = [
  '(function(){',
  'var byId=function(i){return document.getElementById(i);};',
  'var lang=((navigator.language||navigator.userLanguage||"en")+"").slice(0,2).toLowerCase();',
  'if(lang!=="fr"){lang="en";}',
  'var T=D.i18n[lang];',
  'byId("sub").textContent=T.sub;',
  'byId("lblApi").textContent=T.apiKey;',
  'byId("apiKey").placeholder=T.apiPlaceholder;',
  'byId("getKey").textContent=T.getKey;',
  'byId("lblUnits").textContent=T.units;',
  'byId("optC").textContent=T.celsius;',
  'byId("optF").textContent=T.fahrenheit;',
  'byId("lblWinner").textContent=T.winner;',
  'byId("optAtp").textContent=T.atp;',
  'byId("optWta").textContent=T.wta;',
  'byId("lblTheme").textContent=T.theme;',
  'byId("optDark").textContent=T.dark;',
  'byId("optLight").textContent=T.light;',
  'byId("save").textContent=T.save;',
  'byId("support").textContent=T.support;',
  'byId("apiKey").value=D.apiKey||"";',
  'byId("units").value=D.units||"metric";',
  'byId("tour").value=D.tour||"atp";',
  'byId("theme").value=D.theme||"dark";',
  'byId("save").addEventListener("click",function(){',
  'var out={apiKey:byId("apiKey").value.trim(),units:byId("units").value,tour:byId("tour").value,theme:byId("theme").value};',
  'document.location="pebblejs://close#"+encodeURIComponent(JSON.stringify(out));',
  '});',
  '})();'
].join('');

var STYLE = [
  'body{margin:0;background:#0b5d1e;color:#fff;font-family:-apple-system,Roboto,Helvetica,sans-serif;padding:18px}',
  'h1{color:#e9ff3d;font-size:24px;margin:0 0 2px}',
  '.sub{color:#cfe9d4;font-size:13px;margin-bottom:18px}',
  'label{display:block;margin:16px 0 5px;font-size:13px;color:#cfe9d4}',
  'input,select{width:100%;box-sizing:border-box;padding:11px;border-radius:9px;border:1px solid #0a4a18;background:#08400f;color:#fff;font-size:16px}',
  '.hint{color:#a9cdb0;font-size:11px;margin-top:6px}',
  '.hint a{color:#e9ff3d;text-decoration:none;font-weight:bold}',
  'button{width:100%;margin-top:24px;margin-bottom:18px;padding:15px;border:0;border-radius:11px;background:#e9ff3d;color:#0b5d1e;font-size:17px;font-weight:bold}',
  '.support{display:block;text-align:center;text-decoration:none;background:#0070ba;color:#fff;padding:13px;border-radius:11px;margin-bottom:24px;font-size:15px;font-weight:bold}'
].join('');

function buildConfigPage(cfg) {
  var data = {
    apiKey: cfg.apiKey || '',
    units: cfg.units || 'metric',
    tour: cfg.tour || 'atp',
    theme: cfg.theme || 'dark',
    i18n: I18N
  };

  return [
    '<!DOCTYPE html><html><head><meta charset="utf-8">',
    '<meta name="viewport" content="width=device-width,initial-scale=1">',
    '<title>Grand Slam</title><style>', STYLE, '</style></head><body>',
    '<h1>🎾 Grand Slam</h1>',
    '<div class="sub" id="sub"></div>',
    '<label id="lblApi"></label>',
    '<input id="apiKey" type="password">',
    '<div class="hint"><a id="getKey" href="', OWM_KEYS_URL, '" target="_blank" rel="noopener"></a></div>',
    '<label id="lblUnits"></label>',
    '<select id="units"><option id="optC" value="metric"></option><option id="optF" value="imperial"></option></select>',
    '<label id="lblWinner"></label>',
    '<select id="tour"><option id="optAtp" value="atp"></option><option id="optWta" value="wta"></option></select>',
    '<label id="lblTheme"></label>',
    '<select id="theme"><option id="optDark" value="dark"></option><option id="optLight" value="light"></option></select>',
    '<button id="save"></button>',
    '<a id="support" href="https://paypal.me/yoadadev" target="_blank" rel="noopener" class="support"></a>',
    '<script>var D=', JSON.stringify(data), ';</script>',
    '<script>', PAGE_JS, '</script>',
    '</body></html>'
  ].join('');
}

module.exports = buildConfigPage;
