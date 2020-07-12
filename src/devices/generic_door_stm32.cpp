#ifdef SENSOR_GENERIC_DOOR_SMT32

#include "common_stm32.h"
#include "sensor.h"
#include "sleep.h"

#define DEFAULT_MIN_PERCENTAGE_V_2_SEND 1;
#define MAX_VOLTAGE_V 3.3
#define DEFAULT_MIN_PERCENTAGE_T_2_SEND 10;
#define MAX_TEMP_C 100.0


void set_device_specific_config(device_config_device_t& specific_device_config) {
  specific_device_config.min_percentage_v_2_send = DEFAULT_MIN_PERCENTAGE_V_2_SEND;
  specific_device_config.min_percentage_t_2_send = DEFAULT_MIN_PERCENTAGE_T_2_SEND
}


void pin_interrupt_handler() {
  skip_sleep();
};
void pin_interrupt_handler_door_1() {
  log_debug_ln("Door 1 Interrupt");
  pin_interrupt_handler();
};
void pin_interrupt_handler_door_1_reset() {
  log_debug_ln("Door 1 Reset Interrupt");
  pin_interrupt_handler();
};
void pin_interrupt_handler_door_2() {
  log_debug_ln("Door 2 Interrupt");
  pin_interrupt_handler();
};
void pin_interrupt_handler_door_2_reset() {
  log_debug_ln("Door 2 Reset Interrupt");
  pin_interrupt_handler();
};
void pin_interrupt_handler_door_3() {
  log_debug_ln("Door 3 Interrupt");
  pin_interrupt_handler();
};
void pin_interrupt_handler_door_3_reset() {
  log_debug_ln("Door 3 Reset Interrupt");
  pin_interrupt_handler();
};
void pin_interrupt_handler_door_4() {
  log_debug_ln("Door 4 Interrupt");
  pin_interrupt_handler();
};
void pin_interrupt_handler_door_4_reset() {
  log_debug_ln("Door 4 Reset Interrupt");
  pin_interrupt_handler();
};


//Called once during setup
void sensor_setup() {

  allInput();

  analogReadResolution(ADC_RESOLUTION);

  uint32_t pin_mode =
    //INPUT_PULLUP; //This consumes too much (40 KOhms)
    INPUT_FLOATING;

  #ifdef PIN_DOOR_1
    pinMode(PIN_DOOR_1, pin_mode);
    attachInterrupt(digitalPinToInterrupt(PIN_DOOR_1), pin_interrupt_handler_door_1, CHANGE);
    #ifdef PIN_DOOR_1_RESET
      pinMode(PIN_DOOR_1_RESET, pin_mode);
      attachInterrupt(digitalPinToInterrupt(PIN_DOOR_1_RESET), pin_interrupt_handler_door_1_reset, CHANGE);
    #endif
  #endif
  #ifdef PIN_DOOR_2
    pinMode(PIN_DOOR_2, pin_mode);
    attachInterrupt(digitalPinToInterrupt(PIN_DOOR_2), pin_interrupt_handler_door_2, CHANGE);
    #ifdef PIN_DOOR_2_RESET
      pinMode(PIN_DOOR_2_RESET, pin_mode);
      attachInterrupt(digitalPinToInterrupt(PIN_DOOR_2_RESET), pin_interrupt_handler_door_2_reset, CHANGE);
    #endif
  #endif
  #ifdef PIN_DOOR_3
    pinMode(PIN_DOOR_3, pin_mode);
    attachInterrupt(digitalPinToInterrupt(PIN_DOOR_3), pin_interrupt_handler_door_3, CHANGE);
    #ifdef PIN_DOOR_3_RESET
      pinMode(PIN_DOOR_3_RESET, pin_mode);
      attachInterrupt(digitalPinToInterrupt(PIN_DOOR_3_RESET), pin_interrupt_handler_door_3_reset, CHANGE);
    #endif
  #endif
  #ifdef PIN_DOOR_4
    pinMode(PIN_DOOR_4, pin_mode);
    attachInterrupt(digitalPinToInterrupt(PIN_DOOR_4), pin_interrupt_handler_door_4, CHANGE);
    #ifdef PIN_DOOR_4_RESET
      pinMode(PIN_DOOR_4_RESET, pin_mode);
      attachInterrupt(digitalPinToInterrupt(PIN_DOOR_4_RESET), pin_interrupt_handler_door_4_reset, CHANGE);
    #endif
  #endif
}


bool get_door_open(bool old_door_pin, uint32_t door_pin, uint32_t reset_pin=0) {
  if (reset_pin > 0) {
    if (digitalRead(reset_pin)) {
      //Reset -> mark door as closed
      log_debug_ln("Door reset -> force closed");
      return false;
    } else {
      //If door was open previously then keep that status until reset
      log_debug_ln("Door persistance");
      return old_door_pin || digitalRead(door_pin);
    }
  } else {
    //Just return current door status
    log_debug_ln("Door current");
    return digitalRead(door_pin);
  }
}

uint8_t skippedMeasurements = 0;
float old_battery_v = 0;
float old_temp_c = 0;
#ifdef PIN_DOOR_1
bool old_door_1_open = false;
#endif
#ifdef PIN_DOOR_2
bool old_door_2_open = false;
#endif
#ifdef PIN_DOOR_3
bool old_door_3_open = false;
#endif
#ifdef PIN_DOOR_4
bool old_door_4_open = false;
#endif
bool sensor_measure(CayenneLPP& lpp){

  //Init sensor
    
  //Read sensors
  uint32_t VRef = readVref();
  float battery_v = 1.0 * VRef / 1000;
  float temp_c = readTempSensor(VRef);
  #ifdef PIN_DOOR_1
    bool door_1_open = 
    #ifdef PIN_DOOR_1_RESET
      get_door_open(old_door_1_open, PIN_DOOR_1, PIN_DOOR_1_RESET);
    #else
      get_door_open(old_door_1_open, PIN_DOOR_1);
    #endif
  #endif
  #ifdef PIN_DOOR_2
    bool door_2_open = 
    #ifdef PIN_DOOR_2_RESET
      get_door_open(old_door_2_open, PIN_DOOR_2, PIN_DOOR_2_RESET);
    #else
      get_door_open(old_door_2_open, PIN_DOOR_2);
    #endif
  #endif
  #ifdef PIN_DOOR_3
    bool door_3_open = 
    #ifdef PIN_DOOR_3_RESET
      get_door_open(old_door_3_open, PIN_DOOR_3, PIN_DOOR_3_RESET);
    #else
      get_door_open(old_door_3_open, PIN_DOOR_3);
    #endif
  #endif
  #ifdef PIN_DOOR_4
    bool door_4_open = 
    #ifdef PIN_DOOR_4_RESET
      get_door_open(old_door_4_open, PIN_DOOR_4, PIN_DOOR_4_RESET);
    #else
      get_door_open(old_door_4_open, PIN_DOOR_4);
    #endif
  #endif

  //Stop sensor
    
  //Debug output
  log_debug(F("BATTERY V: "));
  log_debug_ln(battery_v, 3);
  log_debug(F("TEMP C: "));
  log_debug_ln(temp_c, 1);
  #ifdef PIN_DOOR_1
  log_debug(F("DOOR 1 OPEN: "));
  log_debug_ln(door_1_open);
  #endif
  #ifdef PIN_DOOR_2
  log_debug(F("DOOR 2 OPEN: "));
  log_debug_ln(door_2_open);
  #endif
  #ifdef PIN_DOOR_3
  log_debug(F("DOOR 3 OPEN: "));
  log_debug_ln(door_3_open);
  #endif
  #ifdef PIN_DOOR_4
  log_debug(F("DOOR 4 OPEN: "));
  log_debug_ln(door_4_open);
  #endif

  //Find if it a value has changed enough
  bool enough_change = false;

  if ((100.0*(abs(battery_v - old_battery_v) / MAX_VOLTAGE_V) >= device_config.device.min_percentage_v_2_send) ||
      (100.0*(abs(temp_c - old_temp_c) / MAX_TEMP_C) >= device_config.device.min_percentage_t_2_send) ||
      #ifdef PIN_DOOR_1
      (door_1_open != old_door_1_open) ||
      #endif
      #ifdef PIN_DOOR_2
      (door_2_open != old_door_2_open) ||
      #endif
      #ifdef PIN_DOOR_3
      (door_3_open != old_door_3_open) ||
      #endif
      #ifdef PIN_DOOR_4
      (door_4_open != old_door_4_open) ||
      #endif
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
    lpp.addTemperature(SENSOR_DISTANCE_CHANNEL, temp_c);
    old_temp_c = temp_c;
    #ifdef PIN_DOOR_1
    lpp.addDigitalInput(SENSOR_DOOR_1_CHANNEL, door_1_open);
    old_door_1_open = door_1_open;
    #endif
    #ifdef PIN_DOOR_2
    lpp.addDigitalInput(SENSOR_DOOR_2_CHANNEL, door_2_open);
    old_door_2_open = door_2_open;
    #endif
    #ifdef PIN_DOOR_3
    lpp.addDigitalInput(SENSOR_DOOR_3_CHANNEL, door_3_open);
    old_door_3_open = door_3_open;
    #endif
    #ifdef PIN_DOOR_4
    lpp.addDigitalInput(SENSOR_DOOR_4_CHANNEL, door_4_open);
    old_door_4_open = door_4_open;
    #endif
  } else { 
    //Reset counter on interrupt to filter int noise
    if (is_skip_sleep()) skippedMeasurements=0;

    skippedMeasurements ++;

    log_debug(F("Skipped sending measurement: "));
    log_debug_ln(skippedMeasurements);
    log_flush();
  }

  return enough_change;
}

#endif //SENSOR_GENERIC_DOOR_SMT32
