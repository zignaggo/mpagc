#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <UrlEncode.h>
#include <ESP8266HTTPClient.h>

const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASSWORD";

const int PIN_GAS_SENSOR = A0;
const int PIN_BUZZER = 14; 
const int PIN_SDA = 4;
const int PIN_SCL = 5;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = -1;


String whatsappPhone = "";
String whatsappApiKey = "";

ESP8266WebServer server(80);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool gasLeakDetected = false;
int gasThreshold = 400;
bool debugMode = true;

struct GasSensorResult {
  int value;
  bool leakDetected;
};

void debugLog(String message) {
  if (debugMode) {
    Serial.print("[DEBUG] ");
    Serial.println(message);
  }
}

void debugLog(String prefix, String message) {
  if (debugMode) {
    Serial.print("[DEBUG] ");
    Serial.print(prefix);
    Serial.print(" ");
    Serial.println(message);
  }
}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="pt-BR"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Monitor MPAGC</title><style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:Arial,sans-serif;background:#0a0a0a;color:#fafafa;padding:1rem}.container{max-width:800px;margin:0 auto}.header{text-align:center;margin-bottom:2rem}.card{background:#18181b;border:1px solid #27272a;border-radius:.5rem;padding:1.5rem;margin-bottom:1.5rem}.card-title{font-size:1.25rem;font-weight:600;margin-bottom:1rem}.status-item{background:#09090b;border:1px solid #27272a;border-radius:.5rem;padding:1rem;margin-bottom:1rem}.status-label{font-size:.875rem;color:#a1a1aa;margin-bottom:.5rem}.status-value{font-size:1.5rem;font-weight:700}.badge{padding:.25rem .75rem;border-radius:999px;font-size:.75rem;font-weight:600}.badge-success{background:#166534;color:#bbf7d0}.badge-danger{background:#991b1b;color:#fecaca}.form-group{margin-bottom:1rem}.form-label{display:block;font-size:.875rem;margin-bottom:.5rem}.form-input{width:100%;padding:.5rem;background:#09090b;border:1px solid #27272a;border-radius:.5rem;color:#fafafa;font-size:.875rem}.form-input:focus{border-color:#3b82f6;outline:0}.btn{padding:.625rem 1.25rem;font-size:.875rem;font-weight:500;border:none;border-radius:.5rem;cursor:pointer;width:100%;margin-top:.5rem}.btn-primary{background:#3b82f6;color:#fff}.btn-primary:hover{background:#2563eb}.alert{padding:1rem;border-radius:.5rem;font-size:.875rem;margin-bottom:1rem;display:none}.alert.show{display:block}.alert-success{background:#166534;color:#bbf7d0}.alert-error{background:#991b1b;color:#fecaca}</style></head><body><div class="container"><div class="header"><h1>Monitor de Gás</h1><p>MPAGC</p></div><div class="card"><h2 class="card-title">Status</h2><div class="status-item"><div class="status-label">Vazamento</div><div class="status-value"><span id="leak">--</span></div><small>ADC: <span id="gasVal">--</span> | Limiar: <span id="gasTh">--</span></small></div></div><div class="card"><h2 class="card-title">Configurações</h2><div class="alert alert-success" id="sOk"></div><div class="alert alert-error" id="sErr"></div><form id="form"><div class="form-group"><label class="form-label">Limiar Gás (0-1023)</label><input type="number" class="form-input" id="th" min="0" max="1023"></div><div class="form-group"><label class="form-label">WhatsApp</label><input type="text" class="form-input" id="ph" placeholder="5511999999999"></div><div class="form-group"><label class="form-label">API Key</label><input type="text" class="form-input" id="key"></div><button type="submit" class="btn btn-primary">Salvar</button></form></div></div><script>let t;const $=e=>document.getElementById(e),show=(e,s)=>{$(e).textContent=s,$(e).classList.add('show'),setTimeout(()=>$(e).classList.remove('show'),5e3)};async function upd(){try{const e=await(await fetch('/api/status')).json();$('leak').innerHTML=e.gasLeak?'<span class="badge badge-danger">ALERTA</span>':'<span class="badge badge-success">Normal</span>';$('gasVal').textContent=e.gasValue??'--';$('gasTh').textContent=e.gasThreshold??'--'}catch(e){console.error(e)}}async function loadSet(){try{const e=await(await fetch('/api/settings')).json();$('th').value=e.gasThreshold||'';$('ph').value=e.whatsappPhone||'';$('key').value=e.whatsappApiKey||''}catch(e){console.error(e)}}$('form').onsubmit=async e=>{e.preventDefault();try{const s={gasThreshold:parseInt($('th').value)||400,whatsappPhone:$('ph').value.trim(),whatsappApiKey:$('key').value.trim()},a=await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(s)});a.ok?(show('sOk','Salvo!'),upd()):show('sErr','Erro')}catch(e){show('sErr','Erro')}};upd();loadSet();t=setInterval(upd,3e3);document.addEventListener('visibilitychange',()=>{document.hidden?(clearInterval(t),t=null):(upd(),t=setInterval(upd,3e3))});</script></body></html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(10);
  
  Serial.println("\n=== MPAGC - Iniciando Sistema ===");
  
  pinMode(PIN_GAS_SENSOR, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  setupWiFi();
  setupServer();
  setupDisplay();

  Serial.println("Servidor HTTP iniciado na porta 80");
  debugLog("Limiar de detecção de gás: " + String(gasThreshold));
  debugLog("Debug mode: " + String(debugMode ? "ATIVADO" : "DESATIVADO"));

  Serial.println("=== Sistema pronto ===\n");
}

void loop() {
  server.handleClient();
  
  readSensors();
  
  int gasValue = analogRead(PIN_GAS_SENSOR);

  updateDisplay(gasValue, gasLeakDetected);
  
  delay(500);
}


/*  WHATSAPP API */
void sendWhatsappMessage(String message, String phone, String apikey) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado. Mensagem não enviada.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Ignora validação SSL (necessário para ESP8266)
  
  HTTPClient http;

  String encodedMessage = urlEncode(message);
  String url = String("https://api.callmebot.com/whatsapp.php?phone=") + phone + "&text=" + encodedMessage + "&apikey=" + apikey;

  http.begin(client, url);
  http.setTimeout(15000); // Timeout de 15 segundos
  
  int httpCode = http.GET();

  switch (httpCode / 100) {
    case 2:
    case 3:
      Serial.printf("Mensagem enviada! Código HTTP: %d\n", httpCode);
      break;
    default:
      Serial.printf("Erro ao enviar mensagem: %d\n", httpCode);
      if (httpCode == -1) {
        Serial.println("   Dica: Verifique a conexão WiFi e o endpoint HTTPS");
      }
      break;
  }
  
  http.end();

}

/* SETUPS */
void setupServer() {
  debugLog("Configurando rotas do servidor...");

  server.on("/", HTTP_GET, handleRoot);
  
  server.on("/api/status", HTTP_GET, handleGetStatus);
  
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  
  server.on("/api/settings", HTTP_POST, handlePostSettings);
  
  server.onNotFound(handleNotFound);

  server.begin();
}

void setupWiFi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
    if (wifiAttempts > 40) {
      Serial.println("\nERRO: Timeout na conexão WiFi!");
      break;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
    debugLog("RSSI: " + String(WiFi.RSSI()) + " dBm");
  }
  
}

void setupDisplay() {
  Wire.begin(PIN_SDA, PIN_SCL);
  
  debugLog("Inicializando display OLED...");
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    debugLog("ERRO: Display OLED não detectado!");
  } else {
    debugLog("Display OLED inicializado (0x3C)!");
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20, 10);
  display.println("MPAGC");
  display.setTextSize(1);
  display.setCursor(5, 35);
  display.println("Sistema Iniciado");
  display.display();
  delay(2000);
}

/* HTTP HANDLERS */
void handleRoot() {
  debugLog("[HTTP] GET / - Servindo página principal");
  server.send_P(200, "text/html", index_html);
}

void handleGetStatus() {
  debugLog("[API] GET /api/status");
  
  int gasValue = analogRead(PIN_GAS_SENSOR);
  
  StaticJsonDocument<200> doc;
  
  doc["gasLeak"] = gasLeakDetected;
  doc["gasValue"] = gasValue;
  doc["gasThreshold"] = gasThreshold;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  debugLog("  -> Vazamento: " + String(gasLeakDetected ? "SIM" : "NÃO"));
  
  server.send(200, "application/json", response);
}

void handleGetSettings() {
  debugLog("[API] GET /api/settings");
  debugLog("  -> Limiar gás: " + String(gasThreshold));
  
  StaticJsonDocument<300> doc;
  
  doc["gasThreshold"] = gasThreshold;
  doc["whatsappPhone"] = whatsappPhone;
  doc["whatsappApiKey"] = whatsappApiKey;
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
}

void handlePostSettings() {
  debugLog("[API] POST /api/settings");
  
  if (server.hasArg("plain") == false) {
    debugLog("  -> ERRO: Body não encontrado");
    server.send(400, "application/json", "{\"error\":\"Body não encontrado\"}");
    return;
  }
  
  String body = server.arg("plain");
  debugLog("  -> Body recebido: " + body);
  
  StaticJsonDocument<400> doc;
  
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    debugLog("  -> ERRO: JSON inválido - " + String(error.c_str()));
    server.send(400, "application/json", "{\"error\":\"JSON inválido\"}");
    return;
  }
  
  if (doc.containsKey("gasThreshold")) {
    int oldThreshold = gasThreshold;
    gasThreshold = doc["gasThreshold"];
    debugLog("  -> Limiar gás atualizado: " + String(oldThreshold) + " -> " + String(gasThreshold));
  }
  
  if (doc.containsKey("whatsappPhone")) {
    String oldPhone = whatsappPhone;
    whatsappPhone = doc["whatsappPhone"].as<String>();
    debugLog("  -> WhatsApp Phone atualizado: " + oldPhone + " -> " + whatsappPhone);
  }
  
  if (doc.containsKey("whatsappApiKey")) {
    whatsappApiKey = doc["whatsappApiKey"].as<String>();
    debugLog("  -> WhatsApp API Key atualizada");
  }
  
  debugLog("  -> Configurações salvas com sucesso");
  server.send(200, "application/json", "{\"success\":true}");
}

void handleNotFound() {
  debugLog("[HTTP] 404 - Rota não encontrada: " + server.uri());
  server.send(404, "text/plain", "404: Não encontrado");
}

/* SENSORS */
void readSensors() {
  bool previousLeakState = gasLeakDetected;
  GasSensorResult gas = readGasSensor();

  if (!previousLeakState && gasLeakDetected) {
    // Envia mensagem apenas se o telefone e API key estiverem configurados
    if (whatsappPhone.length() > 0 && whatsappApiKey.length() > 0) {
      String msg = "ALERTA: Vazamento de gas detectado! "
                  "ADC=" + String(gas.value) +
                  " / limiar=" + String(gasThreshold);
      sendWhatsappMessage(msg, whatsappPhone, whatsappApiKey);
    } else {
      debugLog("[ALERTA] Vazamento detectado mas WhatsApp não configurado!");
    }
  }
  
  debugLog("[SENSORES] Gás ADC: " + String(gas.value) + "/1023 | Limiar: " + String(gasThreshold) + " | Estado: " + String(gasLeakDetected ? "VAZAMENTO!" : "Normal"));
  
  if(gasLeakDetected) {
    buzz();
  } else {
    unbuzz();
  }
}

GasSensorResult readGasSensor() {
  GasSensorResult res;
  res.value = analogRead(PIN_GAS_SENSOR);
  res.leakDetected = (res.value > gasThreshold);
  gasLeakDetected = res.leakDetected;
  return res;
}


void updateDisplay(int gas_value, bool alert_status) {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Status
  display.setTextSize(2);
  if (alert_status) {
    display.setCursor(10, 2);
    display.setTextColor(BLACK, WHITE); // Invertido
    display.println(" ALERTA ");
    display.setTextColor(WHITE);
    // Borda dupla
    display.drawRect(0, 0, 128, 20, WHITE);
    display.drawRect(1, 1, 126, 18, WHITE);
  } else {
    display.setCursor(20, 2);
    display.println("NORMAL");
  }

  // Linha divisória
  display.drawFastHLine(0, 22, SCREEN_WIDTH, WHITE);

  // GLP: valor do sensor 
  display.setTextSize(1);
  display.setCursor(0, 26);
  display.print("GLP ADC: ");
  display.print(gas_value);

  display.display();
}

void buzz() {
  tone(PIN_BUZZER, 2000);  // 2000 Hz
}

void unbuzz() {
  noTone(PIN_BUZZER);
}