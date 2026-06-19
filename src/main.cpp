#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "relay_control.h"
#include "pzem_monitor.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastEnergy = 0;

// ====================== CALLBACK MQTT ======================
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("\n[MQTT] Pesan masuk → %s\n", topic);

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (!error) {
    int zone = doc["zone"] | doc["relay_channel"] | 1;
    bool state = doc["state"] | (strcmp(doc["command"], "on") == 0);
    controlRelay(zone, state);
  } else {
    Serial.println("[JSON] Parse gagal, coba fallback...");
    // Fallback string
    String str = message;
    str.toLowerCase();
    if (str.indexOf("on") != -1) controlRelay(1, true);
    else if (str.indexOf("off") != -1) controlRelay(1, false);
  }
}

// ====================== RECONNECT ======================
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Menghubungkan...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println(" Berhasil!");
      mqttClient.subscribe(TOPIC_CONTROL);
    } else {
      Serial.print(" Gagal, rc=");
      Serial.print(mqttClient.state());
      delay(RECONNECT_INTERVAL);
    }
  }
}

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ECO-LIGHT IOT STARTED ===");
  
  initRelays();
  initPZEM();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callback);
}

// ====================== LOOP ======================
void loop() {
  if (!mqttClient.connected()) reconnect();
  mqttClient.loop();

  // Publish Energy
  if (millis() - lastEnergy > INTERVAL_ENERGY) {
    lastEnergy = millis();
    if (readPZEMData()) {
      publishPZEMtoMQTT(mqttClient);
    }
  }
}