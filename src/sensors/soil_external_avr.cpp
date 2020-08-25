#include "config.h"
#ifdef SENSOR_SOIL_EXTERNAL_AVR

#include "sensor.h"
#include "sleep.h"
#include "arduino.h"
class Sensor_soil: Sensor{
    private:
        void set_config(device_config_device_t& specific_device_config) override;
        void init (bool firstTime) override;
        bool measure_intern() override;
        void send(CayenneLPP& lpp) override;
        void stop () override;

        float battery_v;
        float soil_v;
        float soil_percent;
};
static Sensor_soil sensor;

#define SENSOR_SOIL_ENABLED A2
#define SENSOR_SOIL_WARM_TIME_MS 500
#define SENSOR_SOIL_ADC A1
#define SENSOR_SOIL_MAX_VALUE_V 3.0
#define SENSOR_BATTERY_ADC A0

void Sensor_soil::set_config(device_config_device_t& specific_device_config) {
    //Not implemented
}





void Sensor_soil::init(bool firstTime){

    //Configure PINs for measurement
    pinMode(SENSOR_SOIL_ENABLED, OUTPUT);
    digitalWrite(SENSOR_SOIL_ENABLED, HIGH);
    analogReference(SENSOR_ADC_REFERENCE);
    pinMode(SENSOR_SOIL_ADC, INPUT);
    pinMode(SENSOR_BATTERY_ADC, INPUT);

    //Sleep while soil sensor gets stable
    do_sleep(SENSOR_SOIL_WARM_TIME_MS, /*sleep_adc*/false);
}

void Sensor_soil::stop(){
  //Disable soil sensor
  digitalWrite(SENSOR_SOIL_ENABLED, LOW);
}

static float adc_to_mv(uint16_t raw){
  const uint16_t VCC_REF = 1100;
  const uint16_t MAX_ADC = 1024;
  const uint16_t R1 = 370;
  const uint16_t R2 = 1000;
  return (1.0*raw*VCC_REF/MAX_ADC) * (R1 + R2) / R1;
}
bool Sensor_soil::measure_intern() {
  
  battery_v = adc_to_mv(analogRead(SENSOR_BATTERY_ADC))/1000;
  soil_v = adc_to_mv(analogRead(SENSOR_SOIL_ADC))/1000;
  soil_percent = (1.0 - (soil_v / SENSOR_SOIL_MAX_VALUE_V))*100;

  return true; //Allways send
}

void Sensor_soil::send(CayenneLPP& lpp) { 
  lpp.addAnalogInput(SENSOR_BATTERY_CHANNEL, battery_v);
  lpp.addAnalogInput(SENSOR_SOIL_VOLTAGE_CHANNEL, soil_v);
  lpp.addRelativeHumidity(SENSOR_SOIL_HUMIDITY_CHANNEL, soil_percent);
}

#endif