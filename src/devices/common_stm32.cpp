#if defined(ARDUINO_ARCH_STM32)

#include "common_stm32.h"
#include "sensor.h"

#define ADC_RANGE (1<<ADC_RESOLUTION)

// Average
uint32_t averageAnalogInput(uint32_t pin, uint8_t measurements) {
  uint32_t total = 0;
  for (uint8_t i=0; i<measurements; i++){
    total += analogRead(pin);
  }
  return total / measurements;

}

// VREF

//See https://github.com/stm32duino/Arduino_Core_STM32/pull/539
const uint16_t* const ADC_VREFINT_3V0_30C =  reinterpret_cast<uint16_t*>(ADC_VREFINT_3V0_30C_ADDR);
const uint32_t CALIBRATION_REFERENCE_VOLTAGE = 3000;
const float VREFINT = CALIBRATION_REFERENCE_VOLTAGE * (*ADC_VREFINT_3V0_30C) / ADC_RANGE;

uint32_t readVref()
{
  return (VREFINT * ADC_RANGE / averageAnalogInput(AVREF, device_config.measure_average)); // ADC sample to mV
}


// TEMPERATURE
#ifdef ADC_TEMP_3V0_30C_ADDR
    // see datasheet for position of the calibration values, this is for STM32L4xx
    const uint16_t* const ADC_TEMP_3V0_30C =  reinterpret_cast<uint16_t*>(ADC_TEMP_3V0_30C_ADDR);
    const uint16_t* const ADC_TEMP_3V0_130C =  reinterpret_cast<uint16_t*>(ADC_TEMP_3V0_130C_ADDR);


    float readTempSensor(int32_t VRef)
    {
    // scale constants to current reference voltage
    float adcCalTemp30C = static_cast<float>(*ADC_TEMP_3V0_30C);
    float adcCalTemp130C = static_cast<float>(*ADC_TEMP_3V0_130C);

    //Read and convert to calibration value
    float adcTempValue = 1.0 * averageAnalogInput(ATEMP, device_config.measure_average) * VRef / CALIBRATION_REFERENCE_VOLTAGE;

    return (adcTempValue - adcCalTemp30C)/(adcCalTemp130C - adcCalTemp30C) * (130.0 - 30.0) + 30.0;
    }
#else
  float readTempSensor(int32_t VRef) {return 0;}
#endif


void allInput()
{
  pinMode(PA0, INPUT_PULLDOWN);
  pinMode(PA1, INPUT_PULLDOWN);
  //Serial port
  //pinMode(PA2, INPUT_PULLDOWN);
  //pinMode(PA3, INPUT_PULLDOWN);
  pinMode(PA4, INPUT_PULLDOWN);
  pinMode(PA5, INPUT_PULLDOWN);
  pinMode(PA6, INPUT_PULLDOWN);
  pinMode(PA7, INPUT_PULLDOWN);
  pinMode(PA8, INPUT_PULLDOWN);
  pinMode(PA9, INPUT_PULLDOWN);
  pinMode(PA10, INPUT_PULLDOWN);
  
  pinMode(PA11, INPUT_PULLDOWN);
  pinMode(PA12, INPUT_PULLDOWN);
  //WSI port
  //pinMode(PA13, INPUT_PULLDOWN);
  //pinMode(PA14, INPUT_PULLDOWN);
  pinMode(PA15, INPUT_PULLDOWN);

  pinMode(PB0, INPUT_PULLDOWN);
  pinMode(PB1, INPUT_PULLDOWN);
  pinMode(PB3, INPUT_PULLDOWN);
  pinMode(PB4, INPUT_PULLDOWN);
  pinMode(PB5, INPUT_PULLDOWN);
  pinMode(PB6, INPUT_PULLDOWN);
  pinMode(PB7, INPUT_PULLDOWN);
}

#endif