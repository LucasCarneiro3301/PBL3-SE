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

    if (!client.connected())
    {
        connectMQTT();
    }

    client.publish("/status", "ATIVO");
    
    trained_mlp_model(&mlp);
}

void loop()
{
    temperature = getTemperature();
    humidity = getHumidity();
    luminosity = getLuminosity();
    gas_level = getGasLevel();
    environment = 100*getEnvironmentalConditions();

    if(!stop) {
        if(flag500ms) {
            flag500ms = false;
            Serial.printf("TEMP: %f - HUMID: %f - GAS: %f - LUX: %f - ENVIRONMENT: %f\n\n", temperature, humidity, gas_level, luminosity, environment);
        }
        if(flag2s) {
            flag2s = false;
            client.publish("/temperature", String(getTemperature()).c_str());
            client.publish("/humidity", String(getHumidity()).c_str());
            client.publish("/luminosity", String(getLuminosity()).c_str());
            client.publish("/gas", String(getGasLevel()).c_str());
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

    return (adc * 100/4095);
}

float getEnvironmentalConditions()
{
    float xMin[4] = {5.793664, 20.596113, 9.414636, 160.582703};
    float xMax[4] = {41.263657, 81.931076, 217.787125, 1086.463989};
    float X[4] = {temperature, humidity, gas_level, luminosity};

    for (int j = 0; j < 4; j++) {
		X[j] = (X[j] - xMin[j]) / (xMax[j] - xMin[j]);
	}

    forward(&mlp, X);
    return mlp.output_layer_outputs[0];
}

void trained_mlp_model(MLP* mlp) {
    mlp->input_layer_length = 4;
    mlp->hidden_layer_length = 10;
    mlp->output_layer_length = 1;

    float hidden_layer_weights[mlp->hidden_layer_length][mlp->input_layer_length+1] = {
        {-0.356924, 0.618732, 0.670465, -0.049423, 0.756215},
        {4.530966, -0.387522, 0.228863, 2.415418, -1.799217},
        {-0.117189, 0.276763, -0.317315, 0.320550, 0.900807},
        {-0.181342, -0.323829, 0.522486, 0.247581, 1.433480},
        {0.296701, 0.579473, -0.417481, -0.337416, 0.755573},
        {1.013583, -0.288321, 0.303478, -3.889414, 2.723397},
        {-0.254679, 0.667375, 0.640776, -0.064995, 0.631764},
        {0.090767, 0.797015, 0.565025, -0.054839, 1.045970},
        {-5.436245, -0.334825, 0.278600, 1.523835, 3.057570},
        {-0.339902, 4.057984, -0.080821, -0.298409, -1.539995}
    };
    float output_layer_weights[mlp->output_layer_length][mlp->hidden_layer_length+1] = {{-1.278173, 2.497450, -0.980342, -1.152672, -0.523618, 2.293101, -1.104998, -1.021778, 2.461326, 1.786587, -1.164977}};

    mlp->hidden_layer_weights = (float**)malloc(mlp->hidden_layer_length * sizeof(float*));

	for(int i = 0; i < mlp->hidden_layer_length; i++) {
		mlp->hidden_layer_weights[i] = (float*)malloc((mlp->input_layer_length + 1) * sizeof(float));
		for(int j = 0; j < (mlp->input_layer_length + 1); j++) {
			mlp->hidden_layer_weights[i][j] = hidden_layer_weights[i][j];
		}
	}

	mlp->output_layer_weights = (float**)malloc(1 * sizeof(float*));

	for(int i = 0; i < mlp->output_layer_length; i++) {
		mlp->output_layer_weights[i] = (float*)malloc((mlp->hidden_layer_length + 1) * sizeof(float));
		for(int j = 0; j < (mlp->hidden_layer_length + 1); j++) {
			mlp->output_layer_weights[i][j] = output_layer_weights[i][j];
		}
	}

    mlp->hidden_layer_outputs = (float*) malloc(mlp->hidden_layer_length * sizeof(float));
    mlp->output_layer_outputs = (float*) malloc(mlp->output_layer_length * sizeof(float));
}