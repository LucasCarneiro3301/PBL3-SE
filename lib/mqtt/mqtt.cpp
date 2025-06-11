#include "mqtt.hpp"

const char *MQTT_BROKER = "000.000.000.000";    // IP do broker Mosquitto
const int MQTT_PORT = 1883;                     // Porta do broker
const char *MQTT_USER = "--";                   // Usuário MQTT
const char *MQTT_PASS = "--";                   // Senha MQTT

const char *MQTT_CLIENT_ID = "esp32_control";
const char *MQTT_TOPIC_SUB = "/control";        // Tópico de controle

const char* WIFI_SSID = "--";
const char* WIFI_PASSWORD = "--";


WiFiClient espClient;
PubSubClient client(espClient);

// Conecta ao Wi-Fi
void connectWiFi()
{
    Serial.printf("Conectando-se ao WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// Conecta ao broker MQTT
void connectMQTT()
{
    while (!client.connected())
    {
        Serial.println("Conectando ao broker MQTT...");
        if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS))
        {
            Serial.println("Conectado ao MQTT!");
            client.subscribe(MQTT_TOPIC_SUB);
            Serial.print("Inscrito no tópico: ");
            Serial.println(MQTT_TOPIC_SUB);
        }
        else
        {
            Serial.print("Falha, rc=");
            Serial.print(client.state());
            Serial.println(" tentando novamente em 2 segundos...");
            delay(2000);
        }
    }
}


// Callback de mensagem MQTT
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    String msg;

    for (int i = 0; i < length; i++)
    {
        msg += (char)payload[i];
    }

    Serial.printf("Recebido no tópico %s: %s\n", topic, msg.c_str());

    if (msg.equalsIgnoreCase("On"))
    {
        stop = true;
        Serial.printf("PARADA SOLICITADA!!!\n\n");
        client.publish("/status", "PARADO");
        digitalWrite(RED, true);
    }
    else if (msg.equalsIgnoreCase("Off"))
    {
        stop = false;
        Serial.printf("REINICIO SOLICITADO!!!\n\n");
        client.publish("/status", "ATIVO");
        digitalWrite(RED, false);
    }
}