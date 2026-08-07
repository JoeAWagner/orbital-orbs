#include "WebPortal.h"
#include "Settings.h"
#include "version.h"
#include <Update.h>
#include <WiFi.h>

// ---- Control panel page (single self-contained HTML document) -------------
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Info Orbs Control</title>
<style>
  :root { color-scheme: dark; }
  * { box-sizing: border-box; }
  body { margin:0; font-family: system-ui, -apple-system, Segoe UI, Roboto, sans-serif;
         background:#0d1117; color:#e6edf3; }
  header { padding:20px; text-align:center; background:#161b22; border-bottom:1px solid #30363d; }
  header h1 { margin:0; font-size:1.3rem; }
  header .sub { color:#8b949e; font-size:.85rem; margin-top:4px; }
  main { max-width:520px; margin:0 auto; padding:16px; }
  .card { background:#161b22; border:1px solid #30363d; border-radius:12px;
          padding:16px; margin-bottom:16px; }
  .card h2 { margin:0 0 12px; font-size:1rem; color:#58a6ff; }
  .row { display:flex; justify-content:space-between; padding:4px 0; font-size:.9rem; }
  .row span:first-child { color:#8b949e; }
  .btns { display:flex; gap:10px; }
  button { flex:1; padding:12px; border:0; border-radius:10px; background:#238636;
           color:#fff; font-size:1rem; cursor:pointer; }
  button:active { transform:scale(.98); }
  button.secondary { background:#30363d; }
  input[type=range] { width:100%; }
  input[type=number] { width:90px; padding:8px; border-radius:8px; border:1px solid #30363d;
                       background:#0d1117; color:#e6edf3; }
  input[type=text] { width:100%; padding:10px; border-radius:8px; border:1px solid #30363d;
                     background:#0d1117; color:#e6edf3; }
  label { display:block; margin-bottom:8px; color:#8b949e; font-size:.85rem; }
  .save { margin-top:10px; }
  .val { color:#e6edf3; font-weight:600; }
</style>
</head>
<body>
<header>
  <h1>&#x1F52E; Info Orbs</h1>
  <div class="sub" id="ip">connecting...</div>
</header>
<main>
  <div class="card">
    <h2>Status</h2>
    <div class="row"><span>Current widget</span><span class="val" id="widget">-</span></div>
    <div class="row"><span>Brightness</span><span class="val" id="bright">-</span></div>
    <div class="row"><span>Auto-rotate</span><span class="val" id="cycle">-</span></div>
    <div class="row"><span>Uptime</span><span class="val" id="uptime">-</span></div>
    <div class="row"><span>Free memory</span><span class="val" id="heap">-</span></div>
    <div class="row"><span>Firmware</span><span class="val" id="fw">-</span></div>
  </div>

  <div class="card">
    <h2>Widget</h2>
    <div class="btns">
      <button class="secondary" onclick="post('/api/prev')">&#x25C0; Prev</button>
      <button class="secondary" onclick="post('/api/next')">Next &#x25B6;</button>
    </div>
  </div>

  <div class="card">
    <h2>Brightness</h2>
    <input type="range" min="10" max="255" id="brightSlider"
           oninput="setBright(this.value)">
  </div>

  <div class="card">
    <h2>Auto-rotate widgets</h2>
    <label>Seconds between widgets (0 = off, use the buttons)</label>
    <input type="number" min="0" max="3600" id="cycleInput">
    <button class="save" onclick="saveCycle()">Save</button>
  </div>

  <div class="card">
    <h2>Settings</h2>
    <label>Weather location (city, state / city, country)</label>
    <input type="text" id="sWeather" placeholder="Shoreham, NY">
    <label style="margin-top:12px"><input type="checkbox" id="sMetric"> Use metric units (&deg;C)</label>
    <label style="margin-top:12px">Stock / crypto tickers (comma separated, up to 5)</label>
    <input type="text" id="sStocks" placeholder="BTC/USD,AAPL,TSLA,MSFT,GOOG">
    <label style="margin-top:12px">Timezone (e.g. America/New_York)</label>
    <input type="text" id="sTz" placeholder="America/New_York">
    <label style="margin-top:12px"><input type="checkbox" id="sFmt24"> 24-hour clock</label>
    <button class="save" onclick="saveSettings()">Save &amp; apply</button>
    <div id="saveMsg" style="margin-top:8px;color:#3fb950;font-size:.85rem;"></div>
  </div>

  <div class="card">
    <h2>Custom widget</h2>
    <label>Title</label>
    <input type="text" id="cTitle" placeholder="Custom">
    <label style="margin-top:12px">Static text (leave blank if using a URL below)</label>
    <input type="text" id="cText" placeholder="Hello from the orbs!">
    <label style="margin-top:12px">Or fetch from URL (overrides text)</label>
    <input type="text" id="cUrl" placeholder="https://api.example.com/value">
    <label style="margin-top:12px">JSON field (dot path, optional - e.g. data.amount)</label>
    <input type="text" id="cField" placeholder="leave blank to show raw response">
    <button class="save" onclick="saveCustom()">Save &amp; apply</button>
    <div id="customMsg" style="margin-top:8px;color:#3fb950;font-size:.85rem;"></div>
  </div>

  <div class="card">
    <h2>Firmware update (OTA)</h2>
    <label>Upload a new firmware .bin to flash over WiFi (no USB needed)</label>
    <input type="file" id="fwFile" accept=".bin">
    <button class="save" onclick="uploadFw()">Upload &amp; flash</button>
    <div id="fwMsg" style="margin-top:8px;font-size:.85rem;color:#8b949e;"></div>
  </div>
</main>
<script>
function fmtUptime(s){var h=Math.floor(s/3600),m=Math.floor((s%3600)/60),x=s%60;
  return (h?h+'h ':'')+(m?m+'m ':'')+x+'s';}
function refresh(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('ip').textContent = d.ip;
    document.getElementById('widget').textContent = d.widget;
    document.getElementById('bright').textContent = d.brightness;
    document.getElementById('cycle').textContent = d.cycle>0 ? d.cycle+'s' : 'off';
    document.getElementById('uptime').textContent = fmtUptime(d.uptime);
    document.getElementById('heap').textContent = Math.round(d.heap/1024)+' KB';
    document.getElementById('fw').textContent = d.fw;
    var bs=document.getElementById('brightSlider');
    if(document.activeElement!==bs) bs.value = d.brightness;
    var ci=document.getElementById('cycleInput');
    if(document.activeElement!==ci) ci.value = d.cycle;
  }).catch(()=>{});
}
function post(url){return fetch(url,{method:'POST'}).then(refresh);}
var bt;
function setBright(v){clearTimeout(bt);bt=setTimeout(function(){
  fetch('/api/brightness?value='+v,{method:'POST'}).then(refresh);},120);}
function saveCycle(){var v=document.getElementById('cycleInput').value;
  fetch('/api/cycle?value='+v,{method:'POST'}).then(refresh);}
function loadSettings(){
  fetch('/api/settings').then(r=>r.json()).then(d=>{
    document.getElementById('sWeather').value = d.weatherLocation;
    document.getElementById('sMetric').checked = d.metric;
    document.getElementById('sStocks').value = d.stocks;
    document.getElementById('sTz').value = d.timezone;
    document.getElementById('sFmt24').checked = d.format24;
    document.getElementById('cTitle').value = d.customTitle;
    document.getElementById('cText').value = d.customText;
    document.getElementById('cUrl').value = d.customUrl;
    document.getElementById('cField').value = d.customField;
  }).catch(()=>{});
}
function saveCustom(){
  var body = new URLSearchParams();
  body.set('customTitle', document.getElementById('cTitle').value);
  body.set('customText', document.getElementById('cText').value);
  body.set('customUrl', document.getElementById('cUrl').value);
  body.set('customField', document.getElementById('cField').value);
  var msg = document.getElementById('customMsg');
  msg.textContent = 'Saving & refreshing...';
  fetch('/api/settings',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:body.toString()})
    .then(r=>r.json()).then(()=>{ msg.textContent='Saved. Switch to the "Custom" widget to see it.';
      setTimeout(function(){msg.textContent='';}, 5000); })
    .catch(()=>{ msg.textContent='Save failed - try again.'; });
}
function saveSettings(){
  var body = new URLSearchParams();
  body.set('weatherLocation', document.getElementById('sWeather').value);
  body.set('metric', document.getElementById('sMetric').checked);
  body.set('stocks', document.getElementById('sStocks').value);
  body.set('timezone', document.getElementById('sTz').value);
  body.set('format24', document.getElementById('sFmt24').checked);
  var msg = document.getElementById('saveMsg');
  msg.textContent = 'Saving & refreshing the orbs...';
  fetch('/api/settings',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:body.toString()})
    .then(r=>r.json()).then(()=>{ msg.textContent='Saved. The orbs are updating.';
      setTimeout(function(){msg.textContent='';}, 4000); })
    .catch(()=>{ msg.textContent='Save failed - try again.'; });
}
function uploadFw(){
  var f=document.getElementById('fwFile').files[0];
  var msg=document.getElementById('fwMsg');
  if(!f){ msg.textContent='Pick a firmware .bin first.'; return; }
  var fd=new FormData(); fd.append('firmware', f, f.name);
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/update');
  xhr.upload.onprogress=function(e){ if(e.lengthComputable){
    msg.textContent='Uploading '+Math.round(e.loaded/e.total*100)+'% ...'; }};
  xhr.onload=function(){ msg.textContent = (xhr.responseText.indexOf('OK')>=0)
    ? 'Success! The orbs are rebooting with the new firmware. Reload this page in ~15s.'
    : 'Update failed - check the file and try again.'; };
  xhr.onerror=function(){ msg.textContent='Connection lost (the board is likely rebooting). Reload in ~15s.'; };
  msg.textContent='Starting upload...';
  xhr.send(fd);
}
loadSettings(); refresh();
setInterval(function(){ if(!document.hidden) refresh(); }, 5000);
</script>
</body>
</html>
)HTML";

WebPortal::WebPortal(ScreenManager *sm, WidgetSet *widgetSet)
    : m_server(80), m_sm(sm), m_widgetSet(widgetSet) {}

void WebPortal::begin() {
    if (m_started) {
        return;
    }
    m_server.on("/", HTTP_GET, [this]() { handleRoot(); });
    m_server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    m_server.on("/api/next", HTTP_POST, [this]() { handleNext(); });
    m_server.on("/api/prev", HTTP_POST, [this]() { handlePrev(); });
    m_server.on("/api/brightness", HTTP_POST, [this]() { handleBrightness(); });
    m_server.on("/api/cycle", HTTP_POST, [this]() { handleCycle(); });
    m_server.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
    m_server.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });

    // Over-the-air firmware update: browser POSTs a .bin to /update
    m_server.on(
        "/update", HTTP_POST,
        [this]() {
            m_server.sendHeader("Connection", "close");
            bool ok = !Update.hasError();
            m_server.send(200, "text/plain", ok ? "OK" : "FAIL");
            delay(500);
            ESP.restart();
        },
        [this]() {
            HTTPUpload &upload = m_server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("OTA update starting: %s\n", upload.filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (Update.end(true)) {
                    Serial.printf("OTA update success: %u bytes. Rebooting.\n", upload.totalSize);
                } else {
                    Update.printError(Serial);
                }
            }
        });

    m_server.begin();
    m_started = true;
    Serial.print("Web control panel started at http://");
    Serial.println(WiFi.localIP());
}

void WebPortal::handle() {
    if (m_started) {
        m_server.handleClient();
    }
}

void WebPortal::notifyChanged() {
    if (m_onChanged) {
        m_onChanged();
    }
}

void WebPortal::handleRoot() {
    m_server.send_P(200, "text/html", INDEX_HTML);
}

void WebPortal::handleStatus() {
    String widgetName = "-";
    if (m_widgetSet != nullptr && m_widgetSet->getCurrent() != nullptr) {
        widgetName = m_widgetSet->getCurrent()->getName();
    }
    String json = "{";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"widget\":\"" + widgetName + "\",";
    json += "\"brightness\":" + String(m_sm->getBrightness()) + ",";
    json += "\"cycle\":" + String(settings.getCycleDelaySeconds()) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"fw\":\"" FW_VERSION "\"";
    json += "}";
    m_server.send(200, "application/json", json);
}

void WebPortal::handleNext() {
    if (m_widgetSet != nullptr) {
        m_widgetSet->next();
    }
    handleStatus();
}

void WebPortal::handlePrev() {
    if (m_widgetSet != nullptr) {
        m_widgetSet->prev();
    }
    handleStatus();
}

void WebPortal::handleBrightness() {
    if (m_server.hasArg("value")) {
        int v = m_server.arg("value").toInt();
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        settings.setBrightness((uint8_t) v);
        // Light path: main loop applies brightness and does a single redraw.
        // No data re-fetch, and the redraw happens on the render thread - not here.
        if (m_onLiveApply) {
            m_onLiveApply();
        }
    }
    handleStatus();
}

void WebPortal::handleCycle() {
    if (m_server.hasArg("value")) {
        int v = m_server.arg("value").toInt();
        settings.setCycleDelaySeconds(v);
        if (m_onLiveApply) {
            m_onLiveApply();
        }
    }
    handleStatus();
}

// Escape a string for safe embedding inside a JSON double-quoted value
static String jsonEscape(const String &in) {
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            // skip
        } else {
            out += c;
        }
    }
    return out;
}

void WebPortal::handleGetSettings() {
    String json = "{";
    json += "\"weatherLocation\":\"" + jsonEscape(settings.getWeatherLocation()) + "\",";
    json += "\"metric\":" + String(settings.getMetric() ? "true" : "false") + ",";
    json += "\"stocks\":\"" + jsonEscape(settings.getStockList()) + "\",";
    json += "\"timezone\":\"" + jsonEscape(settings.getTimezoneLocation()) + "\",";
    json += "\"format24\":" + String(settings.getFormat24Hour() ? "true" : "false") + ",";
    json += "\"customTitle\":\"" + jsonEscape(settings.getCustomTitle()) + "\",";
    json += "\"customText\":\"" + jsonEscape(settings.getCustomText()) + "\",";
    json += "\"customUrl\":\"" + jsonEscape(settings.getCustomUrl()) + "\",";
    json += "\"customField\":\"" + jsonEscape(settings.getCustomField()) + "\"";
    json += "}";
    m_server.send(200, "application/json", json);
}

void WebPortal::handleSaveSettings() {
    if (m_server.hasArg("weatherLocation")) {
        settings.setWeatherLocation(m_server.arg("weatherLocation"));
    }
    if (m_server.hasArg("stocks")) {
        settings.setStockList(m_server.arg("stocks"));
    }
    if (m_server.hasArg("timezone")) {
        settings.setTimezoneLocation(m_server.arg("timezone"));
    }
    if (m_server.hasArg("metric")) {
        settings.setMetric(m_server.arg("metric") == "true");
    }
    if (m_server.hasArg("format24")) {
        settings.setFormat24Hour(m_server.arg("format24") == "true");
    }
    if (m_server.hasArg("customTitle")) {
        settings.setCustomTitle(m_server.arg("customTitle"));
    }
    if (m_server.hasArg("customText")) {
        settings.setCustomText(m_server.arg("customText"));
    }
    if (m_server.hasArg("customUrl")) {
        settings.setCustomUrl(m_server.arg("customUrl"));
    }
    if (m_server.hasArg("customField")) {
        settings.setCustomField(m_server.arg("customField"));
    }
    notifyChanged();
    m_server.send(200, "application/json", "{\"ok\":true}");
}
