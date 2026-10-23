#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoModbus.h>
#include <vector>

// Wi-Fi Hotspot credentials
const char ssid[] = "Opta_Modbus_AP";
const char pass[] = "12345678"; // Minimum 8 characters for WPA2

const std::vector<uint8_t> leds = {LED_D0, LED_D1, LED_D2, LED_D3};
const unsigned long ledInterval = 1000;

// Hotspot IP configuration
const IPAddress ip(10, 0, 0, 1);
const IPAddress dns(0, 0, 0, 0);
const IPAddress subnet(255, 255, 255, 0);

WiFiServer wifiServer(502);
ModbusTCPServer modbusTCPServer;

std::vector<WiFiClient> clients;
const size_t MAX_CLIENTS = 3;

size_t currentLedIndex = 0;
uint16_t sampleCounter = 0;
unsigned long lastModbusDataUpdate = 0;

void updateLeds() {
  static unsigned long lastLedUpdate = 0;
  unsigned long now = millis();
  if (now - lastLedUpdate >= ledInterval) {
    lastLedUpdate = now;
    digitalWrite(leds[currentLedIndex], LOW);
    currentLedIndex = (currentLedIndex + 1) % leds.size();
    digitalWrite(leds[currentLedIndex], HIGH);
  }
}

void handleModbusClients() {
  WiFiClient newClient = wifiServer.accept();
  if (newClient) {
    if (clients.size() < MAX_CLIENTS) {
      clients.push_back(newClient);
      Serial.print("New Modbus TCP client connected! Active clients: ");
      Serial.println(clients.size());
    } else {
      Serial.println("Max client limit reached. Connection refused.");
      newClient.stop();
    }
  }

  for (auto client = clients.begin(); client != clients.end(); ) {
    if (client->connected()) {
      if (client->available() > 0) {
        modbusTCPServer.accept(*client);
        modbusTCPServer.poll();
      }
      ++client;
    } else {
      Serial.println("Client disconnected.");
      client->stop();
      client = clients.erase(client);
    }
  }
}

void setup() {
  Serial.begin(115200);

  clients.reserve(MAX_CLIENTS);

  for (uint8_t led : leds) {
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);
  }
  digitalWrite(leds[0], HIGH);

  Serial.println("Configuring Wi-Fi Access Point...");

  WiFi.config(ip, dns, subnet);

  int status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING && status != WL_CONNECTED) {
    Serial.println("Error: Failed to create Wi-Fi Hotspot!");
  } else {
    Serial.print("Hotspot active! SSID: ");
    Serial.println(ssid);
    Serial.print("Opta IP: ");
    Serial.println(WiFi.localIP());
  }

  wifiServer.begin();

  if (!modbusTCPServer.begin(1)) {
    Serial.println("Error: Failed to start Modbus TCP Server!");
  } else {
    Serial.println("Modbus TCP Server active on port 502.");
    modbusTCPServer.configureHoldingRegisters(0x00, 10);
    modbusTCPServer.configureCoils(0x00, 4);
  }
}

void loop() {
  updateLeds();
  handleModbusClients();

  unsigned long now = millis();
  if (now - lastModbusDataUpdate >= 1000) {
    lastModbusDataUpdate = now;
    sampleCounter++;

    modbusTCPServer.holdingRegisterWrite(0x00, (uint16_t)(now / 1000));
    modbusTCPServer.holdingRegisterWrite(0x01, (uint16_t)currentLedIndex);
    modbusTCPServer.holdingRegisterWrite(0x02, sampleCounter);

    for (size_t i = 0; i < leds.size(); i++) {
      modbusTCPServer.coilWrite(i, (i == currentLedIndex) ? 1 : 0);
    }
  }
}