#include "config.h"
#include "sleep.h"
#include "lorawan.h"
#include "sensor.h"
#include "STM32LowPower.h"
#include "IWatchdog.h"

void setup() {
    sleep_setup();
    delay(5*1000);
    
    Serial.begin(serialPortSpeed);
    init_device_config();
    print_buildinfo();

    #ifdef IWDG_TIME_S
      IWatchdog.begin(IWDG_TIME_S*1000*1000);
    #endif
    #ifdef DEBUG_DURING_SLEEP
      HAL_DBGMCU_EnableDBGStopMode();
    #endif
    
    
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
