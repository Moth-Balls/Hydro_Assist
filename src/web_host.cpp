#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// WiFi Credentials
const char* ssid = "isaac-priv-net";
const char* password = "goodlife";

float phValue = 7.2;
float ecValue = 1250.0;

AsyncWebServer server(80);

// HTML Website Block
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hydro-Assist Monitor</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        * { 
            margin: 0; padding: 0; box-sizing: border-box; 
            /* NEW IMPROVED LEAF CURSOR */
            cursor: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="%234caf50" d="M17,8C15.5,1 7,1 3,5C2.1,10 5,16 11,20V23H13V20C19,16 21.9,10 21,5C17,1 8.5,1 7,8Z"/></svg>') 12 12, auto !important;
        }
        body { font-family: 'Segoe UI', sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 20px; color: #333; }
        .container { max-width: 1200px; margin: 0 auto; }
        header { text-align: center; color: white; margin-bottom: 30px; }
        .status { display: inline-block; padding: 8px 16px; border-radius: 20px; font-weight: bold; margin-top: 10px; background: #f44336; color: white; }
        .status.connected { background-color: #4caf50; }
        .dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 30px; }
        .sensor-card { background: white; border-radius: 15px; padding: 30px; box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2); text-align: center; }
        .sensor-value { font-size: 3.5em; font-weight: bold; color: #667eea; margin: 15px 0; }
        .controls { background: white; border-radius: 15px; padding: 20px; margin-bottom: 30px; display: flex; gap: 10px; align-items: center; }
        select, button { padding: 10px; border-radius: 5px; border: 1px solid #ddd; font-weight: bold; cursor: inherit; }
        .charts-section { background: white; border-radius: 15px; padding: 20px; }
        .chart-container { position: relative; height: 250px; }
    </style>
</head>
<body>
    <div class="container">
        <header><h1>pH/EC Sensor Monitor</h1><div id="stat" class="status">Disconnected</div></header>
        <div class="controls">
            <select id="pSel"><option value="">Select a plant...</option></select>
            <div id="pInfo" style="margin-left: 20px; font-weight: bold; color: #667eea;"></div>
        </div>
        <div class="dashboard">
            <div class="sensor-card"><div>💧 pH Level</div><div class="sensor-value" id="phV">--</div><div id="phT" style="color:#999">Target: --</div></div>
            <div class="sensor-card"><div>⚡ EC Level</div><div class="sensor-value" id="ecV">--</div><div id="ecT" style="color:#999">Target: --</div></div>
        </div>
        <div class="charts-section">
            <h2>Historical Data (15 Min Window)</h2>
            <div class="chart-container"><canvas id="phC"></canvas></div>
            <div class="chart-container"><canvas id="ecC"></canvas></div>
        </div>
    </div>
    <script>
        let sData = []; let phChart, ecChart; let selP = null;
        const MAX_POINTS = 90; 
        const pData = [{n:"Arugula", pha:6.5, phr:"6.0-7.0", ecr:"0.8-1.2"}, {n:"Lettuce", pha:6.5, phr:"6.0-7.0", ecr:"0.8-1.2"}];
        
        function init() {
            const ctxP = document.getElementById('phC').getContext('2d'); const ctxE = document.getElementById('ecC').getContext('2d');
            const cfg = { type:'line', data:{ labels:[], datasets:[{ data:[], tension:0.4, fill:true }] }, options:{ responsive:true, maintainAspectRatio:false, animation: false } };
            phChart = new Chart(ctxP, JSON.parse(JSON.stringify(cfg))); phChart.data.datasets[0].label = 'pH'; phChart.data.datasets[0].borderColor = '#667eea';
            ecChart = new Chart(ctxE, JSON.parse(JSON.stringify(cfg))); ecChart.data.datasets[0].label = 'EC'; ecChart.data.datasets[0].borderColor = '#764ba2';
            const s = document.getElementById('pSel'); pData.forEach((p,i)=>{ let o=document.createElement('option'); o.value=i; o.innerText=p.n; s.appendChild(o); });
            s.onchange = (e) => { selP = pData[e.target.value] || null; updateUI(); };
        }
        function updateUI() { if(selP) { document.getElementById('pInfo').innerText = "Growing: " + selP.n; document.getElementById('phT').innerText = "Target: " + selP.phr; document.getElementById('ecT').innerText = "Target: " + selP.ecr; } }
        async function fetchD() {
            try {
                const r = await fetch('/data'); const d = await r.json();
                document.getElementById('phV').innerText = d.ph.toFixed(2); document.getElementById('ecV').innerText = d.ec.toFixed(0);
                document.getElementById('stat').innerText = "Connected"; document.getElementById('stat').classList.add('connected');
                const t = new Date().toLocaleTimeString(); 
                sData.push({t, ph:d.ph, ec:d.ec}); 
                if(sData.length > MAX_POINTS) sData.shift(); 
                phChart.data.labels = sData.map(x=>x.t); phChart.data.datasets[0].data = sData.map(x=>x.ph); phChart.update('none');
                ecChart.data.labels = sData.map(x=>x.t); ecChart.data.datasets[0].data = sData.map(x=>x.ec); ecChart.update('none');
            } catch(e) { document.getElementById('stat').innerText = "Disconnected"; document.getElementById('stat').classList.remove('connected'); }
        }
        window.onload = () => { init(); setInterval(fetchD, 10000); };
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("An error occurred while mounting LittleFS");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.print("\nConnected! IP: "); 
  Serial.println(WiFi.localIP());

  // Serve the HTML file from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/website.html", "text/html");
  });

  // Serve the JSON data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"ph\":" + String(phValue) + ",\"ec\":" + String(ecValue) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.begin();
}

void loop() {
  // Simulating sensor changes every 10 seconds
  phValue = 6.5 + ((float)random(-10, 10) / 100.0);
  ecValue = 1100 + random(-20, 20);
  delay(10000); 
}