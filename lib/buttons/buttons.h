#ifndef BUTTONS_H
#define BUTTONS_H

#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "pico/time.h"

#define BTN_A 5
#define BTN_B 6

extern volatile int8_t offset;
extern volatile bool lock_humidity;

void init_buttons();
void gpio_irq_handler(uint gpio, uint32_t events);

#endif