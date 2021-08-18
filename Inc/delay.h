#ifndef DELAY_H
#define DELAY_H

#include "type_define.h"

void delay_init(u8 SYSCLK);
void delay_ms(u16 nms);
void delay_us(u32 nus);

#endif
