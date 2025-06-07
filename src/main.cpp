#include <Arduino.h>

#include "../lib/setup/setup.hpp"
#include "../lib/mqtt/mqtt.hpp"
#include "../lib/mlp/mlp.h"

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
float getEnvironmentalConditions();
void trained_mlp_model(MLP* mlp);

MLP mlp;
float temperature, humidity, luminosity, gas_level, environment;

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
    
    trained_mlp_model(&mlp);
}

void loop()
{
    temperature = getTemperature();
    humidity = getHumidity();
    luminosity = getLuminosity();
    gas_level = getGasLevel();
    environment = getEnvironmentalConditions();

    if(!stop) {
        if(flag500ms) {
            flag500ms = false;
            Serial.printf("TEMP: %f - HUMID: %f - GAS: %f - LUX: %f - ENVIRONMENT: %f\n\n", temperature, humidity, gas_level, luminosity, environment);
        }
        if(flag2s) {
            flag2s = false;
            client.publish("/temperature", String(temperature).c_str());
            client.publish("/humidity", String(humidity).c_str());
            client.publish("/luminosity", String(luminosity).c_str());
            client.publish("/gas", String(gas_level).c_str());
            client.publish("/environment", (environment >= 80.0)?"Propicio a vida":(environment <= 80.0 && environment >= 50.0)?"Moderado":"Hostil");
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


void IRAM_ATTR _500ms_timer() 
{
    flag500ms = true;
}

void IRAM_ATTR _2s_timer() 
{
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

float getEnvironmentalConditions()
{
    float X[4] = {temperature, humidity, gas_level, luminosity};
    forward(&mlp, X);
    return mlp.output_layer_outputs[0];
}

void trained_mlp_model(MLP* mlp) {
    float hidden_layer_weights[10][4+1] = {
        {7.449806, 0.181137, -0.472057, 0.046361, -4.857895},
        {-0.751881, 0.020071, 0.844090, 0.566661, 0.962430},
        {0.505790, 0.455488, 0.106521, 0.293246, 0.760718},
        {-0.001633, -3.484349, 0.277633, 1.023582, 1.005119},
        {-0.140420, 0.863770, 0.368107, 0.089362, 0.914042},
        {0.917888, 0.164913, 0.371158, -0.063512, 0.245188},
        {0.919015, -0.197816, 0.266934, 3.914030, -0.640058},
        {1.040644, 0.224041, 0.266534, 0.843221, 0.334403},
        {2.815262, -0.603506, 0.403068, -1.971833, 0.741603},
        {-1.032765, 0.608852, 1.006948, 0.453830, 0.756457}
    };
    float output_layer_weights[1][10+1] = {{-2.133985, -1.103779, -0.154775, -1.754287, -0.437949, 0.376591, 2.915985, 0.477521, 2.537050, -1.424970, -0.486438}};

    mlp->hidden_layer_weights = (float**)malloc(10 * sizeof(float*));

	for(int i = 0; i < 10; i++) {
		mlp->hidden_layer_weights[i] = (float*)malloc((4 + 1) * sizeof(float));
		for(int j = 0; j < (3 + 1); j++) {
			mlp->hidden_layer_weights[i][j] = hidden_layer_weights[i][j];
		}
	}

	mlp->output_layer_weights = (float**)malloc(1 * sizeof(float*));

	for(int i = 0; i < 1; i++) {
		mlp->output_layer_weights[i] = (float*)malloc((10 + 1) * sizeof(float));
		for(int j = 0; j < (10 + 1); j++) {
			mlp->output_layer_weights[i][j] = output_layer_weights[i][j];
		}
	}
}