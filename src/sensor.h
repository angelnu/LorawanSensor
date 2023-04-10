#ifndef sensor_h
#define sensor_h

#include <CayenneLPP.h>
#ifdef SENSOR_SOIL_AVR
namespace std {
    #include "Vector.h"
    template<class T> using vector = Vector<T>;
}
#else
#include <vector>
#endif
#include "config.h"

class Sensor;
class Sensors {
    public:
        static void register_sensor(Sensor* sensorPtr) {
            sensor_pointers().push_back(sensorPtr);
        };
        static void set_config(device_config_device_t& specific_device_config);
        static void setup();
        static bool measure(CayenneLPP& lpp);
        static void sleep();
    private:
        Sensors();
        static bool iv_firstTime_setup;
        static bool iv_ready;
        static uint8_t skippedMeasurements;
        static std::vector<Sensor*>& sensor_pointers();

        //Helpers
        static void allInput();
};

class Sensor {
    public:
        Sensor(const char *name) :
            iv_firstTime_setup(true),
            iv_ready(false),
            iv_name(name) {
                Sensors::register_sensor(this);
            };
        ~Sensor(){ };
        
        virtual void set_config(device_config_device_t& specific_device_config) {};
        
        void setup() {
            if (not iv_ready) init(iv_firstTime_setup);
            iv_firstTime_setup = false;
            iv_ready = true;
        }
        bool measure() {
            setup();
            return measure_intern();
        };
        
        virtual void send(CayenneLPP& lpp) {};

        const char* get_name() {return this->iv_name;}

        void sleep() {
            if (iv_ready) stop();
            iv_ready = false;
        };

    private:
        bool iv_firstTime_setup;
        bool iv_ready;
        const char *iv_name;

        //Methods to override
        //Called after sleep or when the device starts
        virtual void init (bool firstTime) {};
        //Prepare data to send. Returns if the data is worth sensing (different enough)
        virtual bool measure_intern() {return false;};
        //Prepare to sleep
        virtual void stop () {};

    protected:

        //Helpers
        uint32_t averageAnalogInput(uint32_t pin, uint8_t measurements);
};


#endif // sensor_h
