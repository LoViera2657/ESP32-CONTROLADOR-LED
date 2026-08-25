#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// --- Credenciais do Wi-Fi ---
// Configure as informações da sua rede local aqui
const char* ssid = "NOME_DO_SEU_WIFI"; 
const char* password = "SENHA_DO_SEU_WIFI";

WebServer server(80);
Preferences preferences;

const int ledPin = 4; // Pino GPIO da ESP32 conectado ao Gate do MOSFET

// Variáveis de controle de estado
bool isLedOn = false;
int currentBrightness = 255;  // Brilho inicial


// Interface Web Premium (HTML, CSS e JS incorporados usando PROGMEM para otimização de memória)
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Painel de Controle de Iluminação</title>
  <style>
    :root {
      --bg-dark: #0a0a0c;
      --accent-color: #00adb5;
      --accent-glow: rgba(0, 173, 181, 0.4);
      --btn-off-bg: #1e1e24;
      --text-primary: #ffffff;
      --text-secondary: #8e8e93;
      --brightness-pct: 1.0; /* Controlada dinamicamente via JS */
    }

    body {
      margin: 0;
      padding: 0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background-color: var(--bg-dark);
      background-image: radial-gradient(circle at 50% 30%, rgba(0, 173, 181, 0) 0%, #0a0a0c 70%);
      color: var(--text-primary);
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      transition: background-image 0.5s ease, background-color 0.5s ease;
      overflow-x: hidden;
      -webkit-tap-highlight-color: transparent;
    }

    /* Efeito de iluminação suave projetada no fundo ao ligar o LED */
    body.led-active {
      background-image: radial-gradient(
        circle at 50% 35%, 
        rgba(0, 173, 181, calc(0.04 + 0.18 * var(--brightness-pct))) 0%, 
        #0a0a0c 70%
      );
    }

    .container {
      text-align: center;
      width: 100%;
      max-width: 400px;
      padding: 30px 20px;
      box-sizing: border-box;
    }

    h1 {
      font-size: 26px;
      font-weight: 600;
      letter-spacing: -0.5px;
      margin: 0 0 10px 0;
      color: var(--text-primary);
      transition: text-shadow 0.5s ease;
    }

    body.led-active h1 {
      text-shadow: 0 0 15px rgba(0, 173, 181, calc(0.1 + 0.3 * var(--brightness-pct)));
    }

    .subtitle {
      font-size: 14px;
      color: var(--text-secondary);
      margin-bottom: 50px;
    }

    /* Container do Botão Redondo de Energia */
    .power-btn-wrapper {
      position: relative;
      display: inline-block;
      margin-bottom: 50px;
    }

    .power-btn {
      width: 150px;
      height: 150px;
      border-radius: 50%;
      background: var(--btn-off-bg);
      border: 1px solid rgba(255, 255, 255, 0.05);
      cursor: pointer;
      outline: none;
      display: flex;
      align-items: center;
      justify-content: center;
      box-shadow: 
        0 10px 30px rgba(0, 0, 0, 0.5),
        inset 0 1px 2px rgba(255, 255, 255, 0.05);
      transition: all 0.5s cubic-bezier(0.4, 0, 0.2, 1);
    }

    .power-btn svg {
      width: 54px;
      height: 54px;
      stroke: var(--text-secondary);
      stroke-width: 2;
      fill: none;
      transition: all 0.5s cubic-bezier(0.4, 0, 0.2, 1);
    }

    /* Estado Ativo (Ligado) com Brilho Reativo à Intensidade */
    body.led-active .power-btn {
      background: radial-gradient(circle, #00adb5 0%, #008f95 100%);
      border-color: #00adb5;
      box-shadow: 
        0 0 calc(20px + 30px * var(--brightness-pct)) rgba(0, 173, 181, calc(0.2 + 0.5 * var(--brightness-pct))),
        inset 0 1px 3px rgba(255, 255, 255, 0.4);
    }

    body.led-active .power-btn svg {
      stroke: #ffffff;
      filter: drop-shadow(0 0 calc(4px + 6px * var(--brightness-pct)) rgba(255, 255, 255, 0.8));
    }

    /* Anel de brilho externo decorativo */
    .glow-ring {
      position: absolute;
      top: -12px;
      left: -12px;
      right: -12px;
      bottom: -12px;
      border-radius: 50%;
      border: 1px solid rgba(0, 173, 181, 0);
      transform: scale(0.9);
      transition: all 0.6s cubic-bezier(0.4, 0, 0.2, 1);
      pointer-events: none;
      z-index: -1;
    }

    body.led-active .glow-ring {
      border-color: rgba(0, 173, 181, calc(0.05 + 0.15 * var(--brightness-pct)));
      transform: scale(1.1);
      box-shadow: inset 0 0 calc(10px + 15px * var(--brightness-pct)) rgba(0, 173, 181, calc(0.05 + 0.15 * var(--brightness-pct)));
    }

    /* Card do Controle Deslizante com Efeitos Suaves de Transição */
    .slider-card {
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid rgba(255, 255, 255, 0.04);
      border-radius: 24px;
      padding: 24px;
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      box-shadow: 0 15px 35px rgba(0, 0, 0, 0.3);
      opacity: 0;
      transform: translateY(20px) scale(0.95);
      pointer-events: none;
      transition: all 0.5s cubic-bezier(0.4, 0, 0.2, 1);
    }

    body.led-active .slider-card {
      opacity: 1;
      transform: translateY(0) scale(1);
      pointer-events: auto;
    }

    .slider-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;
    }

    .slider-title {
      font-size: 13px;
      text-transform: uppercase;
      letter-spacing: 1.2px;
      color: var(--text-secondary);
      font-weight: 600;
    }

    .slider-value {
      font-size: 16px;
      font-weight: 500;
      color: var(--text-secondary);
    }

    .slider-value span.val {
      font-size: 24px;
      font-weight: 700;
      color: var(--accent-color);
      transition: text-shadow 0.3s;
    }

    body.led-active .slider-value span.val {
      text-shadow: 0 0 10px rgba(0, 173, 181, calc(0.1 + 0.3 * var(--brightness-pct)));
    }

    /* Estilização Customizada do Range Input */
    .slider-container {
      position: relative;
      display: flex;
      align-items: center;
      height: 40px;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 6px;
      border-radius: 3px;
      background: rgba(255, 255, 255, 0.1);
      outline: none;
      margin: 0;
      transition: background 0.2s ease;
    }

    /* Thumb (Botão Arrastável) - WebKit (Chrome/Safari) */
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 26px;
      height: 26px;
      border-radius: 50%;
      background: #ffffff;
      cursor: pointer;
      box-shadow: 
        0 4px 10px rgba(0, 0, 0, 0.5),
        0 0 0 4px rgba(0, 173, 181, calc(0.15 + 0.25 * var(--brightness-pct)));
      transition: transform 0.15s, box-shadow 0.2s;
      margin-top: -10px;
    }

    .slider:disabled::-webkit-slider-thumb {
      background: #55555d;
      box-shadow: none;
      cursor: not-allowed;
      transform: scale(0.9);
    }

    .slider::-webkit-slider-thumb:active {
      transform: scale(1.15);
    }

    .slider::-webkit-slider-runnable-track {
      width: 100%;
      height: 6px;
      cursor: pointer;
    }

    /* Thumb - Firefox */
    .slider::-moz-range-thumb {
      width: 26px;
      height: 26px;
      border: none;
      border-radius: 50%;
      background: #ffffff;
      cursor: pointer;
      box-shadow: 
        0 4px 10px rgba(0, 0, 0, 0.5),
        0 0 0 4px rgba(0, 173, 181, calc(0.15 + 0.25 * var(--brightness-pct)));
      transition: transform 0.15s, box-shadow 0.2s;
    }

    .slider:disabled::-moz-range-thumb {
      background: #55555d;
      box-shadow: none;
      cursor: not-allowed;
      transform: scale(0.9);
    }
  </style>
</head>
<body>
  <div class="container">
    <h1 id="titleText">Iluminação</h1>
    <div id="statusSubtitle" class="subtitle">Desconectado</div>

    <!-- Botão de Liga/Desliga Circular Premium -->
    <div class="power-btn-wrapper">
      <div class="glow-ring" id="glowRing"></div>
      <button class="power-btn" id="powerBtn" onclick="toggleLed()" aria-label="Ligar ou Desligar">
        <svg viewBox="0 0 24 24">
          <path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path>
          <line x1="12" y1="2" x2="12" y2="12"></line>
        </svg>
      </button>
    </div>

    <!-- Card de Controle de Intensidade -->
    <div class="slider-card" id="sliderCard">
      <div class="slider-header">
        <span class="slider-title">Intensidade</span>
        <span class="slider-value"><span id="pwmValue" class="val">100%</span></span>
      </div>
      <div class="slider-container">
        <input type="range" min="0" max="255" value="255" class="slider" id="pwmSlider" oninput="updatePWMLocal(this.value)" onchange="sendPWMUpdate(this.value)" disabled>
      </div>
    </div>
  </div>

  <script>
    // Executa ao carregar a página
    document.addEventListener("DOMContentLoaded", () => {
      syncState();
    });

    // Sincroniza o estado atual com a ESP32 de forma assíncrona
    function syncState() {
      const subtitle = document.getElementById('statusSubtitle');
      subtitle.innerText = "Sincronizando...";
      
      fetch('/status')
        .then(response => {
          if (!response.ok) throw new Error("Erro de rede");
          return response.json();
        })
        .then(data => {
          updateUI(data.state, data.brightness, data.mode);
        })
        .catch(err => {
          console.error("Erro na sincronização:", err);
          subtitle.innerText = "Erro de conexão";
        });
    }

    // Altera o estado (Ligar/Desligar) ao clicar no botão
    function toggleLed() {
      fetch('/toggle')
        .then(response => response.json())
        .then(data => {
          updateUI(data.state, data.brightness, data.mode);
        })
        .catch(err => console.error("Erro ao chavear estado:", err));
    }

    // Atualiza a interface localmente enquanto arrasta (sem sobrecarregar a ESP32)
    function updatePWMLocal(val) {
      const percentage = Math.round((val / 255.0) * 100);
      document.getElementById('pwmValue').innerText = percentage + '%';
      updateCSSBrightness(val);
      updateSliderTrack(val);
    }

    // Envia o valor final da intensidade para o ESP32 quando o usuário solta o slider
    function sendPWMUpdate(val) {
      fetch('/slider?value=' + val)
        .then(response => response.json())
        .then(data => {
          updateUI(data.state, data.brightness, data.mode);
        })
        .catch(err => console.error("Erro ao enviar intensidade:", err));
    }

    // Atualiza todos os elementos visuais na página
    function updateUI(state, brightness, mode) {
      const body = document.body;
      const slider = document.getElementById('pwmSlider');
      const valDisplay = document.getElementById('pwmValue');
      const subtitle = document.getElementById('statusSubtitle');

      const modeText = mode === "AP" ? "Modo Direto" : "Rede Local";
      subtitle.innerText = state ? `Ligado (${modeText})` : `Desligado (${modeText})`;

      if (state) {
        body.classList.add('led-active');
        slider.disabled = false;
        slider.value = brightness;
        const percentage = Math.round((brightness / 255.0) * 100);
        valDisplay.innerText = percentage + '%';
        updateCSSBrightness(brightness);
        updateSliderTrack(brightness);
      } else {
        body.classList.remove('led-active');
        slider.disabled = true;
        updateCSSBrightness(0); // Zera o brilho no CSS
      }
    }

    // Atualiza as variáveis CSS para adaptar a intensidade dos halos e glows
    function updateCSSBrightness(brightness) {
      const pct = brightness / 255.0;
      document.documentElement.style.setProperty('--brightness-pct', pct);
    }

    // Preenche visualmente a barra do slider até o ponto do controle deslizante
    function updateSliderTrack(val) {
      const slider = document.getElementById('pwmSlider');
      const min = isNaN(parseInt(slider.min)) ? 0 : parseInt(slider.min);
      const max = parseInt(slider.max) || 255;
      const pct = ((val - min) / (max - min)) * 100;
      slider.style.background = `linear-gradient(to right, var(--accent-color) 0%, var(--accent-color) ${pct}%, rgba(255, 255, 255, 0.1) ${pct}%, rgba(255, 255, 255, 0.1) 100%)`;
    }
  </script>
</body>
</html>
)rawliteral";

// Função para enviar o JSON padronizado usando a biblioteca ArduinoJson
void sendJsonResponse(bool state, int brightness) {
  JsonDocument doc;
  doc["state"] = state;
  doc["brightness"] = brightness;
  doc["mode"] = (WiFi.status() == WL_CONNECTED) ? "STA" : "AP";

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void setup() {
  Serial.begin(115200);
  
  // Abre o namespace "led" nas preferências em modo leitura/escrita (false)
  preferences.begin("led", false);
  
  // Recupera o estado anterior gravado na memória Flash NVS. 
  // Caso não existam dados salvos, define os valores padrões indicados no segundo parâmetro.
  isLedOn = preferences.getBool("state", false);
  currentBrightness = preferences.getInt("brightness", 255);
  
  // Configuração do pino e inicialização no estado recuperado da NVS
  pinMode(ledPin, OUTPUT);
  analogWrite(ledPin, isLedOn ? currentBrightness : 0); 

  // Conexão Wi-Fi (Modo Station) com limite de tentativas (Timeout)
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int tentativas = 0;
  const int maxTentativas = 20; // 20 tentativas * 500ms = 10 segundos
  while (WiFi.status() != WL_CONNECTED && tentativas < maxTentativas) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  
  // Se conectou, imprime as informações da rede local
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado com sucesso no modo Station (STA)!");
    Serial.print("IP do Painel de Controle: ");
    Serial.println(WiFi.localIP());
  } 
  // Caso ocorra timeout, entra no modo Access Point (AP) como fallback
  else {
    Serial.println("\nFalha ao conectar no Wi-Fi configurado.");
    Serial.println("Iniciando no modo Access Point (AP) de fallback...");
    
    // SSID: ESP32-LED-Painel, sem senha
    WiFi.softAP("ESP32-LED-Painel");
    
    Serial.println("Ponto de Acesso (AP) ativo!");
    Serial.println("SSID: ESP32-LED-Painel");
    Serial.print("IP para acesso direto: ");
    Serial.println(WiFi.softAPIP()); // Geralmente 192.168.4.1
  }

  // Rota principal: Serve a página HTML
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  // Rota de status passivo (usado na inicialização da página)
  server.on("/status", HTTP_GET, []() {
    sendJsonResponse(isLedOn, currentBrightness);
  });

  // Rota do botão liga/desliga
  server.on("/toggle", HTTP_GET, []() {
    isLedOn = !isLedOn;
    
    // Liga no brilho memorizado ou desliga (0 PWM)
    analogWrite(ledPin, isLedOn ? currentBrightness : 0);
    
    // Grava na NVS apenas se o valor físico tiver mudado para preservar a memória Flash
    if (preferences.getBool("state", false) != isLedOn) {
      preferences.putBool("state", isLedOn);
    }
    
    sendJsonResponse(isLedOn, currentBrightness);
  });

  // Rota de atualização do slider
  server.on("/slider", HTTP_GET, []() {
    if (server.hasArg("value")) {
      int val = server.arg("value").toInt();
      if (val >= 0 && val <= 255) {
        currentBrightness = val;
        isLedOn = true; // Se o slider foi movido, assume-se que o LED deve acender
        analogWrite(ledPin, currentBrightness);
        
        // Grava na NVS se o valor de brilho ou estado tiverem sido alterados
        if (preferences.getInt("brightness", 255) != currentBrightness) {
          preferences.putInt("brightness", currentBrightness);
        }
        if (preferences.getBool("state", false) != isLedOn) {
          preferences.putBool("state", isLedOn);
        }
      }
    }
    
    sendJsonResponse(isLedOn, currentBrightness);
  });

  // Inicializa o servidor web
  server.begin();
  Serial.println("Servidor Web HTTP iniciado com sucesso.");
}

void loop() {
  server.handleClient();
}
