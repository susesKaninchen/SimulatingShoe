#include "defines.h"

void init_vibration() {
    ledcSetup(0, PWM_FREQ, PWM_RES);
    ledcAttachPin(Vibrator0, 0);
    ledcWrite(0, 0);
    ledcSetup(1, PWM_FREQ, PWM_RES);
    ledcAttachPin(Vibrator1, 1);
    ledcWrite(1, 0);
    ledcSetup(2, PWM_FREQ, PWM_RES);
    ledcAttachPin(Vibrator2, 2);
    ledcWrite(2, 0);
    ledcSetup(3, PWM_FREQ, PWM_RES);
    ledcAttachPin(Vibrator3, 3);
    ledcWrite(3, 0);
    ledcSetup(4, PWM_FREQ, PWM_RES);
    ledcAttachPin(Vibrator4, 4);
    ledcWrite(4, 0);
    ledcSetup(5, PWM_FREQ, PWM_RES);
    ledcAttachPin(Vibrator5, 5);
    ledcWrite(5, 0);
}

void setVibration(int value) {
    for (int count = 0; count < 6; count++) {
        ledcWrite(count, value);
    }
}

void setVibration(int values[]) {
    for (int count = 0; count < 6; count++) {
        ledcWrite(count, values[count]);
    }
}

void updateVibrationSystem() {
    if (vMode == 0) {  // Aus
        setVibration(0);
    } else if (vMode == 1) {  // Random
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            setVibration(random(0, 255));
        } else {
            setVibration(0);
        }
    } else if (vMode == 2) {  // Beim Auftreten
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            setVibration(255);
        } else {
            setVibration(0);
        }
    } else if (vMode == 3) {  // entlang des ganges
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            int values[] = {255, 255, 0, 255, 0, 255};
            setVibration(values);
        } else if (sensorValueH > baselineH + 60) {
            int values[] = {0, 0, 0, 0, 255, 255};
            setVibration(values);
        } else if (sensorValueV > baselineV + 60) {
            int values[] = {0, 255, 255, 255, 0, 0};
            setVibration(values);
        } else {
            setVibration(0);
        }
    } else if (vMode == 4) {  // Simulate Asphalt
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            if (modeTimerTicks < 5) {
                modeTimerTicks++;
                setVibration(244);
            } else {
                setVibration(0);
            }
        } else {
            modeTimerTicks = 0;
            setVibration(0);
        }
    } else if (vMode == 5) {  // Simulate Schotter
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            if (modeTimerTicks + random(15) < 15) {
                modeTimerTicks++;
                if (ledcRead(1)) {
                    setVibration(255);
                } else {
                    modeTimerTicks = 0;
                    setVibration(0);
                }
            }
        } else {
            modeTimerTicks = 0;
            setVibration(0);
        }
    } else if (vMode == 6) {  // Simulate Gras
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            // if (!(modeTimerTicks%(10 + random(10)))) {
            //   modeTimerTicks++;
            //   if (ledcRead(1)) {
            setVibration(55);
            //  }else {
            //    setVibration(0);
            //  }
        } else {
            modeTimerTicks = 0;
            setVibration(0);
        }
    } else if (vMode == 7) {  // Simulate Sand
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            // if (!(modeTimerTicks%(10 + random(10)))) {
            //   modeTimerTicks++;
            //   if (ledcRead(1)) {
            setVibration(70);
            //  }else {
            //    setVibration(0);
            //  }
        } else {
            modeTimerTicks = 0;
            setVibration(0);
        }
    } else if (vMode == 8) {  // Vibrate on Ground X
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            // if (!(modeTimerTicks%(10 + random(10)))) {
            //   modeTimerTicks++;
            //   if (ledcRead(1)) {
            setVibration(vibrationStrenth);
            //  }else {
            //    setVibration(0);
            //  }
        } else {
            modeTimerTicks = 0;
            setVibration(0);
        }
    } else if (vMode == 9) {  // Vibrate in Air
        if (sensorValueH > baselineH + 60 || sensorValueV > baselineV + 60) {
            // if (!(modeTimerTicks%(10 + random(10)))) {
            //   modeTimerTicks++;
            //   if (ledcRead(1)) {
            //     setVibration(70);
            //   }else {
            setVibration(0);
            modeTimerTicks = 0;
            //  }
        } else {
            setVibration(vibrationStrenth);
            modeTimerTicks = 0;
        }
    } else if (vMode == 10) {  // Vibrate immer
        setVibration(vibrationStrenth);
    }
    /*if (sState == STANDING) {
        setVibration(150);
    }else if (sState == GROUND) {
        setVibration(255);
    }else {
        setVibration(0);
    }*/
}
