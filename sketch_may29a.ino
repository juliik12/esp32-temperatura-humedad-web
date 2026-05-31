#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>
#include <DHT.h>

const char* ssid = "Daniela2022";
const char* password = "5748conesa";

#define DHTPIN 14
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

AsyncWebServer server(80);
WebSocketsServer webSocket(81);

float temperatura = 0;
float humedad = 0;

unsigned long lastSend = 0;
const unsigned long sendInterval = 2000;

void notFound(AsyncWebServerRequest *request);

void sendData() {
  String data = "{\"temperature\":" +
                String(temperatura, 1) +
                ",\"humedad\":" +
                String(humedad, 1) +
                "}";

  webSocket.broadcastTXT(data);
  Serial.println(data);
}

void handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {

    case WStype_DISCONNECTED:
      Serial.printf("[%u] Cliente desconectado\n", num);
      break;

    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);

      Serial.printf("[%u] Cliente conectado desde %d.%d.%d.%d\n",
                    num, ip[0], ip[1], ip[2], ip[3]);
    }
    break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n===== BOOT =====");

  // WiFi estable
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFi.begin(ssid, password);

  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.println(WiFi.localIP());

  dht.begin();

  // =========================
  // LITTLEFS (CORRECTO)
  // =========================
  if (!LittleFS.begin(false)) {
    Serial.println("Error montando LittleFS");
    return;
  }

  Serial.println("LittleFS OK");

  // listar archivos
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {
    Serial.print("Archivo: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }

  // =========================
  // HTTP SERVER
  // =========================
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /");

    if (LittleFS.exists("/index.html")) {
      request->send(LittleFS, "/index.html", "text/html");
    } else {
      request->send(200, "text/plain", "index.html no encontrado");
    }
  });

  server.onNotFound(notFound);

  server.begin();

  // =========================
  // WEBSOCKET
  // =========================
  webSocket.begin();
  webSocket.onEvent(handleWebSocketMessage);

  Serial.println("Servidor iniciado");
}

void loop() {
  webSocket.loop();

  if (millis() - lastSend >= sendInterval) {
    lastSend = millis();

    temperatura = dht.readTemperature();
    humedad = dht.readHumidity();

    if (isnan(temperatura) || isnan(humedad)) {
      Serial.println("Error DHT11");
      return;
    }

    sendData();
  }
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Pagina no encontrada");
}