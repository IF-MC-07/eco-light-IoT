#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <Arduino.h>
#include "config.h"

void initRelays() {
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  
  digitalWrite(RELAY1_PIN, HIGH); // Active LOW
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(RELAY4_PIN, HIGH);
  
  Serial.println("[RELAY] Semua relay diinisialisasi (OFF)");
}

void controlRelay(int zone, bool state) {
  int pin;
  switch(zone) {
    case 1: pin = RELAY1_PIN; break;
    case 2: pin = RELAY2_PIN; break;
    case 3: pin = RELAY3_PIN; break;
    case 4: pin = RELAY4_PIN; break;
    default: Serial.println("[RELAY] Zone tidak valid!"); return;
  }
  
  digitalWrite(pin, state ? LOW : HIGH);
  Serial.printf("[RELAY] Zone %d → %s\n", zone, state ? "ON" : "OFF");
}

#endif