#ifdef SENSOR_GENERIC_BASIC_SMT32

#include "sensors/common_stm32.h"
#include "sensor.h"

#define DEFAULT_MIN_PERCENTAGE_V_2_SEND 1;
#define MAX_VOLTAGE_V 3.3
#define DEFAULT_MIN_PERCENTAGE_T_2_SEND 10;
#define MAX_TEMP_C 100.0


void set_device_specific_config(device_config_device_t& specific_device_config) {
  specific_device_config.min_percentage_v_2_send = DEFAULT_MIN_PERCENTAGE_V_2_SEND;
  specific_device_config.min_percentage_t_2_send = DEFAULT_MIN_PERCENTAGE_T_2_SEND
}


//Called once during setup
void sensor_setup() {

  allInput();

  analogReadResolution(ADC_RESOLUTION);
}



uint8_t skippedMeasurements = 0;
float old_battery_v = 0;
float old_temp_c = 0;
bool sensor_measure(CayenneLPP& lpp){

  //Init sensor
    
  //Read sensors
  uint32_t VRef = readVref();
  float battery_v = 1.0 * VRef / 1000;
  float temp_c = readTempSensor(VRef);

  //Stop sensor
    
  //Debug output
  log_debug(F("BATTERY V: "));
  log_debug_ln(battery_v, 3);
  log_debug(F("TEMP C: "));
  log_debug_ln(temp_c, 1);

  //Find if it a value has changed enough
  bool enough_change = false;

  if ((100.0*(abs(battery_v - old_battery_v) / MAX_VOLTAGE_V) >= device_config.device.min_percentage_v_2_send) ||
      (100.0*(abs(temp_c - old_temp_c) / MAX_TEMP_C) >= device_config.device.min_percentage_t_2_send) ||
      (skippedMeasurements >= device_config.max_skiped_measurements))
  {
    enough_change = true;
  }

  if (enough_change) {
    skippedMeasurements = 0; //Reset
    log_debug_ln(F("Sending measurement"));
    log_flush();
    lpp.reset();
    lpp.addDigitalInput(SENSOR_VERSION_CHANNEL, device_config.version);
    lpp.addAnalogInput(SENSOR_BATTERY_CHANNEL, battery_v);
    old_battery_v = battery_v;
    lpp.addTemperature(SENSOR_TEMP_CHANNEL, temp_c);
    old_temp_c = temp_c;
  } else {
    skippedMeasurements ++;

    log_debug(F("Skipped sending measurement: "));
    log_debug_ln(skippedMeasurements);
    log_flush();
  }

  return enough_change;
}

#endif //SENSOR_SMT32_SOIL
