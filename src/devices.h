#include "arduino.h"

#if defined(SENSOR_SOIL_AVR)
    #define DEVICE_CONFIG_VERSION 1
    struct device_config_device_t {
        //Soil sensor
    };
    #define SENSOR_SOIL_EXTERNAL_AVR
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
    #define SENSOR_SOIL_CAPACITY
#elif defined(SENSOR_GENERIC_BASIC_SMT32)
    #define DEVICE_CONFIG_VERSION 1
    struct device_config_device_t {
        //Generic basic sensor
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
    };
#elif defined(SENSOR_GENERIC_DISTANCE_SMT32)
    #define DEVICE_CONFIG_VERSION 3
    struct device_config_device_t {
        //Distance sensor
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
        uint8_t min_percentage_distance_2_send;
        uint8_t unused;
        int16_t distance_offset;
    };
    #define SENSOR_LIDAR_VL53L1X_SPARFUNK
    #define SENSOR_LIDAR_VL53L1X_SHORT
    //#define SENSOR_LIDAR_VL53L1X_MEDIUM
#elif defined(SENSOR_GENERIC_DOOR_SMT32)
    #define DEVICE_CONFIG_VERSION 1
    struct device_config_device_t {
        //door sensor
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
    };
    #define SENSOR_DOOR_REED
    //send each 6 hours
    #define TX_INTERVAL 3600
    #define DEFAULT_MAX_SKIPED_MEASUREMENTS 6
#elif defined(SENSOR_GENERIC_TRACKER_SMT32)
    #define DEVICE_CONFIG_VERSION 2
    struct device_config_device_t {
        //door sensor
        uint8_t min_percentage_v_2_send;
        uint8_t min_percentage_t_2_send;
        uint8_t min_percentage_distance_2_send;
        uint8_t min_acceleration_dg_2_send;
        uint8_t min_possition_change_mg_2_send;
        uint8_t min_altitude_change_m_2_send;
        int16_t distance_offset;
    };
    //Sensors
    #define SENSOR_GPS_UBLOCK
    //#define SENSOR_LIDAR_VL53L1X_POLOLU
    #define SENSOR_LIDAR_VL53L1X_SPARFUNK
    #define SENSOR_LIDAR_VL53L1X_SHORT
    //#define SENSOR_LIDAR_VL53L1X_MEDIUM
    #define SENSOR_ACCELEROMETER_LIS3DH

    //Config
    //measure each 6 hours (quiet)
    #ifndef TX_INTERVAL
        #define TX_INTERVAL 6*3600
    #endif
    #ifndef DEFAULT_MAX_SKIPED_MEASUREMENTS
        #define DEFAULT_MAX_SKIPED_MEASUREMENTS 1
    #endif
    //Send each 10 minutes (movement)
    #ifndef TX_FAST_INTERVAL
        #define TX_FAST_INTERVAL_S 600
    #endif
#else
    #error "Unkown device type"
#endif