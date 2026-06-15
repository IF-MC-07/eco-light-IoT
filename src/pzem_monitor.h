#ifndef PZEM_MONITOR_H
#define PZEM_MONITOR_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════
// PZEM-004T V3.0 MONITORING MODULE (tanpa library eksternal)
// Membaca: Voltage, Current, Power, Energy, Frequency, Power Factor
// Menggunakan Modbus RTU via Serial2 (UART2)
// ════════════════════════════════════════════════════════════

struct PZEMData {
  float voltage;       // Volt
  float current;       // Ampere
  float power;         // Watt
  float energy;        // kWh
  float frequency;     // Hz
  float powerFactor;   // 0.00 - 1.00
  bool valid;          // Flag validasi data
};

PZEMData pzemData = {0, 0, 0, 0, 0, 0, false};
HardwareSerial pzemSerial(2);  // UART2

// ── Fungsi helper Modbus RTU CRC16 ────────────────────────
uint16_t calculateCRC16(uint8_t* buf, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= buf[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// ── Inisialisasi Serial2 PZEM ─────────────────────────────
void initPZEM() {
  Serial.println("[PZEM] Inisialisasi PZEM-004T v3.0 pada UART2...");
  Serial.printf("[PZEM] RX=GPIO%d, TX=GPIO%d, Baud=%d\n", PZEM_RX_PIN, PZEM_TX_PIN, PZEM_BAUD_RATE);
  
  pzemSerial.begin(PZEM_BAUD_RATE, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
  delay(500);
  
  Serial.println("[PZEM] ✓ Inisialisasi selesai");
}

// ── Baca register PZEM via Modbus RTU ──────────────────────
bool readPZEMRegister(uint8_t addr, uint16_t reg, uint16_t len, uint16_t* result) {
  // Buat request Modbus RTU
  uint8_t request[8];
  request[0] = addr;           // Slave address (default 1)
  request[1] = 0x03;           // Function code (Read Holding Registers)
  request[2] = (reg >> 8);     // Register address MSB
  request[3] = (reg & 0xFF);   // Register address LSB
  request[4] = (len >> 8);     // Number of registers MSB
  request[5] = (len & 0xFF);   // Number of registers LSB
  
  // Hitung CRC16
  uint16_t crc = calculateCRC16(request, 6);
  request[6] = (crc & 0xFF);
  request[7] = (crc >> 8);
  
  // Flush buffer sebelum kirim
  while (pzemSerial.available()) {
    pzemSerial.read();
  }
  
  // Kirim request
  pzemSerial.write(request, 8);
  pzemSerial.flush();
  
  // Tunggu response (max 500ms)
  unsigned long timeout = millis() + 500;
  uint8_t response[32];
  uint8_t idx = 0;
  
  while (millis() < timeout && idx < sizeof(response)) {
    if (pzemSerial.available()) {
      response[idx++] = pzemSerial.read();
    }
  }
  
  // Validasi response
  if (idx < (3 + len * 2 + 2)) {
    return false;  // Response tidak lengkap
  }
  
  if (response[0] != addr || response[1] != 0x03) {
    return false;  // Address atau function code tidak sesuai
  }
  
  // Verifikasi CRC response
  uint16_t respCRC = calculateCRC16(response, idx - 2);
  if ((respCRC & 0xFF) != response[idx - 2] || (respCRC >> 8) != response[idx - 1]) {
    return false;  // CRC tidak cocok
  }
  
  // Ekstrak data
  uint8_t byteCount = response[2];
  if (byteCount != len * 2) {
    return false;  // Byte count tidak sesuai
  }
  
  for (uint16_t i = 0; i < len; i++) {
    result[i] = ((uint16_t)response[3 + i*2] << 8) | response[4 + i*2];
  }
  
  return true;
}

// ── Baca semua parameter dari PZEM ──────────────────────────
bool readPZEMData() {
  // PZEM-004T v3.0 Register Addresses:
  // 0x0000: Voltage (V) - 1 register (2 bytes) -> V * 10
  // 0x0001: Current (A) - 1 register -> A * 100
  // 0x0002: Power (W) - 2 registers -> P * 1
  // 0x0004: Energy (kWh) - 2 registers -> E * 1
  // 0x0006: Frequency (Hz) - 1 register -> F * 10
  // 0x0007: Power Factor - 1 register -> PF * 100
  
  uint16_t registers[10];
  
  // Baca Voltage (1 register)
  if (!readPZEMRegister(1, 0x0000, 1, registers)) {
    Serial.println("[PZEM] ✗ Pembacaan voltage gagal");
    pzemData.valid = false;
    return false;
  }
  pzemData.voltage = registers[0] / 10.0f;
  
  // Baca Current (1 register)
  if (!readPZEMRegister(1, 0x0001, 1, registers)) {
    Serial.println("[PZEM] ✗ Pembacaan current gagal");
    pzemData.valid = false;
    return false;
  }
  pzemData.current = registers[0] / 1000.0f;
  
  // Baca Power (2 registers)
  if (!readPZEMRegister(1, 0x0002, 2, registers)) {
    Serial.println("[PZEM] ✗ Pembacaan power gagal");
    pzemData.valid = false;
    return false;
  }
  pzemData.power = ((uint32_t)registers[0] << 16) | registers[1];
  
  // Baca Energy (2 registers)
  if (!readPZEMRegister(1, 0x0004, 2, registers)) {
    Serial.println("[PZEM] ✗ Pembacaan energy gagal");
    pzemData.valid = false;
    return false;
  }
  pzemData.energy = ((uint32_t)registers[0] << 16) | registers[1];
  pzemData.energy /= 1000.0f;  // Convert to kWh
  
  // Baca Frequency (1 register)
  if (!readPZEMRegister(1, 0x0006, 1, registers)) {
    Serial.println("[PZEM] ✗ Pembacaan frequency gagal");
    pzemData.valid = false;
    return false;
  }
  pzemData.frequency = registers[0] / 10.0f;
  
  // Baca Power Factor (1 register)
  if (!readPZEMRegister(1, 0x0007, 1, registers)) {
    Serial.println("[PZEM] ✗ Pembacaan power factor gagal");
    pzemData.valid = false;
    return false;
  }
  pzemData.powerFactor = registers[0] / 100.0f;
  
  pzemData.valid = true;
  Serial.printf("[PZEM] V:%.2fV | I:%.3fA | P:%.2fW | E:%.3fkWh | F:%.2fHz | PF:%.2f\n",
                pzemData.voltage, pzemData.current, pzemData.power, pzemData.energy, 
                pzemData.frequency, pzemData.powerFactor);
  
  return true;
}

// ── Publish data PZEM ke MQTT dalam format JSON ────────────
bool publishPZEMtoMQTT(PubSubClient& mqttClient) {
  if (!mqttClient.connected()) {
    return false;
  }

  if (!pzemData.valid) {
    Serial.println("[PZEM] Tidak ada data valid untuk dipublish");
    return false;
  }

  // Buat JSON payload
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"voltage\":%.2f,\"current\":%.3f,\"power\":%.2f,\"energy\":%.3f,\"frequency\":%.2f,\"pf\":%.2f}",
    pzemData.voltage,
    pzemData.current,
    pzemData.power,
    pzemData.energy,
    pzemData.frequency,
    pzemData.powerFactor);

  // Publish ke topik energi
  if (!mqttClient.publish(TOPIC_ENERGY, payload)) {
    Serial.println("[MQTT] ✗ Publish PZEM data gagal");
    return false;
  }

  Serial.printf("[MQTT] ✓ Publish energi ke %s\n", TOPIC_ENERGY);
  return true;
}

#endif
