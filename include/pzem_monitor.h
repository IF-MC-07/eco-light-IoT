#ifndef PZEM_MONITOR_H
#define PZEM_MONITOR_H

#include <Arduino.h>
#include "config.h"

inline void initPZEM() {
    // PZEM-004T v3 uses Modbus RTU 9600 8N1
    Serial2.begin(PZEM_BAUD_RATE, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
}

inline uint16_t calculateCRC(uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)data[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

inline bool readPZEM(float &v, float &i, float &p, float &e, float &f, float &pf) {
    // Initialize to 0.0 as requested
    v = 0.0; i = 0.0; p = 0.0; e = 0.0; f = 0.0; pf = 0.0;

    // Clear RX buffer
    while (Serial2.available()) {
        Serial2.read();
    }

    // Request 10 registers from address 0x0000
    // Slave addr: 0x01, Func: 0x04, Addr: 0x0000, Count: 0x000A
    uint8_t request[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x70, 0x0D};
    Serial2.write(request, sizeof(request));
    Serial2.flush();

    // Wait for response, Modbus RTU max response is usually within a few ms
    uint32_t startTime = millis();
    while (Serial2.available() < 25) {
        if (millis() - startTime > 1000) {
            return false; // Timeout
        }
        delay(10);
    }

    uint8_t response[25];
    for (int j = 0; j < 25; j++) {
        response[j] = Serial2.read();
    }

    // Check minimum requirements (Address, Function, Bytes count)
    if (response[0] != 0x01 || response[1] != 0x04 || response[2] != 0x14) {
        return false;
    }

    // Verify CRC
    uint16_t receivedCRC = response[23] | (response[24] << 8);
    uint16_t calculatedCRC = calculateCRC(response, 23);
    if (receivedCRC != calculatedCRC) {
        return false;
    }

    // Parse data
    uint16_t rawVoltage = (response[3] << 8) | response[4];
    uint32_t rawCurrent = (((uint32_t)response[7] << 8) | response[8]) << 16 | (((uint32_t)response[5] << 8) | response[6]);
    uint32_t rawPower = (((uint32_t)response[11] << 8) | response[12]) << 16 | (((uint32_t)response[9] << 8) | response[10]);
    uint32_t rawEnergy = (((uint32_t)response[15] << 8) | response[16]) << 16 | (((uint32_t)response[13] << 8) | response[14]);
    uint16_t rawFreq = (response[17] << 8) | response[18];
    uint16_t rawPf = (response[19] << 8) | response[20];

    v = rawVoltage / 10.0f;
    i = rawCurrent / 1000.0f;
    p = rawPower / 10.0f;
    e = rawEnergy;
    f = rawFreq / 10.0f;
    pf = rawPf / 100.0f;

    return true;
}

#endif // PZEM_MONITOR_H