#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════
// RELAY CONTROL MODULE — Active LOW
//   LOW  = relay NYALA  (lampu menyala)
//   HIGH = relay MATI   (lampu mati)
// ════════════════════════════════════════════════════════════

struct RelayState { int r1, r2, r3, r4; };
extern RelayState currentRelay; // Gunakan extern agar bisa diakses di main.cpp
RelayState currentRelay = {0, 0, 0, 0};

void initRelays() {
  int pins[] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};
  for (int i = 0; i < 4; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], HIGH);  // MATI saat boot (Kondisi Aman)
  }
  Serial.println("[RELAY] Init → semua OFF ✓");
}

// ── FUNGSI BARU: Kontrol Per Channel (Sangat Penting untuk AI) ──
void controlRelay(int channel, bool state) {
  int targetPin;
  
  // Memetakan nomor channel dari AI ke Pin GPIO di config.h
  switch (channel) {
    case 1: targetPin = RELAY1_PIN; currentRelay.r1 = state; break;
    case 2: targetPin = RELAY2_PIN; currentRelay.r2 = state; break;
    case 3: targetPin = RELAY3_PIN; currentRelay.r3 = state; break;
    case 4: targetPin = RELAY4_PIN; currentRelay.r4 = state; break;
    default: 
      Serial.printf("[RELAY] ✗ Channel %d tidak dikenal!\n", channel);
      return;
  }

  // Eksekusi fisik: state true (1) jadi LOW (nyala), false (0) jadi HIGH (mati)
  digitalWrite(targetPin, state ? LOW : HIGH);
  Serial.printf("[RELAY] Channel %d diset ke %s\n", channel, state ? "ON" : "OFF");
}

void setAllRelays(int r1, int r2, int r3, int r4) {
  digitalWrite(RELAY1_PIN, r1 ? LOW : HIGH);
  digitalWrite(RELAY2_PIN, r2 ? LOW : HIGH);
  digitalWrite(RELAY3_PIN, r3 ? LOW : HIGH);
  digitalWrite(RELAY4_PIN, r4 ? LOW : HIGH);
  currentRelay = {r1, r2, r3, r4};
  Serial.printf("[RELAY] ALL -> A:%d B:%d C:%d D:%d\n", r1, r2, r3, r4);
}

void emergencyOff(const char* reason) {
  setAllRelays(0, 0, 0, 0);
  Serial.print("[SAFETY] EMERGENCY OFF: ");
  Serial.println(reason);
}

#endif