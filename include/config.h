#ifndef CONFIG_H
#define CONFIG_H

// --- WiFi Settings ---
#define WIFI_SSID "Sherina"
#define WIFI_PASS "16november2003"
#define WIFI_TIMEOUT_MS 15000

// --- MQTT Settings ---
#define MQTT_BROKER "192.168.1.6"
#define MQTT_PORT 1883

#define ROOM_ID "ROM-1464452b"

// --- MQTT Topics ---
#define TOPIC_SUBSCRIBE_LIGHT "devices/" ROOM_ID "/light/+"
#define TOPIC_SUBSCRIBE_STATUS "devices/" ROOM_ID "/status/request"
#define TOPIC_PUBLISH_ONLINE "devices/" ROOM_ID "/status/online"
#define TOPIC_PUBLISH_STATUS "devices/" ROOM_ID "/status/response"
#define TOPIC_PUBLISH_ENERGY "devices/" ROOM_ID "/energy"
#define TOPIC_PUBLISH_RELAY "devices/" ROOM_ID "/relay/status"

// --- Hardware Pins ---
// Relays (Active LOW)
#define RELAY_CH1_PIN 25 // Zone A
#define RELAY_CH2_PIN 26 // Zone B
#define RELAY_CH3_PIN 27 // Zone C
#define RELAY_CH4_PIN 33 // Zone D

// PZEM-004T (Modbus RTU)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_BAUD_RATE 9600

// LDR
#define LDR_PIN 34

// --- Intervals ---
#define ENERGY_PUBLISH_INTERVAL_MS 5000
#define MQTT_RECONNECT_INTERVAL_MS 5000

#endif // CONFIG_H