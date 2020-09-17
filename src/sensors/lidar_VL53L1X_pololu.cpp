#include "config.h"
#ifdef SENSOR_LIDAR_VL53L1X_POLOLU

#include "sensor.h"
#include <Wire.h>
#include <VL53L1X.h>
class Sensor_lidar: Sensor{
    private:
        void set_config(device_config_device_t& specific_device_config) override;
        void init (bool firstTime) override;
        bool measure_intern() override;
        void send(CayenneLPP& lpp) override;
        void stop () override;

        VL53L1X ivLidar;

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
#define DEFAULT_DISTANCE_OFFSET 100

void Sensor_lidar::set_config(device_config_device_t& specific_device_config) {
    specific_device_config.min_percentage_distance_2_send = DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND;
    specific_device_config.distance_offsset = DEFAULT_DISTANCE_OFFSET;
}





void Sensor_lidar::init(bool firstTime){
    if (firstTime) log_debug_ln("Init lidar VL53L1X");
    pinMode(PIN_LIDAR_POWER, OUTPUT);
    digitalWrite(PIN_LIDAR_POWER, HIGH);
    delay(2);

    ivLidar.setTimeout(500);
    if (!ivLidar.init())
    {
      Serial.println("Failed to init lidar VL53L1X!");
      while (1);
    }
    #if defined(SENSOR_LIDAR_VL53L1X_SHORT)
      ivLidar.setDistanceMode(VL53L1X::Short);
    #elif defined(SENSOR_LIDAR_VL53L1X_MEDIUM)
      ivLidar.setDistanceMode(VL53L1X::Medium);
    #else //Default = long range mode
      ivLidar.setDistanceMode(VL53L1X::Long);
    #endif
    ivLidar.setMeasurementTimingBudget(50000);
  ivLidar.startContinuous(1000);

}
void Sensor_lidar::stop(){
    pinMode(PIN_LIDAR_POWER, INPUT_PULLDOWN);
}



bool Sensor_lidar::measure_intern() {
  uint16_t distance_raw_mm = ivLidar.read();
  peak_signal_mcps = ivLidar.ranging_data.peak_signal_count_rate_MCPS;
  ambient_light_mcps = ivLidar.ranging_data.ambient_count_rate_MCPS;

  //Adjust distances
  distance_dm = 0.01 * distance_raw_mm; //Use dm to use at best the 2 decimals in Cayene analog
  log_debug(F("Range status: "));
  log_debug_ln(ivLidar.ranging_data.range_status);
  if (distance_dm > LIDAR_MAX_DISTANCE_DM)
    distance_dm = LIDAR_MAX_DISTANCE_DM;
  switch(ivLidar.ranging_data.range_status){
    case VL53L1X::RangeValid:
      break;
    case VL53L1X::MinRangeFail:
      distance_dm = 0;
      break;
    case VL53L1X::SignalFail:
    case VL53L1X::RangeValidMinRangeClipped:
    case VL53L1X::OutOfBoundsFail:
      distance_dm = LIDAR_MAX_DISTANCE_DM+1;
      break;
    default:
     distance_dm = -ivLidar.ranging_data.range_status;
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