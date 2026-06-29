#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "relay_control.h"
#include "pzem_monitor.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastWiFiRetry = 0;
unsigned long lastMQTTRetry = 0;
unsigned long lastEnergyPublish = 0;

String getISO8601Time() {
    time_t now;
    time(&now);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "1970-01-01T00:00:00Z";
    }
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return String(buf);
}

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    
    Serial.print("[WiFi] Menghubungkan ke SSID: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[WiFi] Terhubung! IP: ");
        Serial.println(WiFi.localIP());
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    } else {
        Serial.println("[WiFi] Gagal terhubung (Timeout). Lanjut eksekusi...");
    }
}

int getRelayPin(int channel) {
    switch(channel) {
        case 1: return RELAY_CH1_PIN;
        case 2: return RELAY_CH2_PIN;
        case 3: return RELAY_CH3_PIN;
        case 4: return RELAY_CH4_PIN;
        default: return -1;
    }
}

void on_message(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    if (topicStr.startsWith("devices/" ROOM_ID "/light/")) {
        int lastSlash = topicStr.lastIndexOf('/');
        if (lastSlash != -1) {
            String chStr = topicStr.substring(lastSlash + 1);
            int channel = chStr.toInt();
            
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, payloadStr);
            if (!error) {
                const char* command = doc["command"];
                if (command && channel >= 1 && channel <= 4) {
                    bool stateToSet = (String(command) == "on");
                    setRelay(channel, stateToSet);
                    int pin = getRelayPin(channel);
                    
                    Serial.print("[RELAY] Channel ");
                    Serial.print(channel);
                    Serial.print(" -> ");
                    Serial.print(stateToSet ? "ON" : "OFF");
                    Serial.print(" (GPIO");
                    Serial.print(pin);
                    Serial.println(")");
                }
            }
        }
    } else if (topicStr == TOPIC_SUBSCRIBE_STATUS) {
        String statusPayload = buildStatusPayload();
        mqttClient.publish(TOPIC_PUBLISH_STATUS, statusPayload.c_str());
        Serial.println("[MQTT] Published status/response");
    }
}

void connectMQTT() {
    if (mqttClient.connected()) return;
    
    Serial.println("[MQTT] Menghubungkan ke broker...");
    String clientId = "ESP32Client-" + String(ROOM_ID);
    
    if (mqttClient.connect(clientId.c_str())) {
        Serial.println("[MQTT] Terhubung!");
        
        StaticJsonDocument<256> doc;
        doc["room_id"] = ROOM_ID;
        doc["status"] = "online";
        doc["timestamp"] = getISO8601Time();
        
        String onlinePayload;
        serializeJson(doc, onlinePayload);
        mqttClient.publish(TOPIC_PUBLISH_ONLINE, onlinePayload.c_str());
        
        mqttClient.subscribe(TOPIC_SUBSCRIBE_LIGHT);
        Serial.print("[MQTT] Subscribe berhasil: ");
        Serial.println(TOPIC_SUBSCRIBE_LIGHT);
        
        mqttClient.subscribe(TOPIC_SUBSCRIBE_STATUS);
    } else {
        Serial.print("[MQTT] Gagal terhubung, rc=");
        Serial.println(mqttClient.state());
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    initRelays();
    initPZEM();
    
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(on_message);
    
    connectWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        connectMQTT();
    }
}

void loop() {
    unsigned long currentMillis = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
        if (currentMillis - lastWiFiRetry >= MQTT_RECONNECT_INTERVAL_MS) {
            lastWiFiRetry = currentMillis;
            connectWiFi();
        }
    } else {
        if (!mqttClient.connected()) {
            if (currentMillis - lastMQTTRetry >= MQTT_RECONNECT_INTERVAL_MS) {
                lastMQTTRetry = currentMillis;
                connectMQTT();
            }
        } else {
            mqttClient.loop();
        }
    }
    
    if (currentMillis - lastEnergyPublish >= ENERGY_PUBLISH_INTERVAL_MS) {
        lastEnergyPublish = currentMillis;
        
        float v = 0.0, i = 0.0, p = 0.0, e = 0.0, f = 0.0, pf = 0.0;
        readPZEM(v, i, p, e, f, pf);
        
        Serial.print("[PZEM] voltage:");
        Serial.print(v, 1);
        Serial.print(" current:");
        Serial.print(i, 2);
        Serial.print(" power:");
        Serial.println(p, 1);
        
        if (mqttClient.connected()) {
            StaticJsonDocument<256> doc;
            doc["room_id"] = ROOM_ID;
            doc["voltage"] = v;
            doc["current"] = i;
            doc["power"] = p;
            doc["energy"] = e;
            doc["frequency"] = f;
            doc["pf"] = pf;
            doc["timestamp"] = getISO8601Time();
            
            String payload;
            serializeJson(doc, payload);
            mqttClient.publish(TOPIC_PUBLISH_ENERGY, payload.c_str());
            Serial.print("[ENERGY] Published to ");
            Serial.println(TOPIC_PUBLISH_ENERGY);
        }
    }
}