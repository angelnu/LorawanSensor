#include "config.h"
#if defined(ARDUINO_ARCH_STM32)

#define ADC_RANGE (1<<ADC_RESOLUTION)

// Average
extern uint32_t averageAnalogInput(uint32_t pin, uint8_t measurements);

// VREF
extern uint32_t readVref();

// TEMPERATURE
extern float readTempSensor(int32_t VRef);

//Set all as inputs
void allInput();

#endif