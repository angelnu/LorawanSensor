#include "config.h"
#ifdef SENSOR_SMT32_SOIL

#include "sleep.h"
#include "arduino.h"
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

static uint32_t readVref()
{
  return (VREFINT * ADC_RANGE / averageAnalogInput(AVREF, device_config.measure_average)); // ADC sample to mV
}


// TEMPERATURE
#ifdef ADC_TEMP_3V0_30C_ADDR

// see datasheet for position of the calibration values, this is for STM32L4xx
const uint16_t* const ADC_TEMP_3V0_30C =  reinterpret_cast<uint16_t*>(ADC_TEMP_3V0_30C_ADDR);
const uint16_t* const ADC_TEMP_3V0_130C =  reinterpret_cast<uint16_t*>(ADC_TEMP_3V0_130C_ADDR);


static float readTempSensor(int32_t VRef)
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

#include <CapacitiveSensor.h>
CapacitiveSensor* mySensor_ptr;

//Called once during setup
void sensor_setup() {
  allInput();

  analogReadResolution(ADC_RESOLUTION);

  //Soil sensor shield
  pinMode(PB0, OUTPUT);
  pinMode(PB1, OUTPUT);
  digitalWrite(PB0, LOW);
  digitalWrite(PB1, LOW);
  mySensor_ptr   = new CapacitiveSensor(PIN_CAPACITY_SEND, PIN_CAPACITY_READ);
}


float old_battery_v = 0;
float old_temp_c = 0;
float old_soil_p = 0;
uint8_t skippedMeasurements = 0;
bool sensor_measure(CayenneLPP& lpp){
  //Read sensors
  uint32_t VRef = readVref();
  float battery_v = 1.0 * VRef / 1000;
  float temp_c = readTempSensor(VRef);
  float soilRaw = mySensor_ptr->capacitiveSensorRaw(device_config.measure_average);
  float soil_p = (100.0 * (soilRaw - device_config.min_s)) / (device_config.max_s * device_config.measure_average);
  if (soil_p > 100)
    soil_p = 100;

  //Debug output
  log_debug(F("SOIL %: "));
  log_debug_ln(soil_p);
  log_debug(F("BATTERY V: "));
  log_debug_ln(battery_v, 3);
  log_debug(F("TEMP: "));
  log_debug_ln(temp_c, 1);
  log_debug(F("SOIL: "));
  log_debug_ln(soilRaw);
  log_flush();

  //Find if it a value has changed enough
  bool enough_change = false;

  if ((abs(battery_v - old_battery_v) / old_battery_v) >= device_config.min_percentage_v_2_send){
    enough_change = true;
    old_battery_v = battery_v;
  }

  if ((abs(temp_c - old_temp_c) / old_temp_c) >= device_config.min_percentage_s_2_send) {
    enough_change = true;
    old_temp_c = temp_c;
  }

  if (abs(soil_p - old_soil_p) >= device_config.min_percentage_s_2_send) {
    enough_change = true;
    old_soil_p = soil_p;
  }

  if (skippedMeasurements >= device_config.max_skiped_measurements)
    enough_change = true;

  if (enough_change) {
    skippedMeasurements = 0; //Reset
    lpp.reset();
    lpp.addDigitalInput(SENSOR_VERSION_CHANNEL, device_config.version);
    lpp.addAnalogInput(SENSOR_BATTERY_CHANNEL, battery_v);
    lpp.addTemperature(SENSOR_SOIL_TEMP_CHANNEL, temp_c);
    lpp.addRelativeHumidity(SENSOR_SOIL_HUMIDITY_CHANNEL, soil_p);
  } else {
    skippedMeasurements ++;

    log_debug(F("Skipped sending measurement: "));
    log_debug_ln(skippedMeasurements);
    log_flush();
  }

  return enough_change;
}

#endif //SENSOR_SMT32_SOIL
