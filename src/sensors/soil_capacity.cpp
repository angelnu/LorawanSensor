#include "config.h"
#ifdef SENSOR_SOIL_CAPACITY

#include "sensor.h"
#include <CapacitiveSensor.h>
class Sensor_soil: Sensor{
    private:
        void set_config(device_config_device_t& specific_device_config) override;
        void init (bool firstTime) override;
        bool measure_intern() override;
        void send(CayenneLPP& lpp) override;
        //void stop () override;

        CapacitiveSensor* mySensor_ptr;

        float soil_p;
        float old_soil_p = 0;
};
static Sensor_soil sensor;

#define DEFAULT_MIN_S 100
#define DEFAULT_MAX_S 600
#define DEFAULT_MIN_PERCENTAGE_S_2_SEND 5;

void Sensor_soil::set_config(device_config_device_t& specific_device_config) {
  specific_device_config.min_s = DEFAULT_MIN_S;
  specific_device_config.max_s = DEFAULT_MAX_S;
  specific_device_config.min_percentage_s_2_send = DEFAULT_MIN_PERCENTAGE_S_2_SEND;
}





void Sensor_soil::init(bool firstTime){

    if (firstTime) {
        #ifdef PIN_CAPACITY_GROUND_0
            pinMode(PIN_CAPACITY_GROUND_0, OUTPUT);
            digitalWrite(PIN_CAPACITY_GROUND_0, LOW);
        #endif
        #ifdef PIN_CAPACITY_GROUND_1
            pinMode(PIN_CAPACITY_GROUND_1, OUTPUT);
            digitalWrite(PIN_CAPACITY_GROUND_1, LOW);
        #endif
        mySensor_ptr   = new CapacitiveSensor(PIN_CAPACITY_SEND, PIN_CAPACITY_READ);
    }
}

bool Sensor_soil::measure_intern() {
  
  float soilRaw = 1.0 * mySensor_ptr->capacitiveSensorRaw(device_config.measure_average) / device_config.measure_average;
  soil_p = 100.0 * (soilRaw - device_config.device.min_s) / device_config.device.max_s;
  if (soil_p > 100)
    soil_p = 100;

  //Find if it a value has changed enough
  return (abs(soil_p - old_soil_p) >= device_config.device.min_percentage_s_2_send);
}

void Sensor_soil::send(CayenneLPP& lpp) { 
  //Add measurements and remember last transmit
  lpp.addRelativeHumidity(SENSOR_SOIL_HUMIDITY_CHANNEL, soil_p);
  old_soil_p = soil_p;
}

#endif