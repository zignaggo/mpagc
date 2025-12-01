#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <UrlEncode.h>

const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASSWORD";

const int GAS_SENSOR_PIN = A0;
const int SCALE_DT_PIN = 14;
const int SCALE_SCK_PIN = 12;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = -1;
const int PIN_BUZZER = 16;

const int PIN_SDA = 4;
const int PIN_SCL = 5;

const String WHATSAPP_PHONE = "5511999999999";
const String WHATSAPP_APIKEY = "1234567890";

ESP8266WebServer server(80);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

float emptyGasWeight = 14.0;
float fullGasWeight = 13.0;
float currentGasWeight = emptyGasWeight;
float fullThreshold = fullGasWeight - emptyGasWeight;

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
<!DOCTYPE html><html lang="pt-BR"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Monitor de Gás - MPAGC</title><style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background:#0a0a0a;color:#fafafa;line-height:1.6;padding:1rem;min-height:100vh}.container{max-width:800px;margin:0 auto}.header{text-align:center;margin-bottom:2rem;padding:1.5rem 0}.header h1{font-size:1.875rem;font-weight:700;margin-bottom:.5rem;letter-spacing:-.025em}.header p{color:#a1a1aa;font-size:.875rem}.card{background:#18181b;border:1px solid #27272a;border-radius:.75rem;padding:1.5rem;margin-bottom:1.5rem;box-shadow:0 1px 3px 0 rgba(0,0,0,.1)}.card-title{font-size:1.25rem;font-weight:600;margin-bottom:1.25rem;color:#fafafa}.status-grid{display:grid;gap:1rem}.status-item{background:#09090b;border:1px solid #27272a;border-radius:.5rem;padding:1.25rem;transition:border-color .2s}.status-item:hover{border-color:#3f3f46}.status-label{font-size:.875rem;color:#a1a1aa;margin-bottom:.5rem;font-weight:500;text-transform:uppercase;letter-spacing:.05em}.status-value{font-size:2rem;font-weight:700;margin-bottom:.25rem;display:flex;align-items:center;gap:.75rem}.status-description{font-size:.875rem;color:#71717a}.badge{display:inline-flex;align-items:center;padding:.25rem .75rem;border-radius:9999px;font-size:.75rem;font-weight:600;text-transform:uppercase;letter-spacing:.05em}.badge-success{background:#166534;color:#bbf7d0}.badge-danger{background:#991b1b;color:#fecaca;animation:pulse 2s infinite}@keyframes pulse{0%,100%{opacity:1}50%{opacity:.7}}.progress-container{width:100%;height:1.5rem;background:#09090b;border:1px solid #27272a;border-radius:9999px;overflow:hidden;margin-top:.75rem}.progress-bar{height:100%;background:linear-gradient(90deg,#3b82f6,#2563eb);transition:width .5s ease,background .3s ease;display:flex;align-items:center;justify-content:center;font-size:.75rem;font-weight:600}.progress-bar.low{background:linear-gradient(90deg,#dc2626,#b91c1c)}.progress-bar.medium{background:linear-gradient(90deg,#f59e0b,#d97706)}.form-group{margin-bottom:1.25rem}.form-label{display:block;font-size:.875rem;font-weight:500;margin-bottom:.5rem;color:#fafafa}.form-input{width:100%;padding:.625rem .875rem;background:#09090b;border:1px solid #27272a;border-radius:.5rem;color:#fafafa;font-size:.875rem;transition:border-color .2s,box-shadow .2s;outline:0}.form-input:focus{border-color:#3b82f6;box-shadow:0 0 0 3px rgba(59,130,246,.1)}.form-input::placeholder{color:#52525b}.form-hint{font-size:.75rem;color:#71717a;margin-top:.375rem}.btn{display:inline-flex;align-items:center;justify-content:center;padding:.625rem 1.25rem;font-size:.875rem;font-weight:500;border:none;border-radius:.5rem;cursor:pointer;transition:background .2s,transform .1s;outline:0}.btn:active{transform:scale(.98)}.btn-primary{background:#3b82f6;color:#fff}.btn-primary:hover{background:#2563eb}.btn-primary:disabled{background:#27272a;color:#52525b;cursor:not-allowed}.alert{padding:1rem;border-radius:.5rem;font-size:.875rem;margin-bottom:1rem;display:none}.alert.show{display:block}.alert-success{background:#166534;color:#bbf7d0;border:1px solid #15803d}.alert-error{background:#991b1b;color:#fecaca;border:1px solid #b91c1c}.icon{width:1.5rem;height:1.5rem;flex-shrink:0}@media (max-width:640px){body{padding:.75rem}.header h1{font-size:1.5rem}.card{padding:1rem}.status-value{font-size:1.5rem}}.loading{display:inline-block;width:1rem;height:1rem;border:2px solid #27272a;border-top-color:#3b82f6;border-radius:50%;animation:spin .6s linear infinite}@keyframes spin{to{transform:rotate(360deg)}}</style></head><body><div class="container"><div class="header"><h1>Monitor de Gás</h1><p>Sistema de Monitoramento e Controle</p></div><div class="card"><h2 class="card-title">Status do Sistema</h2><div class="status-grid"><div class="status-item"><div class="status-label">Vazamento de Gás</div><div class="status-value"><svg class="icon" id="leakIcon" fill="currentColor" viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z"/></svg> <span id="leakStatus">Carregando...</span></div><div class="status-description" id="leakDescription">Verificando sensores...</div><div class="status-description" style="margin-top:0.5rem;font-size:0.75rem;color:#a1a1aa">Sensor: <span id="gasValueDisplay">--</span>/1023 | Limiar: <span id="gasThresholdDisplay">--</span></div></div><div class="status-item"><div class="status-label">Nível de Gás</div><div class="status-value"><span id="gasLevel">--</span>%</div><div class="progress-container"><div class="progress-bar" id="gasProgress" style="width:0%"><span id="gasProgressText"></span></div></div><div class="status-description" id="gasDescription">Peso atual do botijão</div></div></div></div><div class="card"><h2 class="card-title">Modo Pesagem</h2><div class="alert alert-success" id="successAlert">Peso salvo com sucesso!</div><div class="alert alert-error" id="errorAlert">Erro ao salvar peso. Tente novamente.</div><div class="status-item"><div class="status-label">Peso Atual Lido</div><div class="status-value"><span id="currentWeightDisplay">--</span> kg</div><div class="status-description" id="weightDescription">Coloque o botijão na balança e clique em Salvar</div></div><div class="form-group" style="margin-top:1.5rem"><button type="button" class="btn btn-primary" id="saveWeightBtn" style="width:100%">Salvar Peso Atual como Peso Inicial</button><div class="form-hint" style="margin-top:0.5rem">O peso atual será salvo como referência para cálculo do nível de gás</div></div></div></div><script>const UPDATE_INTERVAL=3e2;let updateTimer;const icons={check:'<path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z"/>',warning:'<path d="M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z"/>'};const leakIcon=document.getElementById("leakIcon"),leakStatus=document.getElementById("leakStatus"),leakDescription=document.getElementById("leakDescription"),gasValueDisplay=document.getElementById("gasValueDisplay"),gasThresholdDisplay=document.getElementById("gasThresholdDisplay"),gasLevel=document.getElementById("gasLevel"),gasProgress=document.getElementById("gasProgress"),gasProgressText=document.getElementById("gasProgressText"),gasDescription=document.getElementById("gasDescription"),currentWeightDisplay=document.getElementById("currentWeightDisplay"),weightDescription=document.getElementById("weightDescription"),saveWeightBtn=document.getElementById("saveWeightBtn"),successAlert=document.getElementById("successAlert"),errorAlert=document.getElementById("errorAlert");function init(){updateStatus(),startAutoUpdate(),saveWeightBtn.addEventListener("click",handleSaveWeight)}async function updateStatus(){try{const e=await fetch("/api/status"),t=await e.json();updateLeakStatus(t.gasLeak||!1),updateGasLevel(t.gasLevel||0),currentWeightDisplay.textContent=t.currentWeight?t.currentWeight.toFixed(2):"--",weightDescription.textContent=t.currentWeight?"Peso atual lido pela balança":"Aguardando leitura da balança",gasValueDisplay.textContent=t.gasValue!==undefined?t.gasValue:"--",gasThresholdDisplay.textContent=t.gasThreshold!==undefined?t.gasThreshold:"--"}catch(e){console.error("Error updating status:",e),showOfflineStatus()}}function updateLeakStatus(e){e?(leakIcon.innerHTML=icons.warning,leakIcon.style.color="#dc2626",leakStatus.innerHTML='<span class="badge badge-danger">Detectado</span>',leakDescription.textContent="⚠️ Vazamento detectado! Verifique imediatamente.",leakDescription.style.color="#fca5a5"):(leakIcon.innerHTML=icons.check,leakIcon.style.color="#22c55e",leakStatus.innerHTML='<span class="badge badge-success">Normal</span>',leakDescription.textContent="Sistema operando normalmente",leakDescription.style.color="#71717a")}function updateGasLevel(e){e=Math.max(0,Math.min(100,e)),gasLevel.textContent=e.toFixed(1),gasProgress.style.width=e+"%",gasProgress.className="progress-bar",e<20?(gasProgress.classList.add("low"),gasDescription.textContent="⚠️ Nível crítico - substitua o botijão em breve",gasDescription.style.color="#fca5a5"):e<50?(gasProgress.classList.add("medium"),gasDescription.textContent="Nível médio - monitore o consumo",gasDescription.style.color="#fcd34d"):(gasDescription.textContent="Nível adequado de gás",gasDescription.style.color="#71717a"),gasProgressText.textContent=e>=10?e.toFixed(0)+"%":""}function showOfflineStatus(){leakStatus.textContent="Offline",leakDescription.textContent="Não foi possível conectar ao sensor",gasLevel.textContent="--",gasProgress.style.width="0%",currentWeightDisplay.textContent="--",weightDescription.textContent="Não foi possível conectar ao sensor",gasValueDisplay.textContent="--",gasThresholdDisplay.textContent="--"}async function handleSaveWeight(){saveWeightBtn.disabled=!0,saveWeightBtn.innerHTML='<span class="loading"></span> Salvando...';try{const e=await fetch("/api/weight",{method:"POST",headers:{"Content-Type":"application/json"}});if(e.ok){const t=await e.json();showAlert("success","Peso salvo com sucesso! Peso inicial: "+t.weight+" kg"),setTimeout(()=>updateStatus(),500)}else showAlert("error","Erro ao salvar peso")}catch(e){console.error("Error saving weight:",e),showAlert("error","Erro de conexão. Verifique sua rede.")}finally{saveWeightBtn.disabled=!1,saveWeightBtn.textContent="Salvar Peso Atual como Peso Inicial"}}function showAlert(e,t){hideAlerts();const a="success"===e?successAlert:errorAlert;a.textContent=t,a.classList.add("show"),setTimeout(()=>{a.classList.remove("show")},5e3)}function hideAlerts(){successAlert.classList.remove("show"),errorAlert.classList.remove("show")}function startAutoUpdate(){updateTimer=setInterval(updateStatus,UPDATE_INTERVAL)}function stopAutoUpdate(){updateTimer&&clearInterval(updateTimer)}document.addEventListener("visibilitychange",()=>{document.hidden?stopAutoUpdate():(updateStatus(),startAutoUpdate())}),init()</script></body></html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(10);
  
  Serial.println("\n=== MPAGC - Iniciando Sistema ===");
  
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(SCALE_DT_PIN, INPUT);
  pinMode(SCALE_SCK_PIN, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  

  setupWiFi();
  setupServer();
  setupDisplay();

  Serial.println("Servidor HTTP iniciado na porta 80");
  debugLog("Peso inicial configurado: " + String(emptyGasWeight) + " kg");
  debugLog("Limiar de detecção de gás: " + String(gasThreshold));
  debugLog("Debug mode: " + String(debugMode ? "ATIVADO" : "DESATIVADO"));
  Serial.println("=== Sistema pronto ===\n");
}

void loop() {
  server.handleClient();
  
  readSensors();
  
  int gasValue = analogRead(GAS_SENSOR_PIN);
  float gasLevel = calculateGasLevel();

  updateDisplay(gasValue, gasLeakDetected, gasLevel);
  
  delay(100);
}


/*  WHATSAPP API */
void sendWhatsappMessage(String message, String phone, String apikey) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado. Mensagem não enviada.");
    return;
  }

  HTTPClient http;

  String encodedMessage = urlEncode(message);
  String url = String("https://api.callmebot.com/whatsapp.php?phone=") + phone + "&text=" + encodedMessage + "&apikey=" + apikey;

  http.begin(url);
  int httpCode = http.GET();

  switch (httpCode / 100) {
    case 2:
    case 3:
      Serial.printf("Mensagem enviada! Código HTTP: %d\n", httpCode);
      break;
    default:
      Serial.printf("Erro ao enviar mensagem: %d\n", httpCode);
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
  
  server.on("/api/weight", HTTP_POST, handlePostWeight);
  
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
  
  float gasLevel = calculateGasLevel();
  int gasValue = analogRead(GAS_SENSOR_PIN);
  
  StaticJsonDocument<200> doc;
  
  doc["gasLeak"] = gasLeakDetected;
  doc["gasLevel"] = gasLevel;
  doc["currentWeight"] = currentGasWeight;
  doc["gasValue"] = gasValue;
  doc["gasThreshold"] = gasThreshold;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  debugLog("  -> Vazamento: " + String(gasLeakDetected ? "SIM" : "NÃO") + " | Nível: " + String(gasLevel) + "% | Peso: " + String(currentGasWeight) + " kg");
  
  server.send(200, "application/json", response);
}

void handleGetSettings() {
  debugLog("[API] GET /api/settings");
  debugLog("  -> Peso inicial: " + String(emptyGasWeight) + " kg | Limiar gás: " + String(gasThreshold));
  
  StaticJsonDocument<200> doc;
  
  doc["emptyGasWeight"] = emptyGasWeight;
  doc["gasThreshold"] = gasThreshold;
  
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
  
  StaticJsonDocument<200> doc;
  
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    debugLog("  -> ERRO: JSON inválido - " + String(error.c_str()));
    server.send(400, "application/json", "{\"error\":\"JSON inválido\"}");
    return;
  }
  
  if (doc.containsKey("emptyGasWeight")) {
    float oldWeight = emptyGasWeight;
    emptyGasWeight = doc["emptyGasWeight"];
    debugLog("  -> Peso inicial atualizado: " + String(oldWeight) + " kg -> " + String(emptyGasWeight) + " kg");
  }
  
  if (doc.containsKey("gasThreshold")) {
    int oldThreshold = gasThreshold;
    gasThreshold = doc["gasThreshold"];
    debugLog("  -> Limiar gás atualizado: " + String(oldThreshold) + " -> " + String(gasThreshold));
  }
  
  debugLog("  -> Configurações salvas com sucesso");
  server.send(200, "application/json", "{\"success\":true}");
}

void handlePostWeight() {
  debugLog("[API] POST /api/weight");
  debugLog("  -> Lendo peso atual...");
  
  readSensors();
  
  // float oldWeight = emptyGasWeight;
  // emptyGasWeight = currentGasWeight;
  
  // debugLog("  -> Peso lido: " + String(currentGasWeight) + " kg");
  // debugLog("  -> Peso inicial atualizado: " + String(oldWeight) + " kg -> " + String(emptyGasWeight) + " kg");
  
  // server.send(200, "application/json", "{\"success\":true,\"weight\":" + String(emptyGasWeight) + "}");
}

void handleNotFound() {
  debugLog("[HTTP] 404 - Rota não encontrada: " + server.uri());
  server.send(404, "text/plain", "404: Não encontrado");
}

/* SENSORS */
void readSensors() {
  bool previousLeakState = gasLeakDetected;
  GasSensorResult gas = readGasSensor();
  currentGasWeight = readScale();

   if (!previousLeakState && gasLeakDetected) {
    String msg = "ALERTA: Vazamento de gas detectado! "
                 "ADC=" + String(gas.value) +
                 " / limiar=" + String(gasThreshold) +
                 " / peso=" + String(currentGasWeight) + " kg";
    sendWhatsappMessage(msg, WHATSAPP_PHONE, WHATSAPP_APIKEY);
  }
  
  debugLog("[SENSORES] Gás ADC: " + String(gas.value) + "/1023 | Limiar: " + String(gasThreshold) + " | Estado: " + String(gasLeakDetected ? "VAZAMENTO!" : "Normal") + " | Peso: " + String(currentGasWeight) + " kg");
  
  if(gasLeakDetected) {
    buzz();
  } else {
    unbuzz();
  }
}

GasSensorResult readGasSensor() {
  GasSensorResult res;
  res.value = analogRead(GAS_SENSOR_PIN);
  res.leakDetected = (res.value > gasThreshold);
  gasLeakDetected = res.leakDetected;
  return res;
}

float readScale() {
  return 20;
}

float calculateGasLevel() {
  if (currentGasWeight <= 0) {
    debugLog("[CALCULO] ERRO: Peso inicial inválido (<= 0)");
    return 0;
  }
  
  float gasWeight = currentGasWeight - emptyGasWeight;
  float gasPercentage = (gasWeight * 100.0 ) / fullThreshold;
  
  return gasPercentage;
}

void updateDisplay(int gas_value, bool alert_status, float gas_level) {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Título
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Monitor Gas - MPAGC");
  display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

  // Status de Vazamento
  display.setTextSize(1);
  display.setCursor(0, 15);
  display.print("Vazamento: ");
  if (alert_status) {
    display.setTextColor(BLACK, WHITE);
    display.println("SIM!");
    display.setTextColor(WHITE);
  } else {
    display.println("NAO");
  }

  // Nível de Gás (Porcentagem)
  display.setCursor(0, 28);
  display.print("Nivel Gas: ");
  display.print(gas_level, 1);
  display.println("%");
  
  // Barra de progresso do gás
  int barWidth = (int)(gas_level * 1.28); // 128 pixels de largura
  if (barWidth > 128) barWidth = 128;
  if (barWidth < 0) barWidth = 0;
  display.drawRect(0, 40, 128, 8, WHITE);
  display.fillRect(1, 41, barWidth, 6, WHITE);

  // Valor Analógico e Limiar
  display.setCursor(0, 52);
  display.print("ADC:");
  display.print(gas_value);
  display.print(" L:");
  display.print(gasThreshold);

  // Exibe tudo
  display.display();
}

void buzz() {
  tone(PIN_BUZZER, 1000, 500);
}

void unbuzz() {
  noTone(PIN_BUZZER);
}