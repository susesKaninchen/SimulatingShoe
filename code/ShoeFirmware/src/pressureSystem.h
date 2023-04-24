#include "defines.h"

void init_pressureSystem() {
    pinMode(zuluftHPin, OUTPUT);    // ZuluftHacke
    pinMode(verbinderPin, OUTPUT);  // VerbinderV
    pinMode(zuluftVPin, OUTPUT);    // ZuluftV
    digitalWrite(zuluftHPin, LOW);
    digitalWrite(verbinderPin, LOW);
    digitalWrite(zuluftVPin, LOW);
}

void messure() {
    // sensorValueG = analogRead(sensorGPin);
    sensorValueH = analogRead(sensorHPin) - sensorValueG;
    sensorValueV = analogRead(sensorVPin) - diff - sensorValueG;
    baselineH = baselineH * 0.990 + sensorValueH * 0.010;
    baselineV = baselineV * 0.990 + sensorValueV * 0.010;
}

void calibrate() {
    // Open Valves
    digitalWrite(zuluftHPin, HIGH);
    digitalWrite(zuluftVPin, HIGH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    digitalWrite(zuluftHPin, LOW);
    digitalWrite(zuluftVPin, LOW);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    int MeasurementsToAverage = 20;  // Anzahl der in den Mettelwert aufgenommenen Messungen
    sensorValueH = 0;
    sensorValueV = 0;
    // sensorValueG = 0;
    for (int i = 0; i < MeasurementsToAverage; ++i) {
        sensorValueH += analogRead(sensorHPin);
        sensorValueV += analogRead(sensorVPin);
        // sensorValueG += analogRead(sensorGPin);
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    sensorValueH /= MeasurementsToAverage;
    sensorValueV /= MeasurementsToAverage;
    sensorValueG /= MeasurementsToAverage;
    diff = sensorValueV - sensorValueH;
    base = sensorValueH;
    baselineH = base;
    baselineV = base;
    startHigh = millis();
    startLow = millis();
    laststartHigh = millis();
    laststartLow = millis();
}

void pumpFront() {
    if (sensorValueH < base - threshold) {
        digitalWrite(zuluftHPin, HIGH);  // ZuluftHacke
    } else {
        digitalWrite(zuluftHPin, LOW);  // ZuluftHacke
    }
    if (sensorValueV < base - threshold) {
        digitalWrite(zuluftVPin, HIGH);  // ZuluftHacke
    } else {
        digitalWrite(zuluftVPin, LOW);  // ZuluftHacke
    }
    if (sensorValueV < sensorValueH - threshold) {  // von leiner Kammer in große Kammer
        digitalWrite(verbinderPin, HIGH);
    } else {
        digitalWrite(verbinderPin, LOW);
    }
}

void pumpBack() {
    if (sensorValueH < base - threshold) {
        digitalWrite(zuluftHPin, HIGH);  // ZuluftHacke
    } else {
        digitalWrite(zuluftHPin, LOW);  // ZuluftHacke
    }
    if (sensorValueV < base - threshold) {
        digitalWrite(zuluftVPin, HIGH);  // ZuluftHacke
    } else {
        digitalWrite(zuluftVPin, LOW);  // ZuluftHacke
    }
    if (sensorValueH < sensorValueV - threshold) {  // von leiner Kammer in große Kammer
        digitalWrite(verbinderPin, HIGH);
    } else {
        digitalWrite(verbinderPin, LOW);
    }
}

void evenOut(int millisecends) {
    if (millisecends > 0) {
        if (valveTimer == 0) {
            valveTimer = millis();
        } else {
            if (valveTimer + millisecends < millis()) {
                state = 3;
                valveTimer = 0;
            }
        }
    }
    digitalWrite(verbinderPin, HIGH);
    digitalWrite(zuluftHPin, LOW);
    digitalWrite(zuluftVPin, LOW);
}

void holdValue(int value) {
    if (sensorValueV < value) {
        if (sensorValueV < sensorValueH - threshold) {  // von leiner Kammer in große Kammer
            digitalWrite(verbinderPin, HIGH);
        } else {
            digitalWrite(verbinderPin, LOW);
        }
    } else {
        digitalWrite(zuluftHPin, LOW);
        digitalWrite(verbinderPin, LOW);
        digitalWrite(zuluftVPin, LOW);
    }
    if (sensorValueH < base - threshold) {
        digitalWrite(zuluftHPin, HIGH);  // ZuluftHacke
    } else {
        digitalWrite(zuluftHPin, LOW);  // ZuluftHacke
    }
    if (sensorValueV < base - threshold) {
        digitalWrite(zuluftVPin, HIGH);  // ZuluftHacke
    } else {
        digitalWrite(zuluftVPin, LOW);  // ZuluftHacke
    }
}

void setValue(int value) {
    if (sensorValueV < value) {
        if (sensorValueV < sensorValueH - threshold) {  // von leiner Kammer in große Kammer
            digitalWrite(verbinderPin, HIGH);
        } else {
            digitalWrite(verbinderPin, LOW);
        }
    } else {
        state = 2;  // Even Out
    }
    if (sensorValueH < base - threshold) {
        digitalWrite(zuluftHPin, HIGH);  // ZuluftHacke
    } else {
        digitalWrite(zuluftHPin, LOW);  // ZuluftHacke
    }
    if (sensorValueV < base - threshold) {
        digitalWrite(zuluftVPin, HIGH);  // ZuluftFront
    } else {
        digitalWrite(zuluftVPin, LOW);  // ZuluftFront
    }
}

void updateValveSystem() {
    if (state == 0) {  // PUMP_FRONT
        pumpFront();
    } else if (state == 1) {  // PUMP_BACK
        pumpBack();
    } else if (state == 2) {  // EVEN_OUT
        evenOut(1000);
    } else if (state == 3) {  // PAUSE
        digitalWrite(zuluftHPin, LOW);
        digitalWrite(verbinderPin, LOW);
        digitalWrite(zuluftVPin, LOW);
    } else if (state == 4) {  // OPEN
        digitalWrite(zuluftHPin, HIGH);
        digitalWrite(verbinderPin, LOW);
        digitalWrite(zuluftVPin, HIGH);
    } else if (state == 5) {  // Evenout vorever
        evenOut(0);
    } else if (state == 6) {  // CALLIBRATE
        calibrate();
        state = 3;
    } else if (state == 7) {  //

    } else if (state < 5000) {  // Go To Value FRONT
        holdValue(state);
    } else {
        setValue(state * 0.1);
    }
    cc++;
    if (cc == 1000) {
        cc = 0;
    }
}

void setupMPU() {
    Wire.beginTransmission(0b1101000);  // This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
    Wire.write(0x6B);                   // Accessing the register 6B - Power Management (Sec. 4.28)
    Wire.write(0b00000000);             // Setting SLEEP register to 0. (Required; see Note on p. 9)
    Wire.endTransmission();
    Wire.beginTransmission(0b1101000);  // I2C address of the MPU
    Wire.write(0x1B);                   // Accessing the register 1B - Gyroscope Configuration (Sec. 4.4)
    Wire.write(0x00011000);             // Setting the gyro to full scale +/- 500deg./s
    /*
      0 = 250 °/s
      1 = 500 °/s
      2 = 1000 °/s
      3 = 2000 °/s
    */
    Wire.endTransmission();
    Wire.beginTransmission(0b1101000);  // I2C address of the MPU
    Wire.write(0x1C);                   // Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5)
    Wire.write(0b00001000);             // Setting the accel to +/- 2g
    /*
      0 ± 2g
      1 ± 4g
      2 ± 8g
      3 ± 16g
     */
    Wire.endTransmission();
}

void recordAccelRegisters() {
    Wire.beginTransmission(0b1101000);  // I2C address of the MPU
    Wire.write(0x3B);                   // Starting register for Accel Readings
    Wire.endTransmission();
    Wire.requestFrom(0b1101000, 6);  // Request Accel Registers (3B - 40)
    while (Wire.available() < 6)
        ;
    accelX = Wire.read() << 8 | Wire.read();  // Store first two bytes into accelX
    accelY = Wire.read() << 8 | Wire.read();  // Store middle two bytes into accelY
    accelZ = Wire.read() << 8 | Wire.read();  // Store last two bytes into accelZ
    // processAccelData();
}  // gForceX = accelX / 16384.0;

void recordGyroRegisters() {
    Wire.beginTransmission(0b1101000);  // I2C address of the MPU
    Wire.write(0x43);                   // Starting register for Gyro Readings
    Wire.endTransmission();
    Wire.requestFrom(0b1101000, 6);  // Request Gyro Registers (43 - 48)
    while (Wire.available() < 6)
        ;
    gyroX = Wire.read() << 8 | Wire.read();  // Store first two bytes into accelX
    gyroY = Wire.read() << 8 | Wire.read();  // Store middle two bytes into accelY
    gyroZ = Wire.read() << 8 | Wire.read();  // Store last two bytes into accelZ
    // processGyroData();
}  //  rotX = gyroX / 131.0;
