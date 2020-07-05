#ifdef SENSOR_GENERIC_DISTANCE_SMT32

#include "common_stm32.h"
#include "sensor.h"
#include "sleep.h"
#include <Wire.h>
#include <VL53L1X.h>

#define DEFAULT_MIN_PERCENTAGE_V_2_SEND 1;
#define MAX_VOLTAGE_V 3.3
#define DEFAULT_MIN_PERCENTAGE_T_2_SEND 10;
#define MAX_TEMP_C 100.0
#define DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND 5;
#define MAX_DISTANCE_DM 40.00


void set_device_specific_config(device_config_device_t& specific_device_config) {
  specific_device_config.min_percentage_v_2_send = DEFAULT_MIN_PERCENTAGE_V_2_SEND;
  specific_device_config.min_percentage_t_2_send = DEFAULT_MIN_PERCENTAGE_T_2_SEND;
  specific_device_config.min_percentage_distance_2_send = DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND;
}

VL53L1X lidar;
void init_lidar(){
  log_debug_ln("Init LIDAR");
  pinMode(PIN_LIDAR_POWER, OUTPUT);
  digitalWrite(PIN_LIDAR_POWER, HIGH);
  delay(2);
  Wire.begin();
  //Wire.setClock(400000); // use 400 kHz I2C 

  lidar.setTimeout(500);
  if (!lidar.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1);
  }
  lidar.setDistanceMode(VL53L1X::Long);
  lidar.setMeasurementTimingBudget(50000);
  lidar.startContinuous(1000);

}
void stop_lidar(){
  //Wire.endTransmission(true);
  Wire.end();

  digitalWrite(PIN_LIDAR_POWER, LOW);
  pinMode(PB6, OUTPUT);
  digitalWrite(PB6, LOW);
  pinMode(PB7, OUTPUT);
  digitalWrite(PB7, LOW);
}

//Called once during setup
void sensor_setup() {

  allInput();

  analogReadResolution(ADC_RESOLUTION);

  init_lidar();
}



uint8_t skippedMeasurements = 0;
float old_battery_v = 0;
float old_temp_c = 0;
float old_distance_dm = 0;
float old_peak_signal_mcps=0;
float old_ambient_light_mcps=0;
bool sensor_measure(CayenneLPP& lpp){

  //Init sensor
  init_lidar();
    
  //Read sensors
  uint32_t VRef = readVref();
  float battery_v = 1.0 * VRef / 1000;
  float temp_c = readTempSensor(VRef);
  float distance_dm = 0.01 * lidar.read(); //Use dm to use at best the 2 decimals in Cayene analog
  float peak_signal_mcps = lidar.ranging_data.peak_signal_count_rate_MCPS;
  float ambient_light_mcps = lidar.ranging_data.ambient_count_rate_MCPS;

  //Adjust distances
  if (distance_dm > MAX_DISTANCE_DM)
    distance_dm = MAX_DISTANCE_DM;
  switch(lidar.ranging_data.range_status){
    case lidar.RangeValid:
      break;
    case lidar.MinRangeFail:
      distance_dm = 0;
      break;
    case lidar.SignalFail:
    case lidar.RangeValidMinRangeClipped:
    case lidar.OutOfBoundsFail:
      distance_dm = MAX_DISTANCE_DM+1;
      break;
    default:
     distance_dm = -lidar.ranging_data.range_status;
  }

  //Stop sensor
  stop_lidar();
    
  //Debug output
  log_debug(F("BATTERY V: "));
  log_debug_ln(battery_v, 3);
  log_debug(F("TEMP C: "));
  log_debug_ln(temp_c, 1);
  log_debug(F("DISTANCE (dm): "));
  log_debug_ln(distance_dm, 1);
  log_debug(F("PEAK SIGNAL (MCPS): "));
  log_debug_ln(peak_signal_mcps, 1);
  log_debug(F("AMBIENT LIGHT (MCPS): "));
  log_debug_ln(ambient_light_mcps, 1);

  //Find if it a value has changed enough
  bool enough_change = false;

  if ((100.0*(abs(battery_v - old_battery_v) / MAX_VOLTAGE_V) >= device_config.device.min_percentage_v_2_send) ||
      (100.0*(abs(temp_c - old_temp_c) / MAX_TEMP_C) >= device_config.device.min_percentage_t_2_send) ||
      (100.0*(abs(distance_dm - old_distance_dm) / MAX_DISTANCE_DM) >= device_config.device.min_percentage_distance_2_send) ||
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
    lpp.addAnalogInput(SENSOR_DISTANCE_CHANNEL, distance_dm);
    old_distance_dm = distance_dm;
    lpp.addAnalogInput(SENSOR_PEAK_SIGNAL_CHANNEL, peak_signal_mcps);
    old_peak_signal_mcps = peak_signal_mcps;
    lpp.addLuminosity(SENSOR_LUMINOSITY_CHANNEL, ambient_light_mcps);
    old_ambient_light_mcps = ambient_light_mcps;
  } else {
    skippedMeasurements ++;

    log_debug(F("Skipped sending measurement: "));
    log_debug_ln(skippedMeasurements);
    log_flush();
  }

  return enough_change;
}

#endif //SENSOR_SMT32_SOIL
