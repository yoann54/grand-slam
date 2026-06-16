/*
 * config_page.js — page de réglages auto-contenue (HTML), servie en data: URI
 * depuis index.js. On n'utilise PAS Clay : sa page (~90 KB) ne s'affiche pas dans
 * la webview de la nouvelle app Core. Cette page fait quelques Ko, fonctionne
 * hors-ligne, et renvoie les réglages via `pebblejs://close#<json>` que le JS lit
 * dans son handler webviewclosed.
 *
 * La clé API OpenWeatherMap reste dans le storage du téléphone — jamais envoyée
 * à la montre (c'est le JS du téléphone qui appelle l'API météo).
 */

var PAGE_JS = [
  '(function(){',
  'var byId=function(i){return document.getElementById(i);};',
  'byId("apiKey").value=D.apiKey||"";',
  'byId("units").value=D.units||"metric";',
  'byId("tour").value=D.tour||"atp";',
  'byId("save").addEventListener("click",function(){',
  'var out={apiKey:byId("apiKey").value.trim(),units:byId("units").value,tour:byId("tour").value};',
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
  '.hint{color:#a9cdb0;font-size:11px;margin-top:4px}',
  'button{width:100%;margin-top:24px;margin-bottom:18px;padding:15px;border:0;border-radius:11px;background:#e9ff3d;color:#0b5d1e;font-size:17px;font-weight:bold}',
  '.support{display:block;text-align:center;text-decoration:none;background:#0070ba;color:#fff;padding:13px;border-radius:11px;margin-bottom:24px;font-size:15px;font-weight:bold}'
].join('');

function buildConfigPage(cfg) {
  var data = {
    apiKey: cfg.apiKey || '',
    units: cfg.units || 'metric',
    tour: cfg.tour || 'atp'
  };

  return [
    '<!DOCTYPE html><html><head><meta charset="utf-8">',
    '<meta name="viewport" content="width=device-width,initial-scale=1">',
    '<title>Grand Slam</title><style>', STYLE, '</style></head><body>',
    '<h1>🎾 Grand Slam</h1>',
    '<div class="sub">Heure & météo des 4 tournois du Grand Chelem. Votre clé reste sur le téléphone.</div>',
    '<label>Clé API OpenWeatherMap</label>',
    '<input id="apiKey" type="password" placeholder="collez votre clé">',
    '<div class="hint">Gratuite sur openweathermap.org → API keys.</div>',
    '<label>Unité de température</label>',
    '<select id="units"><option value="metric">Celsius (°C)</option><option value="imperial">Fahrenheit (°F)</option></select>',
    '<label>Vainqueur affiché (dernière édition)</label>',
    '<select id="tour"><option value="atp">ATP (messieurs)</option><option value="wta">WTA (dames)</option></select>',
    '<button id="save">Enregistrer</button>',
    '<a href="https://paypal.me/yoadadev" target="_blank" rel="noopener" class="support">☕ Soutenir le développeur</a>',
    '<script>var D=', JSON.stringify(data), ';</script>',
    '<script>', PAGE_JS, '</script>',
    '</body></html>'
  ].join('');
}

module.exports = buildConfigPage;
