#include <Arduino.h>

#include "../lib/setup/setup.hpp"
#include "../lib/mqtt/mqtt.hpp"

hw_timer_t *timer500ms = NULL;
hw_timer_t *timer2s = NULL;

unsigned long last_time = 0;
volatile bool flag = false, flag500ms = false, flag2s = false;
bool stop = false;
DHTesp dht;

void IRAM_ATTR _500ms_timer();
void IRAM_ATTR _2s_timer();
void IRAM_ATTR gpio_handler();
float getTemperature(void);
float getHumidity(void);
float getLuminosity(void);
float getGasLevel(void);

void setup()
{
    Serial.begin(BAUD_RATE);

    led_setup();
    btn_setup();

    attachInterrupt(digitalPinToInterrupt(BTN), gpio_handler, FALLING);

    timer500ms = timerBegin(0, 80, true);

    timerAttachInterrupt(timer500ms, &_500ms_timer, true);
    timerAlarmWrite(timer500ms, 5e5, true);
    timerAlarmEnable(timer500ms);

    timer2s = timerBegin(1, 80, true);

    timerAttachInterrupt(timer2s, &_2s_timer, true);
    timerAlarmWrite(timer2s, 2e6, true);
    timerAlarmEnable(timer2s);


    dht.setup(DHT, DHTesp::DHT11);

    connectWiFi();

    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(mqttCallback);
}

void loop()
{
    if(!stop) {
        if(flag500ms) {
            flag500ms = false;
            Serial.printf("TEMP: %f - HUMID: %f - LUX: %f - GAS: %f\n\n", getTemperature(), getHumidity(), getLuminosity(), getGasLevel());
        }
        if(flag2s) {
            flag2s = false;
            client.publish("/temperature", String(getTemperature()).c_str());
            client.publish("/humidity", String(getHumidity()).c_str());
            client.publish("/luminosity", String(getLuminosity()).c_str());
            client.publish("/gas", String(getGasLevel()).c_str());
        }
    } 
    
    if(flag) {
        flag = false;
        client.publish("/status", (stop)?"PARADO":"ATIVO");
    }

    if (!client.connected())
    {
        connectMQTT();
    }

    client.loop();

    sys_delay_ms(100);
}


void IRAM_ATTR _500ms_timer() {
  flag500ms = true;
}

void IRAM_ATTR _2s_timer() {
  flag2s = true;
}

void IRAM_ATTR gpio_handler()
{
    unsigned long now = millis();

    if (now - last_time > 200)
    {
        stop = !stop;
        last_time = now;
        flag = true;

        if(stop) {
            Serial.printf("PARADA SOLICITADA!!!\n\n");
            digitalWrite(RED, true);
        }
        else {
            Serial.printf("REINICIO SOLICITADO!!!\n\n");
            digitalWrite(RED, false);  
        }
    }
}

float getTemperature(void)
{
    TempAndHumidity data = dht.getTempAndHumidity();

    if (!(isnan(data.temperature)))
        return data.temperature;
    else
        return -99.99;
}

float getHumidity(void)
{
    TempAndHumidity data = dht.getTempAndHumidity();

    if (!(isnan(data.humidity)))
        return data.humidity;
    else
        return -99.99;
}

float getLuminosity(void)
{
    uint16_t adc = analogRead(LDR);

    float vout = (adc * 3.3/4095);
    
    float rldr = (10000 * (3.3 - vout))/((vout)?vout:1e-3); 

    float lux = 2255000 * pow(rldr, -1);  // Empiricamente

    return lux;
}

float getGasLevel(void)
{
    int adc = analogRead(GAS);

    return (adc * 3.3/4095);
}