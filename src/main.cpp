#include "config.h"
#include "sleep.h"
#include "lorawan.h"
#include "sensor.h"
//#include "STM32LowPower.h"
#ifdef IWDG_TIME_S
#include "IWatchdog.h"
#endif

void setup() {

    #ifdef DEBUG_DURING_SLEEP
        HAL_DBGMCU_EnableDBGStopMode();
    #elif defined(NO_DEBUG_DURING_SLEEP)
        HAL_DBGMCU_DisableDBGStopMode();
    #endif

    SystemClock_Config();

    sleep_setup();
    Serial.begin(serialPortSpeed);
    
    #ifdef IWDG_TIME_S
      if (IWatchdog.isReset(/* clear */ true)) {
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
    
    log_error("SystemCoreClock: ");
    log_error_ln(SystemCoreClock);
    log_error("F_CPU clock: ");
    log_error_ln(F_CPU);
    log_error("HSE_VALUE: ");
    log_error_ln(HSE_VALUE);
    
    Sensors::setup();
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
    
    //Measure
    bool mustSend = Sensors::measure(lpp);

    // Start job (sending automatically starts OTAA too)
    if (mustSend) {
    
      lorawan_resume();

      lorawan_send(lpp.getBuffer(), lpp.getSize());

      lorawan_suspend();
    }

}

void loop() {
  loop_periodically(device_config.measure_interval_s*1000, loop_work); 
  //log_info(F("WAIT"));
  //while(1){IWatchdog.reload();delay(5*1000);};
}
