#ifndef config_h
#define config_h

#include "Arduino.h"
#include "devices.h"

//Device config

#ifndef DEBUG
  #define DEBUG 0 
#endif
#ifndef DEFAULT_MAX_SKIPED_MEASUREMENTS
  #if DEBUG
    #define DEFAULT_MAX_SKIPED_MEASUREMENTS 1
  #else
    #define DEFAULT_MAX_SKIPED_MEASUREMENTS 60
  #endif
#endif
#define DEFAULT_AVERAGE_MEASUREMENTS 5
#define PAUSE_SECONDS_AFTER_WATCHDOG 600

struct lorawan_keys_t {
  uint8_t appkey[16];
  uint64_t deveui;
  uint64_t appeui;
};

struct device_config_t {
  //Common part
  uint8_t version_config;
  uint8_t version;
  uint8_t is_debug;
  uint8_t reserved;
  uint16_t measure_interval_s;
  uint8_t measure_average;
  uint8_t max_skiped_measurements;
  //device specific
  device_config_device_t device;
};

#define UID_BYTES_LENGTH 12

extern lorawan_keys_t lorawan_keys;
extern device_config_t device_config;
extern void set_device_specific_config(device_config_device_t& specific_device_config); //implemented differently for each device
void init_device_config(bool reset=false);
void write_device_config(device_config_t &device_config);
void print_buildinfo();



//channels
enum cayenneLPP_channels {
    SENSOR_VERSION_CHANNEL=1,
    SENSOR_BATTERY_CHANNEL=3,
    SENSOR_SOIL_VOLTAGE_CHANNEL=4,
    SENSOR_SOIL_HUMIDITY_CHANNEL=40,
    SENSOR_TEMP_CHANNEL=41,
    SENSOR_DISTANCE_CHANNEL=42,
    SENSOR_PEAK_SIGNAL_CHANNEL=43,
    SENSOR_LUMINOSITY_CHANNEL=44,
    SENSOR_DOOR_1_CHANNEL=45,
    SENSOR_DOOR_2_CHANNEL=46,
    SENSOR_DOOR_3_CHANNEL=47,
    SENSOR_DOOR_4_CHANNEL=48,
    SENSOR_GPS_POS_CHANNEL=49,
    SENSOR_GPS_SIV_CHANNEL=50,
    SENSOR_ACCELEROMETER_CHANNEL=51,
    SENSOR_MOVING_CHANNEL=52
};

//Sensor settings
#if defined(ARDUINO_ARCH_AVR)
  #define SENSOR_AVR_SOIL 1

  #define SENSOR_ADC_REFERENCE INTERNAL
#elif defined(ARDUINO_ARCH_STM32)

  #if defined(STM32L0xx)
    //See https://www.mouser.de/datasheet/2/389/stm32l010k8-1851374.pdf
    #define ADC_VREFINT_3V0_30C_ADDR 0x1FF80078
  #elif defined(STM32L4xx)
    //See https://www.mouser.de/datasheet/2/389/stm32l412c8-1851177.pdf
    #define ADC_VREFINT_3V0_30C_ADDR 0x1FFF75AA
    #define ADC_TEMP_3V0_30C_ADDR  0x1FFF75A8 
    #define ADC_TEMP_3V0_130C_ADDR 0x1FFF75CA
  #else
    #error Unknown STM32 MMU
  #endif
#else
  #error No config available for MCU
#endif

//Serial port
#define serialPortSpeed 115200
#define log_flush Serial.flush
#define log_error Serial.print
#define log_error_ln Serial.println
#define log_warning Serial.print
#define log_warning_ln Serial.println
#define log_info Serial.print
#define log_info_ln Serial.println
#if DEBUG
  #define log_debug Serial.print
  #define log_debug_ln Serial.println
#else
  #define log_debug if (false) Serial.print
  #define log_debug_ln if (false) Serial.println
#endif

#define Q(x) #x
#define QUOTE(x) Q(x)
#define GIT_COMMIT_ID QUOTE(PIO_SRC_REV)

#ifndef UNUSED
  #define UNUSED(x) x=x
#endif

//Lorawan
#define USE_ADR 1
// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
#ifdef TX_INTERVAL
  //Use directly
#elif DEBUG
  const uint32_t TX_INTERVAL=60;
#else
  const uint32_t TX_INTERVAL=60;
#endif

#define LMIC_CLOCK_ERROR_PERCENTAGE 5




#endif //config_h
