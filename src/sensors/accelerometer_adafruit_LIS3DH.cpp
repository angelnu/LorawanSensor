#include "config.h"
#ifdef SENSOR_ACCELEROMETER_ADAFRUIT_LIS3DH

#include "sensor.h"
#include <Adafruit_LIS3DH.h>
#include "lis3dh_reg.h"
class Sensor_accelerometer: Sensor{
    public:
        Sensor_accelerometer() : Sensor(__FILE__) {}
    private:
        void set_config(device_config_device_t& specific_device_config) override;
        void init (bool firstTime) override;
        bool measure_intern() override;
        //void send(CayenneLPP& lpp) override;
        //void stop () override;

        Adafruit_LIS3DH iv_sensor;
};
static Sensor_accelerometer sensor;

void Sensor_accelerometer::set_config(device_config_device_t& specific_device_config) {
}





void Sensor_accelerometer::init(bool firstTime){
    
    if (firstTime) {
        if (! iv_sensor.begin(0x19)) {   // change this to 0x19 for alternative i2c address
            log_error_ln(F("Failed to init accelerometer LIS3DH"));
            while (1) yield();
        }
        log_info_ln("Accelerometer LIS3DH found!");
    }
}

bool Sensor_accelerometer::measure_intern() {

    iv_sensor.read();      // get X Y and Z data at once
    // Then print out the raw data
    Serial.print("X:  "); Serial.print(iv_sensor.x);
    Serial.print("  \tY:  "); Serial.print(iv_sensor.y);
    Serial.print("  \tZ:  "); Serial.print(iv_sensor.z);

    /* Or....get a new sensor event, normalized */
    sensors_event_t event;
    iv_sensor.getEvent(&event);

    /* Display the results (acceleration is measured in m/s^2) */
    Serial.print("\t\tX: "); Serial.print(event.acceleration.x);
    Serial.print(" \tY: "); Serial.print(event.acceleration.y);
    Serial.print(" \tZ: "); Serial.print(event.acceleration.z);
    Serial.println(" m/s^2 ");

    Serial.println();
    return false;
}

#endif //SENSOR_ACCELEROMETER_ADAFRUIT_LIS3DH