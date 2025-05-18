#ifndef _BSP_KEY_H
#define _BSP_KEY_H

#include "stm32f4xx.h"

extern uint8_t key_short_press;
extern uint8_t key_long_press;

int key_init(void);

#endif