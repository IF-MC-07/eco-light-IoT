# Eco-Light & Space Optimizer - IoT Side (ESP32)

**Proyek Tugas Akhir IF-4MC-07**  
**Politeknik Negeri Batam Tahun 2026**

Sistem cerdas manajemen energi dan optimalisasi ruang berbasis **AIoT** (YOLOv8 + ESP32 + PZEM-004T).

## ✨ Fitur Utama
- Kontrol otomatis lampu per zona (4 zona) menggunakan Relay Module
- Kontrol AC melalui IR Transmitter
- Monitoring konsumsi energi real-time (Voltage, Current, Power, Energy, PF)
- Komunikasi dua arah via **MQTT**
- Auto reconnect WiFi & MQTT
- Logging debug yang jelas

## 📁 Struktur Folder
ECOLIGHT-IOT/
├── include/
│   ├── config.h
│   ├── relay_control.h
│   └── pzem_monitor.h
├── src/
│   └── main.cpp
├── platformio.ini
├── README.md
├── LICENSE
└── .gitignore

🛠️ Hardware yang Digunakan
- ESP32 DevKit V1
- 4 Channel Relay Module (Active LOW)
- PZEM-004T v3.0
- Modul Sensor Ldr
- Kamera (dikelola oleh AI Server)

## 🔌 Wiring / Pinout

| Komponen          | ESP32 GPIO     | Keterangan                  |
|-------------------|----------------|-----------------------------|
| Relay Channel 1   | 25             | Lampu Zone 1                |
| Relay Channel 2   | 26             | Lampu Zone 2                |
| Relay Channel 3   | 27             | Lampu Zone 3                |
| Relay Channel 4   | 33             | Lampu/AC Master             |
| PZEM-004T RX      | 16 (UART2)     | Data dari PZEM              |
| PZEM-004T TX      | 17 (UART2)     | Command ke PZEM             |

## 🚀 Cara Menjalankan

1. Buka project di **VS Code + PlatformIO**
2. Edit file `include/config.h`:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `MQTT_BROKER` (IP laptop/server AI)
3. Build (✓) → Upload
4. Buka **Serial Monitor** (baud 115200)

## 📡 MQTT Topics

- **Subscribe** (dari AI): `ecolight/zone/control`
- **Publish** (ke AI): `ecolight/energy/data`

**Contoh Payload:**
```json
{"zone": 1, "state": true}
{"zone": 4, "command": "off"}


👥 Tim Pengembang
IoT & Hardware : Ruth Yohana Manurung (3312411032)
AI, Web & Backend : Ridho Putrawan (3312411050)

Repository Lengkap: eco-light-space-optimizer

Dokumentasi lengkap ada di Laporan Tugas Akhir.