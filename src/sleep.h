#ifndef sleep_h
#define sleep_h

#include "arduino.h"

const size_t SLEEP_MODE_DEEPSLEEP = 0;
const size_t SLEEP_MODE_DEEPSLEEP_WITH_AC = 1;
const size_t SLEEP_MODE_PRECISE = 2;

void sleep_setup();

void do_sleep(uint32_t sleepTime, size_t mode = SLEEP_MODE_DEEPSLEEP);

void loop_periodically(uint32_t ms, void (&loop_work)());

//Skip next sleep if more work is needed
void skip_sleep();
bool is_skip_sleep();

#endif
