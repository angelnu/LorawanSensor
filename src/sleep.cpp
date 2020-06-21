#include "sleep.h"
#include "config.h"
//#define SLEEP_ATMEGA 1
#if defined(ARDUINO_ARCH_AVR)
#include "LowPower.h"
#elif defined(ARDUINO_ARCH_STM32)
#include "STM32LowPower.h"
#endif

void sleep_setup() {
  
#if defined(ARDUINO_ARCH_STM32)

  STM32RTC& rtc = STM32RTC::getInstance();
  rtc.setClockSource(STM32RTC::LSE_CLOCK);
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
    //mode = SLEEP_MODE_DEEPSLEEP;
    if (mode==SLEEP_MODE_DEEPSLEEP) {

      uint32_t resto = sleepTime % 1000;
      if (resto > 0) {
        sleepTime -= resto;
        sleepTime += 1000;
      }

      rtc.setSubSeconds(0);
      uint32_t startTime = rtc.getEpoch();
      UNUSED(startTime);

      HAL_SuspendTick();

      //HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
      LowPower.deepSleep(sleepTime /* calibrated for STM32L04 */);

      uint32_t epochMs;
      //sleepTime = (rtc.getEpoch(&epochMs) - startTime) * 1000;
      //sleepTime += epochMs;
      addMillis(sleepTime);
      HAL_ResumeTick();

      log_debug(F("epoch  "));
      log_debug_ln(sleepTime);
      log_debug(F("epoch MS: "));
      log_debug_ln(epochMs);
    
    } else {
      uint32_t start=millis();
      while (millis() < start+sleepTime)
        HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    }

    //Serial.begin(serialPortSpeed);

  #else
    delay(sleepTime);
  #endif

}

void loop_periodically(uint32_t ms, void (&loop_work)()){

  //Time before work
  unsigned long timeBeforeSending = millis();

  loop_work();

  //Time after work
  unsigned long timeAfterSending = millis();

  // go to sleep
  if (ms > (timeAfterSending - timeBeforeSending))
    do_sleep(ms - (timeAfterSending - timeBeforeSending));  // sleep minus elapsed time
  else
    log_warning(F("Skipped sleep"));
};
