#pragma once
#include "defines.h"

void init_pressureSystem() {
    pinMode(zuluftHPin,   OUTPUT);
    pinMode(verbinderPin, OUTPUT);
    pinMode(zuluftVPin,   OUTPUT);
    digitalWrite(zuluftHPin,   LOW);
    digitalWrite(verbinderPin, LOW);
    digitalWrite(zuluftVPin,   LOW);
}

void messure() {
    sensorValueH = analogRead(sensorHPin) - sensorValueG;
    sensorValueV = analogRead(sensorVPin) - diff - sensorValueG;
    baselineH = baselineH * 0.990f + sensorValueH * 0.010f;
    baselineV = baselineV * 0.990f + sensorValueV * 0.010f;
}

void calibrate() {
    // Open valves to fully deflate
    digitalWrite(zuluftHPin, HIGH);
    digitalWrite(zuluftVPin, HIGH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    digitalWrite(zuluftHPin, LOW);
    digitalWrite(zuluftVPin, LOW);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    const int N = 20;
    sensorValueH = 0;
    sensorValueV = 0;
    for (int i = 0; i < N; ++i) {
        sensorValueH += analogRead(sensorHPin);
        sensorValueV += analogRead(sensorVPin);
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    sensorValueH /= N;
    sensorValueV /= N;
    diff      = sensorValueV - sensorValueH;
    base      = sensorValueH;
    baselineH = base;
    baselineV = base;
}

// ── Valve strategies ──────────────────────────────────────────────────────────
void pumpFront() {
    digitalWrite(zuluftHPin, sensorValueH < base - threshold ? HIGH : LOW);
    digitalWrite(zuluftVPin, sensorValueV < base - threshold ? HIGH : LOW);
    digitalWrite(verbinderPin, sensorValueV < sensorValueH - threshold ? HIGH : LOW);
}

void pumpBack() {
    digitalWrite(zuluftHPin, sensorValueH < base - threshold ? HIGH : LOW);
    digitalWrite(zuluftVPin, sensorValueV < base - threshold ? HIGH : LOW);
    digitalWrite(verbinderPin, sensorValueH < sensorValueV - threshold ? HIGH : LOW);
}

void evenOut(int milliseconds) {
    if (milliseconds > 0) {
        if (valveTimer == 0) {
            valveTimer = millis();
        } else if (valveTimer + milliseconds < millis()) {
            state = 3;
            valveTimer = 0;
        }
    }
    digitalWrite(verbinderPin, HIGH);
    digitalWrite(zuluftHPin,   LOW);
    digitalWrite(zuluftVPin,   LOW);
}

// Hold both chambers at a fixed pressure target
void holdValue(int target) {
    digitalWrite(zuluftHPin, sensorValueH < base - threshold ? HIGH : LOW);
    digitalWrite(zuluftVPin, sensorValueV < base - threshold ? HIGH : LOW);
    if (sensorValueV < target) {
        digitalWrite(verbinderPin, sensorValueV < sensorValueH - threshold ? HIGH : LOW);
    } else {
        digitalWrite(zuluftHPin,   LOW);
        digitalWrite(verbinderPin, LOW);
        digitalWrite(zuluftVPin,   LOW);
    }
}

// Pump to target, then auto even-out
void setValue(int target) {
    if (sensorValueV < target) {
        digitalWrite(zuluftHPin, sensorValueH < base - threshold ? HIGH : LOW);
        digitalWrite(zuluftVPin, sensorValueV < base - threshold ? HIGH : LOW);
        digitalWrite(verbinderPin, sensorValueV < sensorValueH - threshold ? HIGH : LOW);
    } else {
        state = 2; // transition to Even Out
    }
}

void updateValveSystem() {
    switch (state) {
        case 0: pumpFront();  break;
        case 1: pumpBack();   break;
        case 2: evenOut(1000); break;
        case 3:
            digitalWrite(zuluftHPin,   LOW);
            digitalWrite(verbinderPin, LOW);
            digitalWrite(zuluftVPin,   LOW);
            break;
        case 4:
            digitalWrite(zuluftHPin,   HIGH);
            digitalWrite(verbinderPin, LOW);
            digitalWrite(zuluftVPin,   HIGH);
            break;
        case 5: evenOut(0); break;
        case 6:
            calibrate();
            state = 3;
            break;
        default:
            if (state < 5000) holdValue(state);
            else              setValue((int)(state * 0.1f));
            break;
    }
    if (++cc >= 1000) cc = 0;
}

// ── MPU-6050 (I2C address 0x68) ───────────────────────────────────────────────
void setupMPU() {
    Wire.beginTransmission(0x68);
    Wire.write(0x6B); Wire.write(0x00); // Wake up
    Wire.endTransmission();

    Wire.beginTransmission(0x68);
    Wire.write(0x1B); Wire.write(0x08); // Gyro ±500°/s
    Wire.endTransmission();

    Wire.beginTransmission(0x68);
    Wire.write(0x1C); Wire.write(0x08); // Accel ±4g
    Wire.endTransmission();
}

void recordAccelRegisters() {
    Wire.beginTransmission(0x68);
    Wire.write(0x3B);
    Wire.endTransmission();
    Wire.requestFrom(0x68, 6);
    while (Wire.available() < 6);
    accelX = Wire.read() << 8 | Wire.read();
    accelY = Wire.read() << 8 | Wire.read();
    accelZ = Wire.read() << 8 | Wire.read();
}

void recordGyroRegisters() {
    Wire.beginTransmission(0x68);
    Wire.write(0x43);
    Wire.endTransmission();
    Wire.requestFrom(0x68, 6);
    while (Wire.available() < 6);
    gyroX = Wire.read() << 8 | Wire.read();
    gyroY = Wire.read() << 8 | Wire.read();
    gyroZ = Wire.read() << 8 | Wire.read();
}
