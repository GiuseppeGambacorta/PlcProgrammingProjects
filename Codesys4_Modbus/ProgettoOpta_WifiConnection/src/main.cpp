#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoModbus.h>
#include <vector>


// Wifi Credential
const char ssid[] = "TP-LINK_2.4GHz_E0FF27";
const char pass[] = "gambacorta";

// Static IP network configuration
const IPAddress ip(192, 168, 1, 155);      // Static IP for Arduino Opta
const IPAddress gateway(0, 0, 0, 0);    // Router / Default Gateway IP
const IPAddress subnet(255, 255, 255, 0);   // Subnet mask
const IPAddress dns(0, 0, 0, 0);          // Nessun server DNS       // DNS Server IP (or 8.8.8.8)

const std::vector<uint8_t> leds = {LED_D0, LED_D1, LED_D2, LED_D3};
const unsigned long ledInterval = 1000;

WiFiServer wifiServer(502);
ModbusTCPServer modbusTCPServer;

std::vector<WiFiClient> clients;
const size_t MAX_CLIENTS = 3;

size_t currentLedIndex = 0;
uint16_t sampleCounter = 0;
uint16_t valueFromCodesys = 0;
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

  Serial.print("Connecting to Wi-Fi network: ");
  Serial.println(ssid);

  // Configure static network parameters before initiating the connection
  WiFi.config(ip, dns, gateway, subnet);

  // Connect in Station mode
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected successfully!");
  Serial.print("Static IP Address assigned: ");
  Serial.println(WiFi.localIP());

  wifiServer.begin();

  if (!modbusTCPServer.begin(1)) {
    Serial.println("Error: Failed to start Modbus TCP Server!");
  } else {
    Serial.println("Modbus TCP Server active on port 502.");
    modbusTCPServer.configureHoldingRegisters(0x00, 10);
    modbusTCPServer.configureCoils(0x00, 5);


    modbusTCPServer.configureInputRegisters(0x00, 10);
    modbusTCPServer.configureDiscreteInputs(0x00, 5);

  }
}

void loop() {
  updateLeds();
  handleModbusClients();

  unsigned long now = millis();
  if (now - lastModbusDataUpdate >= 100) {
    lastModbusDataUpdate = now;
    sampleCounter++;

    modbusTCPServer.inputRegisterWrite(0x00, (uint16_t)(now / 1000));
    modbusTCPServer.inputRegisterWrite(0x01, (uint16_t)currentLedIndex);
    modbusTCPServer.inputRegisterWrite(0x02, sampleCounter);
    modbusTCPServer.inputRegisterWrite(0x03, sampleCounter);


    for (size_t i = 0; i < leds.size(); i++) {
      modbusTCPServer.discreteInputWrite(i, (i == currentLedIndex) ? 1 : 0);
    }


    valueFromCodesys = modbusTCPServer.holdingRegisterRead(0x00);

    modbusTCPServer.discreteInputWrite(4, valueFromCodesys > 50);
  }
}