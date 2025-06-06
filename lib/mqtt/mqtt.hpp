#ifndef MQTT_HPP
#define MQTT_HPP

#include <WiFi.h>
#include <PubSubClient.h>
#include "../setup/setup.hpp"

extern const char *MQTT_BROKER;     // IP do broker Mosquitto
extern const int MQTT_PORT;
extern const char *MQTT_USER;       // Usuário MQTT
extern const char *MQTT_PASS;       // Senha MQTT

extern const char *MQTT_CLIENT_ID;
extern const char *MQTT_TOPIC_SUB;  // Tópico de controle

extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

extern WiFiClient espClient;
extern PubSubClient client;

void connectWiFi();
void connectMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);

#endif