#include "config.h"
#ifdef SENSOR_DOOR_REED

#include "sensor.h"
#include "sleep.h"
class Sensor_door: Sensor{
    public:
        Sensor_door(uint8_t lpp_channel, uint32_t pin_door, uint32_t pin_door_reset):
            Sensor(__FILE__),
            iv_lpp_channel(lpp_channel),
            iv_pin_door(pin_door),
            iv_pin_door_reset(pin_door_reset) {    
            };
    private:
        //void set_config(device_config_device_t& specific_device_config) override;
        void init (bool firstTime) override;
        bool measure_intern() override;
        void send(CayenneLPP& lpp) override;
        //void stop () override;

        uint8_t iv_lpp_channel;
        uint32_t iv_pin_door;
        uint32_t iv_pin_door_reset;

        bool door_open;
        bool sent_door_open=false;

        static void pin_interrupt_handler();
        bool get_door_open();
};

#define UNUSED_PIN 0xFFFFFFFF
#ifdef PIN_DOOR_1
    #ifndef PIN_DOOR_1_RESET
        #define PIN_DOOR_1_RESET UNUSED_PIN
    #endif
    static Sensor_door sensor_1 = Sensor_door(SENSOR_DOOR_1_CHANNEL, PIN_DOOR_1, PIN_DOOR_1_RESET);
#endif
#ifdef PIN_DOOR_2
    #ifndef PIN_DOOR_2_RESET
        #define PIN_DOOR_2_RESET UNUSED_PIN
    #endif
    static Sensor_door sensor_2 = Sensor_door(SENSOR_DOOR_2_CHANNEL, PIN_DOOR_2, PIN_DOOR_2_RESET);
#endif
#ifdef PIN_DOOR_3
    #ifndef PIN_DOOR_3_RESET
        #define PIN_DOOR_3_RESET UNUSED_PIN
    #endif
    static Sensor_door sensor_3 = Sensor_door(SENSOR_DOOR_3_CHANNEL, PIN_DOOR_3, PIN_DOOR_3_RESET);
#endif
#ifdef PIN_DOOR_4
    #ifndef PIN_DOOR_4_RESET
        #define PIN_DOOR_4_RESET UNUSED_PIN
    #endif
    static Sensor_door sensor_4 = Sensor_door(SENSOR_DOOR_4_CHANNEL, PIN_DOOR_4, PIN_DOOR_4_RESET);
#endif





void Sensor_door::init(bool firstTime){

    if (firstTime) {
        uint32_t pin_mode =
        //INPUT_PULLUP; //This consumes too much (40 KOhms)
        INPUT_FLOATING;

        pinMode(iv_pin_door, pin_mode);
        attachInterrupt(digitalPinToInterrupt(iv_pin_door), pin_interrupt_handler, CHANGE);

        if (iv_pin_door_reset != UNUSED_PIN) {
            pinMode(iv_pin_door_reset, pin_mode);
            attachInterrupt(digitalPinToInterrupt(iv_pin_door_reset), pin_interrupt_handler, CHANGE);
        }
    }
}

bool Sensor_door::measure_intern() {
  
    //Read sensors
    door_open = get_door_open();

    return (door_open != sent_door_open);
}

void Sensor_door::send(CayenneLPP& lpp) { 
  lpp.addDigitalInput(iv_lpp_channel, door_open);
  sent_door_open = door_open;
}

void Sensor_door::pin_interrupt_handler() {
    skip_sleep();
}

bool Sensor_door::get_door_open() {
    if (iv_pin_door_reset != UNUSED_PIN) {
        if (digitalRead(iv_pin_door_reset)) {
            //Reset -> mark door as closed
            log_debug_ln("Door reset -> force closed");
            return false;
        } else {
            //If door was open previously then keep that status until reset
            log_debug_ln("Door persistance");
            return door_open || digitalRead(iv_pin_door);
        }
    } else {
        //Just return current door status
        log_debug_ln("Door current");
        return digitalRead(iv_pin_door);
    }
}

#endif