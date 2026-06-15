#ifndef CONFIG_H
#define CONFIG_H

// ═══════════════════════════════════════════════════════════
// KONFIGURASI ECO-LIGHT — IoT Side
// Ruth Yohana Manurung | IF-4MC-07 | Polibatam 2026
// ⚠️ Ganti WIFI_SSID, WIFI_PASSWORD, dan MQTT_BROKER!
// ═══════════════════════════════════════════════════════════

// ── WiFi ─────────────────────────────────────────────────
#define WIFI_SSID         "Sherina"     // ← GANTI
#define WIFI_PASSWORD     "16november2003"      // ← GANTI
#define WIFI_TIMEOUT_MS   15000  // Timeout koneksi WiFi (15 detik)

// ── MQTT Broker (IP Laptop tempat Mosquitto jalan) ───────
#define MQTT_BROKER       "192.168.1.2"        // ← GANTI IP laptop!
#define MQTT_PORT         1883
#define MQTT_CLIENT_ID    "ESP32_EcoLight_Ruth"
#define MQTT_RECONNECT    5000              // Interval reconnect MQTT (5 detik)
#define MQTT_PAYLOAD_BUFFER_SIZE 256        // Max payload size MQTT
#define MQTT_KEEPALIVE_SEC 30               // Keep-alive MQTT (30 detik)
#define MQTT_SUBSCRIBE_QOS 1                // QoS level untuk subscribe
#define WIFI_RECONNECT_INTERVAL 5000        // Interval reconnect WiFi (5 detik)

// ── MQTT Topics (Polibatam IoT Ecosystem) ────────
#define TOPIC_CONTROL     "esp32/1/light/"   // Subscribe untuk relay control
#define TOPIC_ENERGY      "polibatam/eco/energy"    // Publish data energi PZEM
#define TOPIC_STATUS      "polibatam/eco/status" 
#define MQTT_BROKER       "192.168.1.2"    

// ── MQTT Topics lama (opsional untuk kompatibilitas) ────────
#define TOPIC_LDR         "ecolight/room101/ldr"

// ── Format JSON Kontrol Relay ────────────────────────────
// Subscribe ke TOPIC_CONTROL, terima JSON:
//   {"relay1": 1, "relay2": 0, "relay3": 1, "relay4": 0}
// Nilai: 1 = ON (LOW), 0 = OFF (HIGH)
// Bisa kontrol relay individual, tidak perlu semua 4 relay

// ── Pin Relay (GPIO aman, hindari GPIO 12!) ───────────────
#define RELAY1_PIN  25   // Zone A — Lampu kiri atas
#define RELAY2_PIN  26   // Zone B — Lampu kanan atas
#define RELAY3_PIN  27   // Zone C — Lampu kiri bawah
#define RELAY4_PIN  33   // Zone D — AC control

// ── Pin PZEM-004T v3.0 (Hardware Serial 2) ───────────────
#define PZEM_RX_PIN       16    // GPIO16 → RX untuk UART2
#define PZEM_TX_PIN       17    // GPIO17 → TX untuk UART2
#define PZEM_UART_NUM     2     // UART2 (HardwareSerial2)
#define PZEM_BAUD_RATE    9600  // PZEM-004T default baud rate

// ── Format JSON Data Energi (publish ke TOPIC_ENERGY) ─────────────
// {"voltage":230.45, "current":0.850, "power":195.88,
//  "energy":12.345, "frequency":50.05, "pf":0.95}

// ── Pin Sensor ───────────────────────────────────────────
#define LDR_PIN       34    // ADC, ambil cahaya alami
#define LDR_THRESHOLD 2500  // > nilai ini → override OFF (semua relay OFF)

// ── Timing Interval Publish ───────────────────────────────
// INTERVAL_LDR: publish LDR setiap 1 detik (1000 ms)
// INTERVAL_STATUS: publish status device & energi PZEM setiap 5 detik (5000 ms)
#define INTERVAL_STATUS   5000   // ms — publish status + energy
#define INTERVAL_LDR      1000   // ms — publish LDR
#define INTERVAL_ENERGY   5000   // ms — publish energi PZEM (sync dengan status)

#endif