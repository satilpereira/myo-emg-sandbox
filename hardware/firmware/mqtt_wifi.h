#ifndef MQTT_WIFI_H
#define MQTT_WIFI_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setupWiFi()
{
  delay(10);
  DEBUG_PRINTLN();
  DEBUG_PRINT("Connecting to WiFi: ");
  DEBUG_PRINTLN(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected!");
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

void reconnectMQTT()
{
  while (!mqttClient.connected())
  {
    DEBUG_PRINT("Attempting MQTT connection to broker...");
    if (mqttClient.connect("ESP32_Myo_Client"))
    {
      DEBUG_PRINTLN("connected to Mosquitto!");
    }
    else
    {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(mqttClient.state());
      DEBUG_PRINTLN(" trying again in 2 seconds...");
      delay(2000);
    }
  }
}

void initMQTT()
{
  mqttClient.setServer(mqtt_server, mqtt_port);
}

void handleMQTTLogistics()
{
  if (!mqttClient.connected())
  {
    reconnectMQTT();
  }
  mqttClient.loop();
}

#endif