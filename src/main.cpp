#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "relay_control.h"
#include "pzem_monitor.h"

// ────────────────────────────────────────────────────────────
// OBJEK GLOBAL
// ────────────────────────────────────────────────────────────
WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool wifiOK = false, mqttOK = false;
unsigned long lastStatus = 0, lastLdr = 0, lastEnergy = 0;
unsigned long lastWiFiReconnect = 0, lastMqttReconnect = 0;

// ════════════════════════════════════════════════════════════
// MQTT CALLBACK — Menerima perintah kontrol relay dari Broker
// ════════════════════════════════════════════════════════════
void onMQTTMessage(char* topic, byte* payload, unsigned int len) {
  // Batasi payload untuk mencegah buffer overflow
  if (len >= MQTT_PAYLOAD_BUFFER_SIZE) {
    Serial.printf("[MQTT] Payload terlalu besar (%u)\n", len);
    return;
  }

  char msg[MQTT_PAYLOAD_BUFFER_SIZE + 1];
  memcpy(msg, payload, len);
  msg[len] = '\0';

  Serial.printf("\n[MQTT IN] Topic: %s\n[MQTT IN] Payload: %s\n", topic, msg);

  // Proses perintah kontrol relay dari topik 
  if (strncmp(topic, TOPIC_CONTROL, strlen(TOPIC_CONTROL)) == 0) {
    Serial.println("[Logic] Perintah kontrol zona diterima!");
    
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, msg);
    
    if (!err) {
      // AI mengirim format: {"command": "on", "relay_channel": 1, ...}
      const char* command = doc["command"];
      int channel = doc["relay_channel"];
      
      Serial.printf("[Action] Channel %d -> %s\n", channel, command);      
    

  // Eksekusi ke Relay (Asumsi 1 = ON/LOW, 0 = OFF/HIGH)
      bool state = (strcmp(command, "on") == 0);
      controlRelay(channel, state); 
    } else {
      Serial.print("[JSON] Gagal parsing: ");
      Serial.println(err.c_str());
    }
  }
}

// ──────────────────────────────────────────────────────────────
// KONEKSI WiFi
// ──────────────────────────────────────────────────────────────
bool connectWiFi() {
  Serial.printf("\n[WiFi] Mencoba konek ke: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println("\n[WiFi] ✗ Timeout!");
      Serial.printf("[WiFi] Status: %d\n", WiFi.status());
      return false;
    }
    delay(500);
    Serial.print(".");
    dots++;
    if (dots % 10 == 0) Serial.println("");  // Line break setiap 10 dots
  }

  Serial.printf("\n[WiFi] ✓ Terhubung!\n");
  Serial.printf("[WiFi] IP Address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WiFi] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("[WiFi] DNS: %s\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("[WiFi] RSSI: %d dBm (Signal Strength)\n", WiFi.RSSI());
  
  return true;
}

// ──────────────────────────────────────────────────────────────
// KONEKSI MQTT
// ──────────────────────────────────────────────────────────────
bool connectMQTT() {
  Serial.printf("\n[MQTT] Mencoba konek ke %s:%d\n", MQTT_BROKER, MQTT_PORT);
  Serial.printf("[MQTT] Client ID: %s\n", MQTT_CLIENT_ID);

  // Mencoba konek dengan LWT (Last Will & Testament) JSON yang benar
  // Jika ESP32 mati, broker otomatis kirim {"online":false} ke dashboard
  if (!mqttClient.connect(MQTT_CLIENT_ID, NULL, NULL, 
                         TOPIC_STATUS, 0, true, "{\"online\":false}")) {
    Serial.println("[MQTT] ✗ Gagal terhubung ke Broker.");
    return false; // Harus return false agar tidak lanjut ke subscribe jika gagal
  }

  Serial.println("[MQTT] ✓ Terhubung!");

  // Subscribe ke semua channel menggunakan Wildcard '#'
  // Misal: esp32/1/light/# (akan mendengarkan channel 1, 2, dan 3 sekaligus)
  String subTopic = String(TOPIC_CONTROL) + "#";
  if (!mqttClient.subscribe(subTopic.c_str(), MQTT_SUBSCRIBE_QOS)) {
    Serial.printf("[MQTT] ✗ Subscribe %s gagal\n", subTopic.c_str());
  } else {
    Serial.printf("[MQTT] ✓ Subscribe %s berhasil\n", subTopic.c_str());
  }

  // Publish status online dalam format JSON yang benar
  char ipBuf[1];
  WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));
  
  char statusPayload[2];
  snprintf(statusPayload, sizeof(statusPayload), 
           "{\"online\":true,\"ip\":\"%s\",\"rssi\":%d}", 
           ipBuf, WiFi.RSSI());
           
  mqttClient.publish(TOPIC_STATUS, statusPayload, true);

  return true;

}

// ──────────────────────────────────────────────────────────────
// CEK & JAGA KONEKSI WiFi & MQTT
// ──────────────────────────────────────────────────────────────
void checkConnections() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (wifiOK) {
      wifiOK = false;
      mqttOK = false;
      emergencyOff("WiFi putus");
    }
    if (now - lastWiFiReconnect >= WIFI_RECONNECT_INTERVAL) {
      lastWiFiReconnect = now;
      Serial.println("[WiFi] Koneksi hilang, mencoba reconnect...");
      WiFi.reconnect();
    }
    return;
  }

  if (!wifiOK) {
    wifiOK = true;
    Serial.printf("[WiFi] Kembali terhubung, IP %s\n", WiFi.localIP().toString().c_str());
  }

  if (!mqttClient.connected()) {
    if (mqttOK) {
      mqttOK = false;
      emergencyOff("MQTT putus");
    }
    if (now - lastMqttReconnect >= MQTT_RECONNECT) {
      lastMqttReconnect = now;
      Serial.println("[MQTT] Koneksi hilang, mencoba reconnect...");
      if (connectMQTT()) {
        mqttOK = true;
      }
    }
    return;
  }

  if (!mqttOK) {
    mqttOK = true;
    Serial.println("[MQTT] Kembali terhubung");
  }
}

// ──────────────────────────────────────────────────────────────
// PUBLISH DATA LDR
// ──────────────────────────────────────────────────────────────
void publishLDR() {
  if (!mqttClient.connected()) return;

  int val = analogRead(LDR_PIN);
  bool ov = (val > LDR_THRESHOLD);
  char msg[80];
  snprintf(msg, sizeof(msg), "{\"ldr\":%d,\"override\":%s}",
           val, ov ? "true" : "false");

  if (!mqttClient.publish(TOPIC_LDR, msg)) {
    Serial.println("[MQTT] ✗ Publish LDR gagal");
    mqttOK = false;
  }

  if (ov) {
    Serial.printf("[LDR] Override! %d > %d → semua OFF\n", val, LDR_THRESHOLD);
    setAllRelays(0, 0, 0, 0);
  }
}

// ──────────────────────────────────────────────────────────────
// PUBLISH STATUS DEVICE
// ──────────────────────────────────────────────────────────────
void publishStatus() {
  if (!mqttClient.connected()) return;

  char ipBuf[16] = {0};
  WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));

  char msg[120];
  snprintf(msg, sizeof(msg),
    "{\"online\":true,\"ip\":\"%s\",\"relay\":[%d,%d,%d,%d],\"rssi\":%d}",
    ipBuf,
    currentRelay.r1, currentRelay.r2, currentRelay.r3, currentRelay.r4,
    WiFi.RSSI());

  if (!mqttClient.publish(TOPIC_STATUS, msg)) {
    Serial.println("[MQTT] ✗ Publish status gagal");
    mqttOK = false;
  }
}

// ──────────────────────────────────────────────────────────────
// PUBLISH DATA ENERGI PZEM
// ──────────────────────────────────────────────────────────────
void publishEnergy() {
  if (!mqttClient.connected()) {
    Serial.println("[PZEM] MQTT tidak terhubung, lewati publish");
    return;
  }

  // Baca data PZEM
  if (!readPZEMData()) {
    Serial.println("[PZEM] Gagal membaca data, lewati publish");
    return;
  }

  // Publish data PZEM
  if (!publishPZEMtoMQTT(mqttClient)) {
    Serial.println("[PZEM] Gagal publish ke MQTT");
    mqttOK = false;
  }
}


// ════════════════════════════════════════════════════════════
// SETUP
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  ECO-LIGHT & SPACE OPTIMIZER          ║");
  Serial.println("║  with PZEM-004T Energy Monitoring     ║");
  Serial.println("║  Ruth Yohana Manurung                 ║");
  Serial.println("║  IF-4MC-07 | Polibatam 2026           ║");
  Serial.println("╚════════════════════════════════════════╝\n");

  // 1. Inisialisasi relay
  initRelays();
  Serial.println();

  // 2. Inisialisasi PZEM
  initPZEM();
  Serial.println();

  // 3. Koneksi WiFi
  wifiOK = connectWiFi();
  Serial.println();

  // 4. Setup MQTT client
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(onMQTTMessage);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);
  mqttClient.setBufferSize(MQTT_PAYLOAD_BUFFER_SIZE);

  // 5. Koneksi MQTT (jika WiFi OK)
  if (wifiOK) {
    mqttOK = connectMQTT();
  }

  Serial.println("\n[BOOT] ✓ Setup selesai. Siap menunggu perintah...\n");
}

// ════════════════════════════════════════════════════════════
// LOOP UTAMA (NON-BLOCKING)
// ════════════════════════════════════════════════════════════
void loop() {
  // 1. Jaga koneksi WiFi & MQTT tetap hidup
  checkConnections();

  // 2. Proses pesan MQTT jika terhubung
  if (mqttClient.connected()) {
    mqttClient.loop();
    mqttOK = true;
  } else {
    mqttOK = false;
  }

  unsigned long now = millis();

  // 3. Publish data LDR setiap INTERVAL_LDR
  if (now - lastLdr >= INTERVAL_LDR) {
    lastLdr = now;
    publishLDR();
  }

  // 4. Publish status & energi setiap INTERVAL_STATUS
  if (now - lastStatus >= INTERVAL_STATUS) {
    lastStatus = now;
    publishStatus();
    publishEnergy();  // Baca dan publish energi PZEM
  }
}