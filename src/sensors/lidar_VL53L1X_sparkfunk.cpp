#include "config.h"
#ifdef SENSOR_LIDAR_VL53L1X_SPARFUNK

#include "lidar_VL53L1X.h"
#include <Wire.h>
#include <ComponentObject.h>
#include <RangeSensor.h>
#include <SparkFun_VL53L1X.h>
#include <vl53l1x_class.h>
#include <vl53l1_error_codes.h>

SFEVL53L1X distanceSensor;

void init_lidar(bool firstTime){
  if (firstTime) log_debug_ln("Init lidar VL53L1X");
  pinMode(PIN_LIDAR_POWER, OUTPUT);
  digitalWrite(PIN_LIDAR_POWER, HIGH);
  delay(2);

  if (distanceSensor.begin() != 0)
  {
    Serial.println("Failed to init lidar VL53L1X!");
    while (1);
  }
  #if defined(SENSOR_LIDAR_VL53L1X_SHORT)
    distanceSensor.setDistanceModeShort();
  #elif defined(SENSOR_LIDAR_VL53L1X_MEDIUM)
    distanceSensor.setDistanceModeMedium();
  #else //Default = long range mode
    distanceSensor.setDistanceModeLong();
  #endif
  distanceSensor.setOffset(device_config.device.distance_offsset);
  distanceSensor.startRanging();

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
  while (!distanceSensor.checkForDataReady())
  {
    delay(1);
  }
  uint16_t distance_raw_mm = distanceSensor.getDistance(); //Get the result of the measurement from the sensor
  peak_signal_mcps = distanceSensor.getSignalRate();
  ambient_light_mcps = distanceSensor.getAmbientRate();
  distanceSensor.clearInterrupt();

  distanceSensor.stopRanging();

  //Adjust distances
  distance_dm = 0.01 * distance_raw_mm; //Use dm to use at best the 2 decimals in Cayene analog
  log_debug(F("Range status: "));
  log_debug_ln(distanceSensor.getRangeStatus());
  if (distance_dm > LIDAR_MAX_DISTANCE_DM)
    distance_dm = LIDAR_MAX_DISTANCE_DM;
  switch(distanceSensor.getRangeStatus()){
    case 0:
      break;
    default:
     distance_dm = -distanceSensor.getRangeStatus();
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