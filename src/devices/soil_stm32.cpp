#ifdef SENSOR_SOIL_SMT32

#include "common_stm32.h"
#include "sensor.h"


#define DEFAULT_MIN_S 100
#define DEFAULT_MAX_S 600
#define DEFAULT_MIN_PERCENTAGE_V_2_SEND 1;
#define DEFAULT_MIN_PERCENTAGE_T_2_SEND 10;
#define DEFAULT_MIN_PERCENTAGE_S_2_SEND 5;

void set_device_specific_config(device_config_device_t& specific_device_config) {
  specific_device_config.min_s = DEFAULT_MIN_S;
  specific_device_config.max_s = DEFAULT_MAX_S;
  specific_device_config.min_percentage_v_2_send = DEFAULT_MIN_PERCENTAGE_V_2_SEND;
  specific_device_config.min_percentage_t_2_send = DEFAULT_MIN_PERCENTAGE_T_2_SEND;
  specific_device_config.min_percentage_s_2_send = DEFAULT_MIN_PERCENTAGE_S_2_SEND;

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
  float soil_p = (100.0 * (soilRaw - device_config.device.min_s)) / (device_config.device.max_s * device_config.measure_average);
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

  //Find if it a value has changed enough
  bool enough_change = false;

  if ((abs(battery_v - old_battery_v) / old_battery_v) >= device_config.device.min_percentage_v_2_send){
    enough_change = true;
    old_battery_v = battery_v;
  }

  if ((abs(temp_c - old_temp_c) / old_temp_c) >= device_config.device.min_percentage_t_2_send) {
    enough_change = true;
    old_temp_c = temp_c;
  }

  if (abs(soil_p - old_soil_p) >= device_config.device.min_percentage_s_2_send) {
    enough_change = true;
    old_soil_p = soil_p;
  }

  if (skippedMeasurements >= device_config.max_skiped_measurements)
    enough_change = true;

  if (enough_change) {
    skippedMeasurements = 0; //Reset
    lpp.reset();
    log_debug_ln(F("Sending measurement"));
    log_flush();
    lpp.addDigitalInput(SENSOR_VERSION_CHANNEL, device_config.version);
    lpp.addAnalogInput(SENSOR_BATTERY_CHANNEL, battery_v);
    lpp.addTemperature(SENSOR_TEMP_CHANNEL, temp_c);
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
