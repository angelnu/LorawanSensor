#include "config.h"
#include "sleep.h"
#include "lorawan.h"
#include "sensor.h"
#include "STM32LowPower.h"
#include "IWatchdog.h"

void setup() {

    #ifdef DEBUG_DURING_SLEEP
      HAL_DBGMCU_EnableDBGStopMode();
    #endif

    sleep_setup();
    Serial.begin(serialPortSpeed);
    
    #ifdef IWDG_TIME_S
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        __HAL_RCC_CLEAR_RESET_FLAGS();
        log_info_ln(F("Reset from watchdog -> sleeping"));
      #ifdef PIN_STATUS_LED
        //Blink error
        pinMode(PIN_STATUS_LED, OUTPUT);
        for (size_t i=0;i<5;i++) {
          digitalWrite(PIN_STATUS_LED, HIGH);
          delay(200);
          digitalWrite(PIN_STATUS_LED, LOW);
          delay(200);
        }
      #endif
        log_flush();
        do_sleep(PAUSE_SECONDS_AFTER_WATCHDOG * 1000);
      }

      //Enable watchdog
      IWatchdog.begin(IWDG_TIME_S*1000*1000);
      
    #endif

    //Give a few seconds to connect programmer
    delay(5*1000);
    
    init_device_config();
    print_buildinfo();
    
    sensor_setup();
    #ifdef PIN_STATUS_LED
      pinMode(PIN_STATUS_LED, OUTPUT);
      digitalWrite(PIN_STATUS_LED, HIGH);
    #endif
  
    lorawan_setup();
    
    log_info_ln(F("Starting"));
    log_flush();

    #ifdef PIN_STATUS_LED
      pinMode(PIN_STATUS_LED, OUTPUT);
      digitalWrite(PIN_STATUS_LED, LOW);
    #endif
}

CayenneLPP lpp(51);
void loop_work() {

    #ifdef IWDG_TIME_S
      IWatchdog.reload();
    #endif
    
    lorawan_resume();
    
    //Measure
    bool mustSend = sensor_measure(lpp);

    // Start job (sending automatically starts OTAA too)
    if (mustSend) lorawan_send(lpp.getBuffer(), lpp.getSize());

    lorawan_suspend();

}

void loop() {
  loop_periodically(device_config.measure_interval_s*1000, loop_work); 
  //log_info(F("WAIT"));
  //while(1){IWatchdog.reload();delay(5*1000);};
}
