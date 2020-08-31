#include "config.h"
#ifdef SENSOR_LIDAR_VL53L1X_SPARFUNK

#include "sensor.h"
#include <Wire.h>
#include <ComponentObject.h>
#include <RangeSensor.h>
#include <SparkFun_VL53L1X.h>
#include <vl53l1x_class.h>
#include <vl53l1_error_codes.h>
class Sensor_lidar: Sensor{
    private:
        void set_config(device_config_device_t& specific_device_config) override;
        void init (bool firstTime) override;
        bool measure_intern() override;
        void send(CayenneLPP& lpp) override;
        void stop () override;

        SFEVL53L1X ivLidar;

        float distance_dm;
        float old_distance_dm = 0;
        float peak_signal_mcps;
        float old_peak_signal_mcps=0;
        float ambient_light_mcps;
        float old_ambient_light_mcps=0;
};
static Sensor_lidar sensor;

#define DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND 5;
#define LIDAR_MAX_DISTANCE_DM 40.00
#define DEFAULT_DISTANCE_OFFSET 0

void Sensor_lidar::set_config(device_config_device_t& specific_device_config) {
  specific_device_config.min_percentage_distance_2_send = DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND;
  specific_device_config.distance_offset = DEFAULT_DISTANCE_OFFSET;
}





void Sensor_lidar::init(bool firstTime){

  pinMode(PIN_LIDAR_POWER, OUTPUT);
  digitalWrite(PIN_LIDAR_POWER, HIGH);
  delay(2);

  if (ivLidar.begin() != 0)
  {
    log_debug_ln("Failed to init lidar VL53L1X!");
    while (1);
  }
  if (firstTime) log_debug_ln("Found lidar VL53L1X!");
  #if defined(SENSOR_LIDAR_VL53L1X_SHORT)
    ivLidar.setDistanceModeShort();
  #elif defined(SENSOR_LIDAR_VL53L1X_MEDIUM)
    ivLidar.setDistanceModeLong();
  #else //Default = long range mode
    ivLidar.setDistanceModeLong();
  #endif
  ivLidar.setOffset(device_config.device.distance_offset);
  ivLidar.startRanging();

}
void Sensor_lidar::stop(){

  pinMode(PIN_LIDAR_POWER, INPUT_PULLDOWN);
}



bool Sensor_lidar::measure_intern() {
  return false;
  while (!ivLidar.checkForDataReady())
  {
    delay(1);
  }
  uint16_t distance_raw_mm = ivLidar.getDistance(); //Get the result of the measurement from the sensor
  peak_signal_mcps = ivLidar.getSignalRate();
  ambient_light_mcps = ivLidar.getAmbientRate();
  ivLidar.clearInterrupt();

  ivLidar.stopRanging();

  //Adjust distances
  distance_dm = 0.01 * distance_raw_mm; //Use dm to use at best the 2 decimals in Cayene analog
  log_debug(F("Range status: "));
  log_debug_ln(ivLidar.getRangeStatus());
  if (distance_dm > LIDAR_MAX_DISTANCE_DM)
    distance_dm = LIDAR_MAX_DISTANCE_DM;
  switch(ivLidar.getRangeStatus()){
    case 0:
      break;
    default:
     distance_dm = -ivLidar.getRangeStatus();
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

void Sensor_lidar::send(CayenneLPP& lpp) { 
  //Add measurements and remember last transmit
  lpp.addAnalogInput(SENSOR_DISTANCE_CHANNEL, distance_dm);
  old_distance_dm = distance_dm;
  lpp.addAnalogInput(SENSOR_PEAK_SIGNAL_CHANNEL, peak_signal_mcps);
  old_peak_signal_mcps = peak_signal_mcps;
  lpp.addLuminosity(SENSOR_LUMINOSITY_CHANNEL, ambient_light_mcps);
  old_ambient_light_mcps = ambient_light_mcps;
}

#endif