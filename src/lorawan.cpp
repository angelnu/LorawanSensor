#include "config.h"
#include "sleep.h"
#include "arduino.h"
#include <lmic.h>
#include <SPI.h>
#include <hal/hal.h>
#include "lorawan.h"

/* **************************************************************
 * keys
 * *************************************************************/
#ifdef ABP_MODE
  // LoRaWAN NwkSKey, network session key
  // This should be in big-endian (aka msb).
  static const PROGMEM u1_t NWKSKEY[16] = D_NWKSKEY;

  // LoRaWAN AppSKey, application session key
  // This should also be in big-endian (aka msb).
  static const u1_t PROGMEM APPSKEY[16] = D_APPSKEY;

  // LoRaWAN end-device address (DevAddr)
  // See http://thethingsnetwork.org/wiki/AddressSpace
  // The library converts the address to network byte order as needed, so this should be in big-endian (aka msb) too.
  static const u4_t DEVADDR = D_DEVADDR ;

  // These callbacks are only used in over-the-air activation, so they are
  // left empty here (we cannot leave them out completely unless
  // DISABLE_JOIN is set in arduino-lmic/project_config/lmic_project_config.h,
  // otherwise the linker will complain).
  void os_getArtEui (u1_t* buf) { }
  void os_getDevEui (u1_t* buf) { }
  void os_getDevKey (u1_t* buf) { }
#else //Assume OTAA

  // This EUI must be in little-endian format, so least-significant-byte
  // first. When copying an EUI from ttnctl output, this means to reverse
  // the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
  // 0x70.
  void os_getArtEui (u1_t* buf) { memcpy_P(buf, &lorawan_keys.appeui, 8);}

  // This should also be in little endian format, see above.
  void os_getDevEui (u1_t* buf) { memcpy_P(buf, &lorawan_keys.deveui, 8);}

  // This key should be in big endian format (or, since it is not really a
  // number but a block of memory, endianness does not really apply). In
  // practice, a key taken from ttnctl can be copied as-is.
  void os_getDevKey (u1_t* buf) {  memcpy_P(buf, lorawan_keys.appkey, 16);}

#endif

/* **************************************************************
 * Pins
 * *************************************************************/
const lmic_pinmap lmic_pins = {
  .nss = PIN_LORAWAN_NSS,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = PIN_LORAWAN_RST,
  .dio = {PIN_LORAWAN_DIO0, PIN_LORAWAN_DIO1, PIN_LORAWAN_DIO2} //
};
/* **************************************************************
 * send the message
 * *************************************************************/
void lorawan_setup() {

  pinMode(PIN_LORAWAN_RST, OUTPUT);
  digitalWrite(PIN_LORAWAN_RST, LOW);
  delay(1000);
  digitalWrite(PIN_LORAWAN_RST, HIGH);
  delay(1000);


  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // ### Relax LMIC timing ###
  // Required for ATmega328/ATmega32U4 (8MHz) otherwise downlink messages
  // and OTAA joins on lower spreading factors will likely fail.
  #ifdef LMIC_CLOCK_ERROR_PERCENTAGE
    LMIC_setClockError(LMIC_CLOCK_ERROR_PERCENTAGE * (MAX_CLOCK_ERROR / 100.0));
  #endif

  #ifdef ABP_MODE
    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
      // On AVR, these values are stored in flash and only copied to RAM
      // once. Copy them to a temporary buffer here, LMIC_setSession will
      // copy them into a buffer of its own again.
      uint8_t appskey[sizeof(APPSKEY)];
      uint8_t nwkskey[sizeof(NWKSKEY)];
      memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
      memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
      LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);
    #else
      // If not running an AVR with PROGMEM, just use the arrays directly
      LMIC_setSession (0x13, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_eu868)
      // Set up the channels used by the Things Network, which corresponds
      // to the defaults of most gateways. Without this, only three base
      // channels from the LoRaWAN specification are used, which certainly
      // works, so it is good for debugging, but can overload those
      // frequencies, so be sure to configure the full frequency range of
      // your network here (unless your network autoconfigures them).
      // Setting up channels should happen after LMIC_setSession, as that
      // configures the minimal channel set.
      LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
      LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
      LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
      LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
      LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
      LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
      LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
      LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
      LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
      // TTN defines an additional channel at 869.525Mhz using SF9 for class B
      // devices' ping slots. LMIC does not have an easy way to define set this
      // frequency and support for class B is spotty and untested, so this
      // frequency is not configured here.
    #elif defined(CFG_us915)
      // NA-US channels 0-71 are configured automatically
      // but only one group of 8 should (a subband) should be active
      // TTN recommends the second sub band, 1 in a zero based count.
      // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
      LMIC_selectSubBand(1);
    #endif

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink
    LMIC_setDrTxpow(DR_SF7,14);

  #else //OTAA by default

    LMIC_setLinkCheckMode(USE_ADR);
    LMIC_setAdrMode(USE_ADR);

    LMIC_startJoining();
  #endif
}

#define MS_WAKEUP_EARLY 200
void os_sleep(uint32_t maxPeriod = 60000) {

  uint32_t sleepPeriod = 0;
  uint32_t period = maxPeriod;

  if (maxPeriod <= MS_WAKEUP_EARLY)
    return;

  if (! os_queryTimeCriticalJobs(((sleepPeriod + maxPeriod))*1000 >> US_PER_OSTICK_EXPONENT))
   return;

  while (period > 0) {
    period /= 2;

    if (! os_queryTimeCriticalJobs(((sleepPeriod + period))*1000 >> US_PER_OSTICK_EXPONENT))
      sleepPeriod += period;
  }
  
  if (sleepPeriod > MS_WAKEUP_EARLY) {
    log_debug(F("ENTRY: "));
    log_debug(millis());
    log_debug(F(" - "));
    log_debug_ln(sleepPeriod);
    log_flush();


      
    size_t sleep_mode = (sleepPeriod > 100*1000) ? SLEEP_MODE_DEEPSLEEP : SLEEP_MODE_PRECISE;
    do_sleep(sleepPeriod - MS_WAKEUP_EARLY, sleep_mode);
    //delay(sleepPeriod - MS_WAKEUP_EARLY);

    log_debug(F("EXIT: "));
    log_debug_ln(millis());
  }
}

/* **************************************************************
 * Process a command
 * *************************************************************/
void lorawan_process_command() {
  if (LMIC.dataLen == 0)
    return;
  
  uint8_t* data_ptr = &LMIC.frame[LMIC.dataBeg];
  device_config_t* new_config_ptr;
  uint8_t data_len = LMIC.dataLen - 1; //First byte is the command
  uint8_t port = *(data_ptr-1);
  UNUSED(port);
  uint8_t command = *data_ptr;
  switch (command)
  {
  case 0x00:
    log_debug_ln(F("SENDING CONFIG"));
    log_debug_ln(sizeof(device_config));
    lorawan_send((uint8_t*)(&device_config), sizeof(device_config));
    break;
  
  case 0x01:
    log_debug_ln(F("RESET CONFIG"));
    init_device_config(true);
    log_debug_ln(sizeof(device_config));
    lorawan_send((uint8_t*)(&device_config), sizeof(device_config));
    break;
  
  case 0x02:
    log_debug_ln(F("RECEIVING CONFIG"));
    new_config_ptr = (device_config_t*)(data_ptr+1);

    if (data_len != sizeof(device_config_t)) {
      log_error(F("Invalid length for new config: "));
      log_error_ln(data_len);
      return;
    }

    if (new_config_ptr->version_config != device_config.version_config) {
      log_error(F("Invalid config version: "));
      log_error_ln(new_config_ptr->version_config);
      return;
    }

    write_device_config(*(device_config_t*) new_config_ptr);
    lorawan_send((uint8_t*)(&device_config), sizeof(device_config));
    break;
  
  case 0x05:
    log_debug_ln(F("SENDING DEV UID"));
    lorawan_send((uint8_t*)UID_BASE, UID_BYTES_LENGTH);
    break;
  
  case 0x06:
    log_debug_ln(F("SENDING GIT COMMIT"));
    lorawan_send((uint8_t*)(&GIT_COMMIT_ID), sizeof(GIT_COMMIT_ID));
    break;
  
  case 0x07:
    log_debug_ln(F("SENDING BUILD DATE"));
    lorawan_send((uint8_t*)(&__DATE__), sizeof(__DATE__));
    break;
  
  default:
    log_error(F("Unknown command: "));
    log_debug_ln(command);
  }

  log_info(millis());
  log_info_ln(F("Command processed"));
  log_flush();

}

/* **************************************************************
 * send the message
 * *************************************************************/
void lorawan_send(uint8_t* data, u1_t dlen, uint8_t port) {


  log_info(millis());
  log_info(F(" Sending "));
  log_info(dlen);
  log_info(F(" bytes..."));


  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    log_warning(F("OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    lmic_tx_error_t err = LMIC_setTxData2(port, (xref2u1_t) data, dlen, 0);
    if (err) {
      log_error(F("ERROR sending: "));
      log_error_ln(err);
      return;
    }
  }

  // wait for send to complete
  log_debug(millis());
  log_debug(F(" Waiting.. "));


  while ( (LMIC.opmode & OP_JOINING) or (LMIC.opmode & OP_TXDATA) or (LMIC.opmode & OP_TXRXPEND) ) {
    os_sleep();
    os_runloop_once();
    //HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    //HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  }

  log_info(millis());
  log_info_ln(F(" TX_COMPLETE"));
  log_flush();

  lorawan_process_command();
}



void lorawan_suspend() {

  //Sleep radio - just in case
  os_radio(RADIO_RST);

  // //Disable SPI
  SPI.end();
}

void lorawan_resume() {

  //Enable SPI
  SPI.begin();

  //Reset dutty cycle -> assuming we sleep for minutes
  LMIC.bands[BAND_MILLI].avail = os_getTime();
  LMIC.bands[BAND_CENTI].avail = os_getTime();
  LMIC.bands[BAND_DECI].avail = os_getTime();

  //at least check once
  os_runloop_once();
}


/* **************************************************************
 * Print event for debug
 * *************************************************************/
#if DEBUG
  static void printEvent(ev_t ev) {};
#else
  static void printHex2(unsigned v) {
      v &= 0xff;
      if (v < 16)
          log_debug('0');
      log_debug(v, HEX);
  }

  static void printEvent(ev_t ev) {
      log_debug(millis());
      log_debug(": ");
      switch(ev) {
          case EV_SCAN_TIMEOUT:
              log_debug_ln(F("EV_SCAN_TIMEOUT"));
              break;
          case EV_BEACON_FOUND:
              log_debug_ln(F("EV_BEACON_FOUND"));
              break;
          case EV_BEACON_MISSED:
              log_debug_ln(F("EV_BEACON_MISSED"));
              break;
          case EV_BEACON_TRACKED:
              log_debug_ln(F("EV_BEACON_TRACKED"));
              break;
          case EV_JOINING:
              log_debug_ln(F("EV_JOINING"));
              break;
          case EV_JOIN_TXCOMPLETE:
              log_debug_ln(F("EV_JOIN_TXCOMPLETE"));
              break;
          case EV_JOINED:
              log_debug_ln(F("EV_JOINED"));
              {
                u4_t netid = 0;
                devaddr_t devaddr = 0;
                u1_t nwkKey[16];
                u1_t artKey[16];
                LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
                log_debug("netid: ");
                log_debug_ln(netid, HEX);
                log_debug("devaddr: ");
                log_debug_ln(devaddr, HEX);
                log_debug("AppSKey: ");
                for (size_t i=0; i<sizeof(artKey); ++i) {
                  if (i != 0)
                    log_debug("-");
                  printHex2(artKey[i]);
                }
                log_debug_ln("");
                log_debug("NwkSKey: ");
                for (size_t i=0; i<sizeof(nwkKey); ++i) {
                        if (i != 0)
                                log_debug("-");
                        printHex2(nwkKey[i]);
                }
                log_debug_ln();
              }
              break;
          /*
          || This event is defined but not used in the code. No
          || point in wasting codespace on it.
          ||
          || case EV_RFU1:
          ||     log_debug_ln(F("EV_RFU1"));
          ||     break;
          */
          case EV_JOIN_FAILED:
              log_debug_ln(F("EV_JOIN_FAILED"));
              break;
          case EV_REJOIN_FAILED:
              log_debug_ln(F("EV_REJOIN_FAILED"));
              break;
          case EV_TXCOMPLETE:
              log_debug_ln(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
              if (LMIC.txrxFlags & TXRX_ACK)
                log_debug_ln(F("Received ack"));
              if (LMIC.dataLen) {
                log_debug(F("Received "));
                log_debug(LMIC.dataLen);
                log_debug_ln(F(" bytes of payload"));
              }
              break;
          case EV_LOST_TSYNC:
              log_debug_ln(F("EV_LOST_TSYNC"));
              break;
          case EV_RESET:
              log_debug_ln(F("EV_RESET"));
              break;
          case EV_RXCOMPLETE:
              // data received in ping slot
              log_debug_ln(F("EV_RXCOMPLETE"));
              break;
          case EV_LINK_DEAD:
              log_debug_ln(F("EV_LINK_DEAD"));
              break;
          case EV_LINK_ALIVE:
              log_debug_ln(F("EV_LINK_ALIVE"));
              break;
          /*
          || This event is defined but not used in the code. No
          || point in wasting codespace on it.
          ||
          || case EV_SCAN_FOUND:
          ||    log_debug_ln(F("EV_SCAN_FOUND"));
          ||    break;
          */
          case EV_TXSTART:
              log_debug_ln(F("EV_TXSTART"));
              break;
          default:
              log_debug(F("Unknown event: "));
              log_debug_ln((unsigned) ev);
              break;
      }
  }
#endif

/* **************************************************************
 * Events
 * *************************************************************/
void onEvent (ev_t ev) {

    printEvent(ev);
    switch(ev) {
        default:
            break;
    }
}
