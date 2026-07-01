#ifndef CONFIG_H
#define CONFIG_H

// --- DEBUG LOGGING MACROS ---
// Set to 1 to see text logs in the Serial Monitor.
// Set to 0 to mute text logs so the Arduino Serial Plotter works cleanly.
#define DEBUG_MODE 1

#if DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// --- WiFi Configuration ---
const char *ssid = "Gengarnet";
const char *password = "Fwer2800@";

// --- MQTT Broker Configuration ---
const char *mqtt_server = "192.168.18.53";
const int mqtt_port = 1883;

// --- MQTT Topics ---
const char *emg_topic = "esp32/myo/emg";
const char *battery_topic = "esp32/myo/battery";

#endif