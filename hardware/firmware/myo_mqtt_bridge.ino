#include <myo.h>
#include "config.h"
#include "mqtt_wifi.h"

armband myo;

// --- FreeRTOS Globals ---
typedef struct
{
  char payload[128];
} EMGMsg;

QueueHandle_t emgQueue;
TaskHandle_t mqttTaskHandle;

// --- SERIAL PLOTTER HELPER ---
void printEMGForPlotter(uint8_t *pData)
{
  // // 1. Print Max and Min boundaries first
  // Serial.print("127\t");
  // Serial.print("-128\t");

  // 2. Print the 8 channels separated by tabs
  for (int i = 0; i < 8; i++)
  {
    Serial.print((int8_t)pData[i]);
    if (i < 7)
    {
      Serial.print("\t"); // Bulletproof tab separator
    }
  }

  // 3. One single newline at the very end. NO trailing tabs or commas.
  Serial.println();
}

// --- CALLBACKS ---

void batteryCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  myo.battery = pData[0];
  DEBUG_PRINT("Battery updated: ");
  DEBUG_PRINTLN(myo.battery);

  char batteryStr[4];
  snprintf(batteryStr, sizeof(batteryStr), "%d", myo.battery);
  if (mqttClient.connected())
  {
    mqttClient.publish(battery_topic, batteryStr);
  }
}

void emgCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  // 1. Feed the Arduino Serial Plotter (Ignores DEBUG_MODE so it always plots)
  printEMGForPlotter(pData);

  // 2. Prepare the MQTT payload for the FreeRTOS Queue
  EMGMsg newMsg;
  newMsg.payload[0] = '\0';
  char tempStr[12];

  for (int i = 0; i < length; i++)
  {
    int8_t channelValue = (int8_t)pData[i];
    snprintf(tempStr, sizeof(tempStr), "%d", channelValue);
    strcat(newMsg.payload, tempStr);

    if (i < length - 1)
    {
      strcat(newMsg.payload, " ");
    }
  }

  // Send the formatted string to the FreeRTOS queue
  xQueueSend(emgQueue, &newMsg, 0);
}

// --- FREERTOS TASKS ---

void mqttNetworkTask(void *pvParameters)
{
  for (;;)
  {
    handleMQTTLogistics();

    EMGMsg incomingMsg;

    // BATCH REMOVED: Processing 1 item at a time directly from the queue.
    if (xQueueReceive(emgQueue, &incomingMsg, pdMS_TO_TICKS(10)) == pdPASS)
    {
      if (mqttClient.connected())
      {
        mqttClient.publish(emg_topic, incomingMsg.payload);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// --- SETUP & LOOP ---

void setup()
{
  Serial.begin(115200);

  setupWiFi();
  initMQTT();

  emgQueue = xQueueCreate(30, sizeof(EMGMsg));

  xTaskCreatePinnedToCore(
      mqttNetworkTask,
      "MQTT_Task",
      4096,
      NULL,
      1,
      &mqttTaskHandle,
      0);

  DEBUG_PRINTLN("Connecting to Myo Armband...");
  myo.connect();
  DEBUG_PRINTLN(" - Connected to Myo!");
  delay(100);

  myo.unlock(myohw_unlock_hold);
  delay(100);
  myo.vibration(myohw_vibration_short);

  myo.set_myo_mode(myohw_emg_mode_send_emg,
                   myohw_imu_mode_none,
                   myohw_classifier_mode_disabled);
  delay(200); // <-- CRITICAL: Let Myo process EMG mode

  // FORCE MYO TO NEVER SLEEP
  myo.set_sleep_mode(1);
  delay(200); // <-- CRITICAL: Let Myo process Sleep mode

  myo.emg_notification(TURN_ON)->registerForNotify(emgCallback);
  delay(200); // <-- CRITICAL: Let Myo register the EMG callback

  myo.battery_notification(TURN_ON)->registerForNotify(batteryCallback);
  delay(200); // <-- CRITICAL: Let Myo register the Battery callback
}

void loop()
{
  if (!myo.connected)
  {
    DEBUG_PRINTLN("Myo disconnected! Attempting reconnection...");
    myo.connect();
    DEBUG_PRINTLN(" - Reconnected to Myo");

    myo.set_myo_mode(myohw_emg_mode_send_emg, myohw_imu_mode_none, myohw_classifier_mode_disabled);
    delay(200); // Add delay here too

    myo.set_sleep_mode(1);
    delay(200); // Add delay here too

    myo.emg_notification(TURN_ON)->registerForNotify(emgCallback);
    delay(200); // Add delay here too

    myo.battery_notification(TURN_ON)->registerForNotify(batteryCallback);
    delay(200); // Add delay here too
  }
  delay(10);
}