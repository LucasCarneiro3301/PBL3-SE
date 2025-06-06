#ifndef SETUP_HPP
#define SETUP_HPP

#include <DHTesp.h>

#define BAUD_RATE 115200

#define RED 18                          // LED no pino GPIO 18
#define BTN 19                          // BTN no pino GPIO 19
#define DHT 5
#define LDR 32
#define GAS 33

extern DHTesp dht;

void led_setup();
void btn_setup();

extern bool stop;

#endif