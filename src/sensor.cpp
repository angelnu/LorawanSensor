#include "sensor.h"
#include "config.h"

#include "sleep.h"
#include "Wire.h"

// Average
uint32_t Sensor::averageAnalogInput(uint32_t pin, uint8_t measurements) {
  uint32_t total = 0;
  for (uint8_t i=0; i<measurements; i++){
    total += analogRead(pin);
  }
  return total / measurements;

}

bool Sensors::iv_firstTime_setup=true;
bool Sensors::iv_ready=false;
uint8_t Sensors::skippedMeasurements=0;

std::vector<Sensor*>& Sensors::sensor_pointers(){
    static std::vector<Sensor*>* ans = new std::vector<Sensor*>;
    return *ans;
}

void Sensors::set_config(device_config_device_t& specific_device_config) {
    for (size_t i=0; i < sensor_pointers().size(); i++) {
        sensor_pointers()[i]->set_config(specific_device_config);
    }
}        
void Sensors::setup() {
    if (iv_firstTime_setup) {
        allInput();
        #ifdef ADC_RESOLUTION
            analogReadResolution(ADC_RESOLUTION);
        #endif
    }
    if (not iv_ready) {
        Wire.begin();
        //Wire.setClock(400000); // use 400 kHz I2C 
        //Wire.setClock(10000); // use 100 kHz I2C 
        for (size_t i=0; i < sensor_pointers().size(); i++) {
            sensor_pointers()[i]->setup();
        }
    }
    iv_firstTime_setup = false;
    iv_ready = true;
}

void Sensors::sleep() {
  if (iv_ready) {
        //Wire.setClock(400000); // use 400 kHz I2C 
        //Wire.setClock(10000); // use 100 kHz I2C 
        for (size_t i=0; i < sensor_pointers().size(); i++) {
            sensor_pointers()[i]->sleep();
        }

        Wire.end();
        pinMode(PB6, OUTPUT);
        digitalWrite(PB6, LOW);
        pinMode(PB7, OUTPUT);
        digitalWrite(PB7, LOW);
    }
    iv_ready = false;
}


bool Sensors::measure(CayenneLPP& lpp){
    
    //Init sensor
    setup();
    
    //Read sensors
    bool enough_change = false;
    for (size_t i=0; i < sensor_pointers().size(); i++) {
        //we use the + operator to ensure all functions are called 
        //and avoid the "short circuit" of the OR operator
        enough_change += sensor_pointers()[i]->measure();
    }

    //Stop sensor
    sleep();
    
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
        lpp.addDigitalInput(SENSOR_VERSION_CHANNEL, device_config.version);
        for (size_t i=0; i < sensor_pointers().size(); i++) {
            sensor_pointers()[i]->send(lpp);
        }
    } else {

        //only count skipped in regular mode
        if (! is_fast_sleep() ){
            skippedMeasurements ++;
        }

        log_debug(F("Skipped sending measurement: "));
        log_debug_ln(skippedMeasurements);
        log_flush();
    }

  return enough_change;
}

void Sensors::allInput()
{
    log_debug_ln(F("Setting all pins as inputs"));
    #ifdef ARDUINO_ARCH_STM32
        pinMode(PA0, INPUT_PULLDOWN);
        pinMode(PA1, INPUT_PULLDOWN);
        //Serial port
        //pinMode(PA2, INPUT_PULLDOWN);
        //pinMode(PA3, INPUT_PULLDOWN);
        pinMode(PA4, INPUT_PULLDOWN);
        pinMode(PA5, INPUT_PULLDOWN);
        pinMode(PA6, INPUT_PULLDOWN);
        pinMode(PA7, INPUT_PULLDOWN);
        pinMode(PA8, INPUT_PULLDOWN);
        pinMode(PA9, INPUT_PULLDOWN);
        pinMode(PA10, INPUT_PULLDOWN);
        
        pinMode(PA11, INPUT_PULLDOWN);
        pinMode(PA12, INPUT_PULLDOWN);
        //WSI port
        //pinMode(PA13, INPUT_PULLDOWN);
        //pinMode(PA14, INPUT_PULLDOWN);
        pinMode(PA15, INPUT_PULLDOWN);

        pinMode(PB0, INPUT_PULLDOWN);
        pinMode(PB1, INPUT_PULLDOWN);
        pinMode(PB3, INPUT_PULLDOWN);
        pinMode(PB4, INPUT_PULLDOWN);
        pinMode(PB5, INPUT_PULLDOWN);
        pinMode(PB6, INPUT_PULLDOWN);
        pinMode(PB7, INPUT_PULLDOWN);
    #endif
}

