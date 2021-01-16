#ifndef lorawan_h
#define lorawan_h

#include "Arduino.h"

void lorawan_setup();
void lorawan_send(uint8_t* buffer, uint8_t len, uint8_t port=1);
void lorawan_suspend();
void lorawan_resume();

#endif //lorawan_h
