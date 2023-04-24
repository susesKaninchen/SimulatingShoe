#include "defines.h"

void TaskWebserver(void *pvParameters) {
#ifdef LEFT
    ArduinoOTA.setHostname("ShoeL");
#else
    ArduinoOTA.setHostname("ShoeR");
#endif
    print("Setze Ota NAme", DEBUG_DEBUG);
    ArduinoOTA
        .onStart([]() {
            clientDelay = 1;
            vTaskDelete(tUpload);
            vTaskDelete(tloop);
        })
        .onEnd([]() {
            clientDelay = 200;
            ESP.restart();
        });
    print("StarteOta", DEBUG_DEBUG);
    ArduinoOTA.begin();
    print("OTA Gestartet", DEBUG_DEBUG);
    server.on("/", []() {
        server.send_P(200, "text/html", index_html);
    });
    server.on(UriRegex("^\\/parameter\\/([0-9]+)\\/value\\/([0-9]+)$"), []() {
        String parameter = server.pathArg(0);
        String value = server.pathArg(1);
        if (parameter.toInt() == 0) {         // Setze Druck
            state = value.toInt() * 10;       // *10 um über 50000 zu kommen
        } else if (parameter.toInt() == 1) {  // Vibrate on GND
            vibrate = true;
            vibrationStrenth = value.toInt();
            vMode = 8;
        } else if (parameter.toInt() == 2) {  // Vibrate in Air
            vibrate = true;
            vibrationStrenth = value.toInt();
            vMode = 9;
        } else if (parameter.toInt() == 3) {  // Vibrate continues
            vibrate = true;
            vibrationStrenth = value.toInt();
            vMode = 10;
        } else if (parameter.toInt() == 4) {
        } else if (parameter.toInt() == 5) {
        }
        state = value.toInt();
        server.send(200, "text/plain", "Set " + parameter + " to " + value);
    });
    server.on(UriRegex("^\\/vibrate\\/([0-5])$"), []() {
        String parameter = server.pathArg(0);
        int value = parameter.toInt();
        server.send(200, "text/plain", "Vibrate Motor " + String(value) + "");
        vTaskDelete(tloop);
        ledcWrite(value, 255);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ledcWrite(value, 0);
        xTaskCreatePinnedToCore(TaskLoop, "TaskLoop", ramLoop, NULL, configMAX_PRIORITIES - 1, &tloop, ARDUINO_RUNNING_CORE);
        // xTaskCreatePinnedToCore(TaskUpload,  "TaskUpload",  ramUpload,  NULL,  1,  &tUpload,  ARDUINO_RUNNING_CORE);
    });
    server.on("/stop", []() {
        server.send(200, "text/plain", "Stoppe Upload und ValveSystem");
        state = 3;
        vTaskDelete(tUpload);
    });
    server.on("/start", []() {
        server.send(200, "text/plain", "Stoppe Upload und ValveSystem");
        state = 2600;
#ifdef INFLUXDB
        xTaskCreatePinnedToCore(TaskUpload, "TaskUpload", ramUpload, NULL, 1, &tUpload, ARDUINO_RUNNING_CORE);
#endif
    });
    server.on("/status", []() {
        server.send(200, "text/plain", "Die letzte Messung dauerte: " + String(lastMesurement) + "microsekunden<br>Der letzte Upload dauerte: " + String(lastUpload) + "microsekunden und hatte " + String(lastQueLen) + " Elemente.<br>Freier Heep: " + String(ESP.getFreeHeap()));
    });
    server.on("/vibrate", []() {
        vibrate = !vibrate;
        server.send(200, "text/plain", "Vibrate: " + String(vibrate));
    });
    server.on("/asphalt", []() {
        vibrate = true;
        vMode = 4;
        state = 38000;
        server.send(200, "text/plain", "Simulating Asphalt");
    });
    server.on("/grass", []() {
        vibrate = true;
        vMode = 6;
        state = 5;
        server.send(200, "text/plain", "Simulating Grass");
    });
    server.on("/sand", []() {
        vibrate = true;
        vMode = 7;
        state = 5;
        server.send(200, "text/plain", "Simulating Sand");
    });
    server.on("/lenolium", []() {
        vibrate = false;
        state = 32000;
        server.send(200, "text/plain", "Simulating Sand");
    });
    server.on("/gravel", []() {
        vibrate = true;
        vMode = 5;
        state = 34000;
        server.send(200, "text/plain", "Simulating Sand");
    });
    server.on("/evenout", []() {
        server.send(200, "text/plain", "Evenout");
        state = 2;
    });
    server.on("/restart", []() {
        server.send(200, "text/plain", "Starte Neu");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ESP.restart();
    });
    server.begin();
    for (;;) {
        server.handleClient();
        ArduinoOTA.handle();
        vTaskDelay(clientDelay / portTICK_PERIOD_MS);
    }
}
