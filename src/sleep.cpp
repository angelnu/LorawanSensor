#include "sleep.h"
#include "config.h"
#include "assert.h"

#if defined(ARDUINO_ARCH_AVR)
#include "LowPower.h"
#elif defined(ARDUINO_ARCH_STM32)
#include "STM32LowPower.h"
#endif

void sleep_setup() {
  
#if defined(ARDUINO_ARCH_STM32)

  STM32RTC& rtc = STM32RTC::getInstance();
  #ifdef LSE_CLOCK_ENABLED
    log_debug_ln(F("Low Speed External clock source"));
    rtc.setClockSource(STM32RTC::LSE_CLOCK);
  #else
    log_debug_ln(F("Default clock source"));
  #endif
  rtc.begin();
  
  LowPower.begin();

  #if defined(STM32L4xx) 
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
    HAL_PWREx_DisableBORPVD_ULP();
  #endif
    
#endif
}

/* **************************************************************
* sleep
* *************************************************************/
//Sets the millis value
static void addMillis(unsigned long millis)
{
  return;
  #ifdef ARDUINO_ARCH_AVR
    uint8_t oldSREG = SREG;

    cli();
    extern volatile unsigned long timer0_millis;
    extern volatile unsigned long timer0_overflow_count;
    timer0_millis += millis;
    timer0_overflow_count += microsecondsToClockCycles((uint32_t)millis * 1000) / (64 * 256);
    SREG = oldSREG;
  #elif defined(ARDUINO_ARCH_STM32)
    extern __IO uint32_t uwTick;
     __disable_irq();
    uwTick += millis;
     __enable_irq();
  #endif
}

void do_sleep(uint32_t sleepTime, size_t mode) {

  log_debug(F("Sleep for "));
  log_debug(sleepTime);
  log_debug_ln(F(" mseconds"));
  log_flush();

  #if ! defined(NO_DEEP_SLEEP) && defined(ARDUINO_ARCH_AVR)

    if (mode == SLEEP_MODE_PRECISE) {
      delay(sleepTime);
      return;
    }

    adc_t sleep_adc_parm;
    if (mode == SLEEP_MODE_DEEPSLEEP_WITH_AC)
     sleep_adc_parm = ADC_ON;
    else
     sleep_adc_parm = ADC_OFF;

     // sleep logic using LowPower library
     int delays[] = {8000, 4000, 2000, 1000, 500, 250, 120, 60, 30, 15};
     period_t sleep[] = {SLEEP_8S, SLEEP_4S, SLEEP_2S, SLEEP_1S, SLEEP_500MS,  SLEEP_250MS, SLEEP_120MS, SLEEP_60MS, SLEEP_30MS, SLEEP_15MS};

    // correction for overhead in this routine
    float sleepTimeLeft = sleepTime * 0.95 - 1;

    float x;
    unsigned int i;
    for (i=0; i<=9; i++) {
      for (x=sleepTimeLeft; x>=delays[i]; x-=delays[i]) {
        LowPower.powerDown(sleep[i], sleep_adc_parm, BOD_OFF);
        sleepTimeLeft -= delays[i];
      }
    }

    addMillis(sleepTime);
  #elif ! defined(NO_DEEP_SLEEP) && defined(ARDUINO_ARCH_STM32)

    STM32RTC& rtc = STM32RTC::getInstance();

    //SLEEP_MODE_DEEPSLEEP seems to be precise enough after reseting RTC epoch
    //if (sleepTime > 500)
    //  mode = SLEEP_MODE_DEEPSLEEP;
    if (mode==SLEEP_MODE_DEEPSLEEP) {

      //Round the sleep time
      //if (sleepTime % 1000) sleepTime+=1000;
      //sleepTime = (sleepTime/1000)*1000;

      //remember start
      uint32_t start_ms=0;
      uint32_t startTime = rtc.getEpoch(&start_ms);

      HAL_SuspendTick();

      //HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
      //sleepTime -= 200;
      //log_debug(F("Deep sleep "));
      //log_debug_ln(sleepTime);
      //log_flush();
      LowPower.deepSleep(sleepTime /* calibrated for STM32L04 */);

      //remember start
      uint32_t end_ms=0;
      uint32_t endTime = rtc.getEpoch(&end_ms);

      HAL_ResumeTick();
      sleepTime = (endTime - startTime) * 1000 + endTime - startTime;
      
      addMillis(sleepTime);
      log_debug(F("Deep slept "));
      log_debug_ln(sleepTime);
    
    } else {
      uint32_t start=millis();
      while (millis() < start+sleepTime)
        HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    }

  #else
    delay(sleepTime);
  #endif

}

static uint32_t requested_loop_time_ms;
static uint32_t next_loop_time_ms;
void loop_periodically(uint32_t ms, void (&loop_work)()){

  //Time before work
  unsigned long timeBeforeSending = millis();

  //By default sleep requested time at least changed with fast_sleep()
  requested_loop_time_ms = next_loop_time_ms = ms;

  loop_work();

  //Time after work
  unsigned long timeAfterSending = millis();

  // go to sleep
  if ((!is_skip_sleep()) && next_loop_time_ms > (timeAfterSending - timeBeforeSending)) {
    do_sleep(next_loop_time_ms - (timeAfterSending - timeBeforeSending));  // sleep minus elapsed time
  } else {
    if (is_skip_sleep()) {
      log_debug_ln(F("Skipped sleep by request"));
    } else {
      log_warning_ln(F("Long loop -> skipped sleep"));
    }
  }
};

void fast_sleep(uint32_t ms) {
  //Keep smallest
  if (ms < next_loop_time_ms) next_loop_time_ms = ms;
}
bool is_fast_sleep() {
  return (requested_loop_time_ms != next_loop_time_ms);
}
uint32_t next_sleep_time() {
  return next_loop_time_ms;
}
void skip_sleep() {
  fast_sleep(0);
}
bool is_skip_sleep() {
  return (next_sleep_time() == 0);
}
