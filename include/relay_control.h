#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

inline void initRelays() {
    pinMode(RELAY_CH1_PIN, OUTPUT);
    pinMode(RELAY_CH2_PIN, OUTPUT);
    pinMode(RELAY_CH3_PIN, OUTPUT);
    pinMode(RELAY_CH4_PIN, OUTPUT);
    
    // Active LOW: set HIGH to turn OFF initially
    digitalWrite(RELAY_CH1_PIN, HIGH);
    digitalWrite(RELAY_CH2_PIN, HIGH);
    digitalWrite(RELAY_CH3_PIN, HIGH);
    digitalWrite(RELAY_CH4_PIN, HIGH);
}

inline void setRelay(int channel, bool state) {
    // state = true for ON (LOW), false for OFF (HIGH)
    int pin = -1;
    switch(channel) {
        case 1: pin = RELAY_CH1_PIN; break;
        case 2: pin = RELAY_CH2_PIN; break;
        case 3: pin = RELAY_CH3_PIN; break;
        case 4: pin = RELAY_CH4_PIN; break;
        default: return; // Invalid channel
    }
    
    if (state) {
        digitalWrite(pin, LOW);
    } else {
        digitalWrite(pin, HIGH);
    }
}

inline bool getRelayStatus(int channel) {
    int pin = -1;
    switch(channel) {
        case 1: pin = RELAY_CH1_PIN; break;
        case 2: pin = RELAY_CH2_PIN; break;
        case 3: pin = RELAY_CH3_PIN; break;
        case 4: pin = RELAY_CH4_PIN; break;
        default: return false; // Invalid channel
    }
    // Active LOW: return true if LOW
    return (digitalRead(pin) == LOW);
}

inline String buildStatusPayload() {
    StaticJsonDocument<512> doc;
    doc["room_id"] = ROOM_ID;
    
    JsonArray lights = doc.createNestedArray("lights");
    
    for (int i = 1; i <= 4; i++) {
        JsonObject light = lights.createNestedObject();
        light["relay_channel"] = i;
        light["status"] = getRelayStatus(i) ? "on" : "off";
    }
    
    doc["ac_status"] = "off";
    doc["temperature"] = (char*)NULL; // null in JSON
    
    String payload;
    serializeJson(doc, payload);
    return payload;
}

#endif // RELAY_CONTROL_H