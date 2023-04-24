#include "defines.h"
#include "http_server.h"
#include "pressureSystem.h"
#include "vibration.h"

void classifyMovement() {  //  GROUND,  UP,  DOWN,  STANDING,  UNDEFINED
    // Pressure change on Chambers
    if (sensorValueH > baselineH + 60) {
        sState = GROUND;
    } else if (sensorValueH > baselineH + 20) {
        sState = STANDING;
    } else if (false) {
        sState = UP;
    } else {
        sState = UNDEFINED;
    }
}

void TaskUpload(void *pvParameters)  // This is a task.
{
    (void)pvParameters;
    esp_task_wdt_init(30, false);
    print("Start uploadtask", DEBUG_INFO);
#ifdef INFLUXDB
    InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Links oder Rechts
#ifdef LEFT
    Point sensorStatus("L");  // L, R
#else
    Point sensorStatus("R");                    // L, R
#endif
    client.setHTTPOptions(HTTPOptions().connectionReuse(true).httpReadTimeout(2000));
    client.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE).retryInterval(0));
#endif
    SensorReading item;
    long tempUploadTime = 0;
#ifdef INFLUXDB
    for (;;) {
        int xD = 0;
        while (xQueueReceive(msg_queue, (void *)&item, 0) == pdTRUE && xD < MAX_BATCH_SIZE) {
            xD++;
            // Serial.println(item);
            sensorStatus.addField("ax", item.ax);
            sensorStatus.addField("ay", item.ay);
            sensorStatus.addField("az", item.az);
            sensorStatus.addField("gx", item.gx);
            sensorStatus.addField("gy", item.gy);
            sensorStatus.addField("gz", item.gz);
            sensorStatus.addField("pV", item.pressureFront);
            sensorStatus.addField("pH", item.pressureBack);
            sensorStatus.addField("pB", item.pressureBaseline);
            sensorStatus.setTime(item.time_stamp);
            client.writePoint(sensorStatus);
            // Serial.println(client.pointToLineProtocol(sensorStatus));
            sensorStatus.clearFields();
        }
        lastQueLen = xD;
        print(String("Start Upload von : ") + String(xD), DEBUG_DEBUG);
        tempUploadTime = micros();
        if (!client.flushBuffer()) {
            print("InfluxDB flush failed: ", DEBUG_ERROR);
            print(client.getLastErrorMessage(), DEBUG_ERROR);
            print("Full buffer: ", DEBUG_ERROR);
            print(client.isBufferFull() ? "Yes" : "No", DEBUG_ERROR);
            print("Heap Belegt: " + String(((ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize()) * 100.0) + "%", DEBUG_ERROR);
            print("Free Heap: " + String(ESP.getFreeHeap()), DEBUG_ERROR);
            print("Min Free Heap: " + String(ESP.getMinFreeHeap()), DEBUG_ERROR);
            print("Max Heap: " + String(ESP.getMaxAllocHeap()), DEBUG_ERROR);
        }
        lastUpload = micros() - tempUploadTime;
        print("END Uploadet", DEBUG_DEBUG);
    }
#endif
}

void TaskLoop(void *pvParameters)  // This is a task.
{
    (void)pvParameters;
    esp_task_wdt_init(30, false);
    print("Start Loop", DEBUG_INFO);
    long tempMessure = 0;
    struct timeval tv;  // Erstelle die Zeit
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    for (;;)  // A Task shall never return or exit.
    {
        vTaskDelayUntil(&xLastWakeTime, MESSUREMENT_SMAPLE_TIME / portTICK_PERIOD_MS);
        // Sorge für eine Konstante Messung alle 10 MS
        tempMessure = micros();   // Messe die Microsekunden die eine Messung benötigt
        gettimeofday(&tv, NULL);  // hole die aktuelle Zeit
        recordAccelRegisters();   // Hole Accelerometer Werte
        recordGyroRegisters();    // Hole Gyroscope Messwerte
        messure();                // Messe den druck der Kammern
        classifyMovement();       // Classifiziere die Bewegung
        updateValveSystem();      // Setze die Ventile
        if (vibrate) {
            updateVibrationSystem();  // Setze die Vibration
        }
#ifdef INFLUXDB
        SensorReading pointValue = {accelX, accelY, accelZ, gyroX, gyroY, gyroZ, sensorValueV, sensorValueH, baselineH, getTimeStamp(&tv, 3)};
        // Punkt wird erstellt mit allen gelesenen werten
        if (xQueueSend(msg_queue, (void *)&pointValue, 0) != pdTRUE) {
            print("Queue full", DEBUG_ERROR);
        }
#endif
        lastMesurement = micros() - tempMessure;
        print("Die Messung hat " + String(lastMesurement) + " Microsekunden gedauert", DEBUG_DEBUG);
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    Wire.setClock(400000);
    setupMPU();

#ifdef WiFi_EN
    bool res;
#ifdef LEFT
    res = wm.autoConnect("ShoeL", "shoeshoe");  // password protected ap
#else
    res = wm.autoConnect("ShoeR", "shoeshoe");  // password protected ap
#endif
    print(String(WiFi.localIP()), DEBUG_INFO);
// Sync time
#ifdef INFLUXDB
    timeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
#endif
#endif
    print("time Fertig", DEBUG_INFO);
    // Ensure memory allocation was successful
    if (queueStorageArea == NULL || queueDataStructure == NULL) {
        print("Failed to allocate memory for the queue in (PS)RAM", DEBUG_ERROR);
        while (1)
            ;  // Halt execution
    }
    msg_queue = xQueueCreateStatic(msg_queue_len, sizeof(SensorReading), queueStorageArea, queueDataStructure);
    // Check if the queue creation was successful
    if (msg_queue == NULL) {
        print("Failed to create the queue", DEBUG_ERROR);
        while (1)
            ;  // Halt execution
    } else {
        print("Queue successfully created in (PS)RAM", DEBUG_INFO);
    }
    // msg_queue = xQueueCreate(msg_queue_len, sizeof(SensorReading));
    print("Queue Fertig", DEBUG_INFO);
    init_vibration();
    init_pressureSystem();
    print("pins Fertig", DEBUG_INFO);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    // messure();// Remove inductive nois
    print("mesure2 Fertig", DEBUG_INFO);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    calibrate();
    print("callibarate Fertig", DEBUG_INFO);
    xTaskCreatePinnedToCore(TaskLoop, "TaskLoop", ramLoop, NULL, configMAX_PRIORITIES - 1, &tloop, ARDUINO_RUNNING_CORE);
    print("task1 Fertig", DEBUG_INFO);
#ifdef INFLUXDB
    vTaskDelay(10 / portTICK_PERIOD_MS);
    xTaskCreatePinnedToCore(TaskUpload, "TaskUpload", ramUpload, NULL, 1, &tUpload, ARDUINO_RUNNING_CORE);
#endif
    print("task2 Fertig", DEBUG_INFO);
#ifdef WiFi_EN
    xTaskCreatePinnedToCore(TaskWebserver, "TaskWebserver", ramWebservice, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
#endif
    print("Webserver Fertig", DEBUG_INFO);
}

void loop() {
    esp_task_wdt_init(30, false);
    vTaskDelete(NULL);
}
