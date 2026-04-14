#pragma once
#include "defines.h"

void init_vibration() {
    // ESP32 Arduino 3.x: ledcAttach(pin, freq, resolution) — no separate channel
    for (int i = 0; i < 6; i++) {
        bool ok = ledcAttach(VIBRATOR_PINS[i], PWM_FREQ, PWM_RES);
        Serial.printf("[VIB] Motor %d → GPIO %d  ledcAttach: %s\n",
                      i, VIBRATOR_PINS[i], ok ? "OK" : "FAIL");
        ledcWrite(VIBRATOR_PINS[i], 0);
    }
}

void setVibration(int value) {
    for (int i = 0; i < 6; i++) ledcWrite(VIBRATOR_PINS[i], value);
}

void setVibrationArray(int values[6]) {
    for (int i = 0; i < 6; i++) ledcWrite(VIBRATOR_PINS[i], values[i]);
}

void updateVibrationSystem() {
    bool stepped = (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60);

    switch (vMode) {
        case 0: // Off
            setVibration(0);
            break;

        case 1: // Random on impact
            setVibration(stepped ? random(0, 255) : 0);
            if (!stepped) modeTimerTicks = 0;
            break;

        case 2: // On impact, full strength
            setVibration(stepped ? 255 : 0);
            break;

        case 3: { // Gait pattern
            if (stepped) {
                int v[] = {255, 255, 0, 255, 0, 255};
                setVibrationArray(v);
            } else if (sensorValueH > baselineH + 60) {
                int v[] = {0, 0, 0, 0, 255, 255};
                setVibrationArray(v);
            } else if (sensorValueV > baselineV + 60) {
                int v[] = {0, 255, 255, 255, 0, 0};
                setVibrationArray(v);
            } else {
                setVibration(0);
            }
            break;
        }

        case 4: // Asphalt — short sharp burst on impact
            if (stepped) {
                setVibration(modeTimerTicks < 5 ? 244 : 0);
                modeTimerTicks++;
            } else {
                modeTimerTicks = 0;
                setVibration(0);
            }
            break;

        case 5: // Gravel — random bursts
            if (stepped) {
                if (modeTimerTicks + random(15) < 15) {
                    modeTimerTicks++;
                    setVibration(ledcRead(VIBRATOR_PINS[1]) ? 255 : 0);
                } else {
                    modeTimerTicks = 0;
                    setVibration(0);
                }
            } else {
                modeTimerTicks = 0;
                setVibration(0);
            }
            break;

        case 6: // Grass — soft continuous on impact
            setVibration(stepped ? 55 : 0);
            if (!stepped) modeTimerTicks = 0;
            break;

        case 7: // Sand — medium soft on impact
            setVibration(stepped ? 70 : 0);
            if (!stepped) modeTimerTicks = 0;
            break;

        case 8: // Custom: vibrate on ground, custom strength
            setVibration(stepped ? vibrationStrength : 0);
            break;

        case 9: // Custom: vibrate in air, custom strength
            setVibration(stepped ? 0 : vibrationStrength);
            break;

        case 10: // Continuous
            setVibration(vibrationStrength);
            break;

        default:
            setVibration(0);
            break;
    }
}
