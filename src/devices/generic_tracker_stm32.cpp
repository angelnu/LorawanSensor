#ifdef SENSOR_GENERIC_TRACKER_SMT32

#include "sensors/common_stm32.h"
#include "sensor.h"
#include "sleep.h"
#include "Wire.h"

#include "sensors/lidar_VL53L1X.h"
#include "sensors/gps_ublock.h"
#include "sensors/accelerometer_LIS3DH.h"


void set_device_specific_config(device_config_device_t& specific_device_config) {
  specific_device_config.min_percentage_v_2_send = DEFAULT_MIN_PERCENTAGE_V_2_SEND;
  specific_device_config.min_percentage_t_2_send = DEFAULT_MIN_PERCENTAGE_T_2_SEND;
  specific_device_config.min_percentage_distance_2_send = DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND;
  specific_device_config.distance_offsset = DEFAULT_DISTANCE_OFFSET;
}

void init_sensors(bool firstTime=false) {
  Wire.begin();
  //Wire.setClock(400000); // use 400 kHz I2C 
  Wire.setClock(10000); // use 100 kHz I2C 

  init_lidar(firstTime);
  init_accelerometer(firstTime);
  init_gps(firstTime);
}

void stop_sensors() {
  stop_lidar();
  stop_accelerometer();
  stop_gps();
  Wire.end();
  pinMode(PB6, OUTPUT);
  digitalWrite(PB6, LOW);
  pinMode(PB7, OUTPUT);
  digitalWrite(PB7, LOW);
}

//Called once during setup
void sensor_setup() {

  allInput();

  analogReadResolution(ADC_RESOLUTION);

  init_sensors( /*firstTime*/ true);
}

bool measure_all(){
  //Read sensors
  //we use the + operator to ensure all functions are called 
  //and avoid the "short circuit" of the OR operator
  bool enough_change =
    measure_stm32() +
    measure_lidar() +
    measure_accelerometer() +
    measure_gps() +
    false;
  return enough_change;
}

void send_all(CayenneLPP& lpp) {
  send_stm32(lpp);
  send_lidar(lpp);
  send_accelerometer(lpp);
  send_gps(lpp);
}



uint8_t skippedMeasurements = 0;
bool sensor_measure(CayenneLPP& lpp){

  //Init sensor
  init_sensors();
    
  //Read sensors
  bool enough_change = measure_all();

  //Stop sensor
  stop_sensors();
    
  //Check if we have skipped too many measurements
  if (skippedMeasurements >= device_config.max_skiped_measurements)
  {
    enough_change = true;
  }

  if (enough_change) {
    skippedMeasurements = 0; //Reset
    log_debug_ln(F("Sending measurement"));
    log_flush();
    lpp.reset();
    send_all(lpp);
  } else {
    skippedMeasurements ++;

    log_debug(F("Skipped sending measurement: "));
    log_debug_ln(skippedMeasurements);
    log_flush();
  }

  return enough_change;
}

#endif
