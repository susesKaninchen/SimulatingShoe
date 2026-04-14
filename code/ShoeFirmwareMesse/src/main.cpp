#include "defines.h"
#include "pressureSystem.h"
#include "vibration.h"
#include "web_ui.h"

// ── Step classification ───────────────────────────────────────────────────────
static void classifyMovement() {
    if      (sensorValueH > baselineH + 60) sState = GROUND;
    else if (sensorValueH > baselineH + 20) sState = STANDING;
    else                                     sState = UNDEFINED_STEP;
}

// ── Sensor + control loop (Core 0, high priority) ────────────────────────────
void TaskLoop(void *pvParameters) {
    (void)pvParameters;
    print("TaskLoop started", DEBUG_INFO_LVL);

    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWake, MEASUREMENT_SAMPLE_TIME / portTICK_PERIOD_MS);
        long t0 = micros();

        recordAccelRegisters();
        recordGyroRegisters();
        messure();
        classifyMovement();
        updateValveSystem();
        if (vibrate) updateVibrationSystem();

        ChartSample cs = {
            (int16_t)(sensorValueH - (int)baselineH),
            (int16_t)(sensorValueV - (int)baselineV),
            (int16_t)(accelX / 100),
            (int16_t)(accelY / 100),
            (int16_t)(accelZ / 100)
        };
        chartBufPush(cs);

        lastMeasurement = micros() - t0;
    }
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Wire.begin();
    Wire.setClock(400000);

    setupMPU();
    print("MPU ready", DEBUG_INFO_LVL);

    // ── WiFi ──────────────────────────────────────────────────────────────────
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.printf("Connecting to '%s'", WIFI_SSID);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nConnected!  IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        // Continue without WiFi — sensor loop still works, useful for debugging
        Serial.println("\nWiFi not available — running headless");
    }

    // Keep WiFi connected automatically (ESP-IDF handles reconnect by default)
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);  // don't write credentials to flash on every connect

    // ── Hardware init ─────────────────────────────────────────────────────────
    init_vibration();
    init_pressureSystem();
    print("Hardware ready", DEBUG_INFO_LVL);

    vTaskDelay(50 / portTICK_PERIOD_MS);
    calibrate();
    print("Calibration done", DEBUG_INFO_LVL);

    // ── FreeRTOS tasks ────────────────────────────────────────────────────────
    xTaskCreatePinnedToCore(
        TaskLoop, "TaskLoop",
        ramLoop, NULL, configMAX_PRIORITIES - 1,
        &tloop, ARDUINO_RUNNING_CORE
    );

    if (WiFi.status() == WL_CONNECTED) {
        xTaskCreatePinnedToCore(
            TaskWebserver, "TaskWebserver",
            ramWebservice, NULL, 1,
            NULL, ARDUINO_RUNNING_CORE
        );
        Serial.printf("Dashboard: http://%s/\n", WiFi.localIP().toString().c_str());
    }

    print("Setup complete", DEBUG_INFO_LVL);
}

// Arduino loop() is not used — all work is in FreeRTOS tasks
void loop() {
    vTaskDelete(NULL);
}
