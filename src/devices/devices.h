#include "arduino.h"

#if defined(SENSOR_SOIL_AVR)
    #define DEVICE_CONFIG_VERSION 1
    struct device_config_device_t {
        //Soil sensor
    };
#elif defined(SENSOR_SOIL_SMT32)
    #define DEVICE_CONFIG_VERSION 4
    struct device_config_device_t {
        //Soil sensor
        uint16_t min_s;
        uint16_t max_s;
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
        uint8_t min_percentage_s_2_send;
    };
#elif defined(SENSOR_GENERIC_BASIC_SMT32)
    #define DEVICE_CONFIG_VERSION 1
    struct device_config_device_t {
        //Generic basic sensor
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
    };
#elif defined(SENSOR_GENERIC_DISTANCE_SMT32)
    #define DEVICE_CONFIG_VERSION 2
    struct device_config_device_t {
        //Distance sensor
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
        uint8_t min_percentage_distance_2_send;
    };
#else
    #error "Unkown device type"
#endif