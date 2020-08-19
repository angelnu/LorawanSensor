#include "config.h"
#ifdef SENSOR_LIDAR_VL53L1X_POLOLU

#include "lidar_VL53L1X.h"
#include <Wire.h>
#include <VL53L1X.h>

VL53L1X lidar;

void init_lidar(bool firstTime){
  if (firstTime) log_debug_ln("Init lidar VL53L1X");
  pinMode(PIN_LIDAR_POWER, OUTPUT);
  digitalWrite(PIN_LIDAR_POWER, HIGH);
  delay(2);

  lidar.setTimeout(500);
  if (!lidar.init())
  {
    Serial.println("Failed to init lidar VL53L1X!");
    while (1);
  }
  #if defined(SENSOR_LIDAR_VL53L1X_SHORT)
    lidar.setDistanceMode(VL53L1X::Short);
  #elif defined(SENSOR_LIDAR_VL53L1X_MEDIUM)
    lidar.setDistanceMode(VL53L1X::Medium);
  #else //Default = long range mode
    lidar.setDistanceMode(VL53L1X::Long);
  #endif
  lidar.setMeasurementTimingBudget(50000);
  lidar.startContinuous(1000);

}
void stop_lidar(){

  digitalWrite(PIN_LIDAR_POWER, LOW);
}


float distance_dm;
float old_distance_dm = 0;
float peak_signal_mcps;
float old_peak_signal_mcps=0;
float ambient_light_mcps;
float old_ambient_light_mcps=0;
bool measure_lidar() {
  uint16_t distance_raw_mm = lidar.read();
  peak_signal_mcps = lidar.ranging_data.peak_signal_count_rate_MCPS;
  ambient_light_mcps = lidar.ranging_data.ambient_count_rate_MCPS;

  //Adjust distances
  distance_dm = 0.01 * distance_raw_mm; //Use dm to use at best the 2 decimals in Cayene analog
  log_debug(F("Range status: "));
  log_debug_ln(lidar.ranging_data.range_status);
  if (distance_dm > LIDAR_MAX_DISTANCE_DM)
    distance_dm = LIDAR_MAX_DISTANCE_DM;
  switch(lidar.ranging_data.range_status){
    case lidar.RangeValid:
      break;
    case lidar.MinRangeFail:
      distance_dm = 0;
      break;
    case lidar.SignalFail:
    case lidar.RangeValidMinRangeClipped:
    case lidar.OutOfBoundsFail:
      distance_dm = LIDAR_MAX_DISTANCE_DM+1;
      break;
    default:
     distance_dm = -lidar.ranging_data.range_status;
  }

  //Debug output
  log_debug(F("DISTANCE RAW (mm): "));
  log_debug_ln(distance_raw_mm);
  log_debug(F("DISTANCE (dm): "));
  log_debug_ln(distance_dm, 2);
  log_debug(F("PEAK SIGNAL (MCPS): "));
  log_debug_ln(peak_signal_mcps, 1);
  log_debug(F("AMBIENT LIGHT (MCPS): "));
  log_debug_ln(ambient_light_mcps, 1);

  //Find if it a value has changed enough
  return (
       (100.0*(abs(distance_dm - old_distance_dm) / LIDAR_MAX_DISTANCE_DM) >= device_config.device.min_percentage_distance_2_send)
     );
}

void send_lidar(CayenneLPP& lpp) { 
  //Add measurements and remember last transmit
  lpp.addAnalogInput(SENSOR_DISTANCE_CHANNEL, distance_dm);
  old_distance_dm = distance_dm;
  lpp.addAnalogInput(SENSOR_PEAK_SIGNAL_CHANNEL, peak_signal_mcps);
  old_peak_signal_mcps = peak_signal_mcps;
  lpp.addLuminosity(SENSOR_LUMINOSITY_CHANNEL, ambient_light_mcps);
  old_ambient_light_mcps = ambient_light_mcps;
}

#endif