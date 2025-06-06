#include "setup.hpp"

void led_setup() {
    pinMode(RED, OUTPUT);
    digitalWrite(RED, LOW);
}

void btn_setup() {
    pinMode(BTN, INPUT_PULLUP);
}