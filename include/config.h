#ifndef CONFIG_H
#define CONFIG_H

// ================================================
// CONFIG ECO-LIGHT IOT - Polibatam 2026
// ================================================

// WiFi
#define WIFI_SSID           "Sherina"               // GANTI SESUAI JARINGAN KAMU
#define WIFI_PASSWORD       "16november2003"        // GANTI!

// MQTT
#define MQTT_BROKER         "192.168.1.2"           // GANTI IP Laptop/Server AI
#define MQTT_PORT           1883
#define MQTT_CLIENT_ID      "ESP32_EcoLight_Ruth_01"

// MQTT Topics
#define TOPIC_CONTROL       "ecolight/zone/control"
#define TOPIC_ENERGY        "ecolight/energy/data"
#define TOPIC_STATUS        "ecolight/device/status"

// Pin Definitions
#define RELAY1_PIN  25
#define RELAY2_PIN  26
#define RELAY3_PIN  27
#define RELAY4_PIN  33

#define PZEM_RX_PIN     16
#define PZEM_TX_PIN     17
#define PZEM_BAUD_RATE  9600     // ← Ini yang diperbaiki

// Timing
#define INTERVAL_ENERGY     5000
#define RECONNECT_INTERVAL  5000

#endif