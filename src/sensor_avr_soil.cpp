#include "config.h"
#ifdef SENSOR_AVR_SOIL

#include "sleep.h"
#include "arduino.h"
#include "sensor.h"

static float adc_to_mv(uint16_t raw){
  const uint16_t VCC_REF = 1100;
  const uint16_t MAX_ADC = 1024;
  const uint16_t R1 = 370;
  const uint16_t R2 = 1000;
  return (1.0*raw*VCC_REF/MAX_ADC) * (R1 + R2) / R1;
}

//Called once during setup
void sensor_setup() {}

void sensor_measure(CayenneLPP& lpp){

  //Configure PINs for measurement
  pinMode(SENSOR_SOIL_ENABLED, OUTPUT);
  digitalWrite(SENSOR_SOIL_ENABLED, HIGH);
  analogReference(SENSOR_ADC_REFERENCE);
  pinMode(SENSOR_SOIL_ADC, INPUT);
  pinMode(SENSOR_BATTERY_ADC, INPUT);

  //Sleep while soil sensor gets stable
  do_sleep(SENSOR_SOIL_WARM_TIME_MS, /*sleep_adc*/false);

  //Read sensors
  float battery_v = adc_to_mv(analogRead(SENSOR_BATTERY_ADC))/1000;
  float soil_v = adc_to_mv(analogRead(SENSOR_SOIL_ADC))/1000;
  float soil_percent = (1.0 - (soil_v / SENSOR_SOIL_MAX_VALUE_V))*100;
  lpp.reset();
  lpp.addAnalogInput(SENSOR_BATTERY_CHANNEL, battery_v);
  lpp.addAnalogInput(SENSOR_SOIL_VOLTAGE_CHANNEL, soil_v);
  lpp.addRelativeHumidity(SENSOR_SOIL_HUMIDITY_CHANNEL, soil_percent);

  //Debug output
  log_debug(F("SOIL V: "));
  log_debug_ln(soil_v);
  log_debug(F("SOIL %: "));
  log_debug_ln(soil_percent);
  log_debug(F("BATTERY V: "));
  log_debug_ln(battery_v, 3);

  //Disable soil sensor
  digitalWrite(SENSOR_SOIL_ENABLED, LOW);

}

#endif //SENSOR_AVR_SOIL
