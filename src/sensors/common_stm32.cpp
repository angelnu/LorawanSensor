#if defined(ARDUINO_ARCH_STM32)

#include "sensor.h"
class Common_stm32_sensor: Sensor{
    public:
        Common_stm32_sensor() : Sensor(__FILE__) {}
    private:
        void set_config(device_config_device_t& specific_device_config) override;
        bool measure_intern() override;
        void send(CayenneLPP& lpp) override;

        float battery_v;
        float old_battery_v = 0;
        float temp_c;
        float old_temp_c = 0;

        uint32_t readVref();
        float readTempSensor(int32_t VRef);

};
static Common_stm32_sensor sensor;

#define ADC_RANGE (1<<ADC_RESOLUTION)
#define DEFAULT_MIN_PERCENTAGE_V_2_SEND 1;
#define MAX_VOLTAGE_V 3.3
#define DEFAULT_MIN_PERCENTAGE_T_2_SEND 10;
#define MAX_TEMP_C 100.0

void Common_stm32_sensor::set_config(device_config_device_t& specific_device_config){
    specific_device_config.min_percentage_v_2_send = DEFAULT_MIN_PERCENTAGE_V_2_SEND;
    specific_device_config.min_percentage_t_2_send = DEFAULT_MIN_PERCENTAGE_T_2_SEND;
}





// VREF

//See https://github.com/stm32duino/Arduino_Core_STM32/pull/539
const uint16_t* const ADC_VREFINT_3V0_30C =  reinterpret_cast<uint16_t*>(ADC_VREFINT_3V0_30C_ADDR);
const uint32_t CALIBRATION_REFERENCE_VOLTAGE = 3000;
const float VREFINT = CALIBRATION_REFERENCE_VOLTAGE * (*ADC_VREFINT_3V0_30C) / ADC_RANGE;

uint32_t Common_stm32_sensor::readVref()
{
    return (VREFINT * ADC_RANGE / averageAnalogInput(AVREF, device_config.measure_average)); // ADC sample to mV
}


// TEMPERATURE
#ifdef ADC_TEMP_3V0_30C_ADDR
    // see datasheet for position of the calibration values, this is for STM32L4xx
    const uint16_t* const ADC_TEMP_3V0_30C =  reinterpret_cast<uint16_t*>(ADC_TEMP_3V0_30C_ADDR);
    const uint16_t* const ADC_TEMP_3V0_130C =  reinterpret_cast<uint16_t*>(ADC_TEMP_3V0_130C_ADDR);


    float Common_stm32_sensor::readTempSensor(int32_t VRef)
    {
        // scale constants to current reference voltage
        float adcCalTemp30C = static_cast<float>(*ADC_TEMP_3V0_30C);
        float adcCalTemp130C = static_cast<float>(*ADC_TEMP_3V0_130C);

        //Read and convert to calibration value
        float adcTempValue = 1.0 * averageAnalogInput(ATEMP, device_config.measure_average) * VRef / CALIBRATION_REFERENCE_VOLTAGE;

        return (adcTempValue - adcCalTemp30C)/(adcCalTemp130C - adcCalTemp30C) * (130.0 - 30.0) + 30.0;
    }
#else
    float Common_stm32_sensor::readTempSensor(int32_t VRef) {return 0;}
#endif



bool Common_stm32_sensor::measure_intern() {
  
  uint32_t VRef = readVref();
  //Loop until the Vref is stable ( within 10 mv)
  for (uint32_t new_VRef = readVref(); VRef/10 != new_VRef/10; new_VRef = readVref()) {
    VRef = new_VRef;
  }
  battery_v = 1.0 * VRef / 1000;
  temp_c = readTempSensor(VRef);

  //Debug output
  log_debug(F("BATTERY V: "));
  log_debug_ln(battery_v, 3);
  log_debug(F("TEMP C: "));
  log_debug_ln(temp_c, 1);

  //Find if it a value has changed enough
  return (
      (100.0*(abs(battery_v - old_battery_v) / MAX_VOLTAGE_V) >= device_config.device.min_percentage_v_2_send) ||
      (100.0*(abs(temp_c - old_temp_c) / MAX_TEMP_C) >= device_config.device.min_percentage_t_2_send)
    );
}

void Common_stm32_sensor::send(CayenneLPP& lpp) { 
  //Add measurements and remember last transmit
  lpp.addDigitalInput(SENSOR_VERSION_CHANNEL, device_config.version);
  lpp.addAnalogInput(SENSOR_BATTERY_CHANNEL, battery_v);
  old_battery_v = battery_v;
  lpp.addTemperature(SENSOR_TEMP_CHANNEL, temp_c);
  old_temp_c = temp_c;
}

#endif