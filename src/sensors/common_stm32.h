#include "config.h"
#if defined(ARDUINO_ARCH_STM32)
#include <CayenneLPP.h>

#define ADC_RANGE (1<<ADC_RESOLUTION)
#define DEFAULT_MIN_PERCENTAGE_V_2_SEND 1;
#define MAX_VOLTAGE_V 3.3
#define DEFAULT_MIN_PERCENTAGE_T_2_SEND 10;
#define MAX_TEMP_C 100.0

// Average
extern uint32_t averageAnalogInput(uint32_t pin, uint8_t measurements);

// VREF
extern uint32_t readVref();

// TEMPERATURE
extern float readTempSensor(int32_t VRef);

//Set all as inputs
void allInput();

bool measure_stm32();

void send_stm32(CayenneLPP& lpp);

#endif