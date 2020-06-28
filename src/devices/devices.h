#include "arduino.h"

#if defined(SENSOR_SOIL_AVR)
    struct device_config_device_t {
        //Soil sensor
    };
#elif defined(SENSOR_SOIL_SMT32)
    struct device_config_device_t {
        //Soil sensor
        uint16_t min_s;
        uint16_t max_s;
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
        uint8_t min_percentage_s_2_send;
    };
#elif defined(SENSOR_DISTANCE_SMT32)
    struct device_config_device_t {
        //Soil sensor
        uint16_t min_s;
        uint16_t max_s;
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
        uint8_t min_percentage_d_2_send;
    };
#else
    #error "Unkown device type"
#endif