#ifndef PZEM_MONITOR_H
#define PZEM_MONITOR_H

#include <Arduino.h>
#include "config.h"

struct PZEMData {
  float voltage = 0.0;
  float current = 0.0;
  float power = 0.0;
  float energy = 0.0;
  float frequency = 0.0;
  float powerFactor = 0.0;
  bool valid = false;
};

PZEMData pzemData;
HardwareSerial pzemSerial(2);

// CRC16
uint16_t calculateCRC16(uint8_t *buf, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= buf[i];
    for (uint8_t j = 0; j < 8; j++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
  }
  return crc;
}

void initPZEM() {
  Serial.println("[PZEM] Inisialisasi PZEM-004T v3.0...");
  pzemSerial.begin(PZEM_BAUD_RATE, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
  delay(2000);                    // Delay lebih lama saat init
  Serial.println("[PZEM] UART2 siap");
}

// Baca satu register dengan delay lebih aman
bool readPZEMRegister(uint8_t addr, uint16_t reg, uint16_t len, uint16_t* result) {
  uint8_t request[8];
  request[0] = addr;
  request[1] = 0x03;
  request[2] = reg >> 8;
  request[3] = reg & 0xFF;
  request[4] = len >> 8;
  request[5] = len & 0xFF;

  uint16_t crc = calculateCRC16(request, 6);
  request[6] = crc & 0xFF;
  request[7] = crc >> 8;

  // Bersihkan buffer
  pzemSerial.flush();
  while (pzemSerial.available()) pzemSerial.read();

  pzemSerial.write(request, 8);
  pzemSerial.flush();

  delay(200);   // Delay penting setelah kirim request

  unsigned long timeout = millis() + 1000;
  uint8_t response[32] = {0};
  uint8_t idx = 0;

  while (millis() < timeout && idx < 32) {
    if (pzemSerial.available()) {
      response[idx++] = pzemSerial.read();
    }
  }

  if (idx < 5) {
    Serial.printf("[PZEM] Response pendek (%d bytes)\n", idx);
    return false;
  }

  if (response[0] != addr || response[1] != 0x03) {
    Serial.println("[PZEM] Response header salah");
    return false;
  }

  // Cek CRC
  uint16_t respCRC = calculateCRC16(response, idx - 2);
  if ((respCRC & 0xFF) != response[idx-2] || (respCRC >> 8) != response[idx-1]) {
    Serial.println("[PZEM] CRC Error");
    return false;
  }

  for (uint16_t i = 0; i < len; i++) {
    result[i] = (response[3 + i*2] << 8) | response[4 + i*2];
  }
  return true;
}

bool readPZEMData() {
  uint16_t reg[4];
  Serial.println("[PZEM] Membaca data...");

  // Baca Voltage saja dulu
  if (!readPZEMRegister(1, 0x0000, 1, reg)) {
    Serial.println("[PZEM] ✗ Gagal baca Voltage");
    pzemData.valid = false;
    return false;
  }

  pzemData.voltage = reg[0] / 10.0f;
  
  Serial.printf("[PZEM] Voltage = %.1f V\n", pzemData.voltage);

  if (pzemData.voltage > 100.0) {   // Hanya baca data lain kalau tegangan masuk akal
    readPZEMRegister(1, 0x0001, 1, reg);
    pzemData.current = reg[0] / 1000.0f;

    readPZEMRegister(1, 0x0002, 2, reg);
    pzemData.power = ((uint32_t)reg[0] << 16) | reg[1];

    Serial.printf("[PZEM] Power = %.1f W | Current = %.3f A\n", pzemData.power, pzemData.current);
    pzemData.valid = true;
  }

  return pzemData.valid;
}

bool publishPZEMtoMQTT(PubSubClient& mqttClient) {
  if (!pzemData.valid || !mqttClient.connected()) return false;

  char payload[180];
  snprintf(payload, sizeof(payload), 
    "{\"voltage\":%.1f,\"current\":%.3f,\"power\":%.1f,\"energy\":0.0,\"pf\":0.95}", 
    pzemData.voltage, pzemData.current, pzemData.power);

  bool ok = mqttClient.publish(TOPIC_ENERGY, payload);
  Serial.printf("[MQTT] %s data energi\n", ok ? "✓ Published" : "✗ Gagal");
  return ok;
}

#endif