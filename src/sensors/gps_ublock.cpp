#include "config.h"
#ifdef SENSOR_GPS_UBLOCK

#ifndef GPS_MIN_SATELITES
  #define GPS_MIN_SATELITES 4
#endif

#include "sensor.h"
#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
#include "IWatchdog.h"
class Sensor_gps: Sensor{
    private:
        //void set_config(device_config_device_t& specific_device_config) override;
        void init (bool firstTime) override;
        bool measure_intern() override;
        void send(CayenneLPP& lpp) override;
        void stop () override;

        SFE_UBLOX_GPS ivGPS;
        struct minfoStructure // Structure to hold the module info (uses 341 bytes of RAM)
        {
            char swVersion[30];
            char hwVersion[10];
            uint8_t extensionNo = 0;
            char extension[10][30];
        } minfo;

        float latitude_d;
        float old_latitude_d = 0;
        float longitude_d;
        float old_longitude_d=0;
        float altitude_m;
        float old_altitude_m=0;
        byte SIV;
        byte old_SIV=0;

        int wait_for_pos(size_t timeout_s, size_t satelites=GPS_MIN_SATELITES);
        bool getModuleInfo(uint16_t maxWait = 1100); //Queries module, texts
};
static Sensor_gps sensor;





int Sensor_gps::wait_for_pos(size_t timeout_s, size_t satelites) {
    //Wait for reception
    while (ivGPS.getSIV() < satelites) {
        log_info(timeout_s);
        log_info(F(" - Waiting for SIV: currently "));
        log_info_ln(ivGPS.getSIV());
        #ifdef PIN_STATUS_LED
            pinMode(PIN_STATUS_LED, OUTPUT);
            digitalWrite(PIN_STATUS_LED, HIGH);
            delay(100);
            digitalWrite(PIN_STATUS_LED, LOW);
            delay(500);
            digitalWrite(PIN_STATUS_LED, HIGH);
            delay(100);
            digitalWrite(PIN_STATUS_LED, LOW);
        #endif
        #ifdef IWDG_TIME_S
            IWatchdog.reload();
        #endif
        if (timeout_s >= 1) {
            delay(1000);
            timeout_s -= 1;
        } else {
            return -1;
        }
    }
    log_debug(F("Detected satelites: "));
    log_debug_ln(ivGPS.getSIV());
    return 0;
}



void Sensor_gps::init(bool firstTime) {

    pinMode(PIN_GPS_POWER, INPUT_PULLDOWN);
    
    pinMode(PIN_GPS_BAT_POWER, OUTPUT);
    digitalWrite(PIN_GPS_BAT_POWER, HIGH);
    delay(1000);
    
    if (ivGPS.begin() == false) //Connect to the Ublox module using Wire port
    {
        log_error_ln(F("Ublox GPS not detected."));
        while (1);
    }

    ivGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
    //myGPS.setNMEAOutputPort(Serial);
    //myGPS.saveConfiguration(); //Save the current settings to flash and BBR

  
    if (firstTime) {
        log_info_ln(F("GPS ublock found!"));
        //Print module info
        if (getModuleInfo(1100) == false) // Try to get the module info
        {
            log_error_ln(F("getModuleInfo failed! Freezing..."));
            while (1)
                ;
        }

        log_info_ln();
        log_info_ln(F("GPS Module Info : "));
        log_info(F("Soft version: "));
        log_info_ln(minfo.swVersion);
        log_info(F("Hard version: "));
        log_info_ln(minfo.hwVersion);
        log_info(F("Extensions:"));
        log_info_ln(minfo.extensionNo);
        for (int i = 0; i < minfo.extensionNo; i++)
        {
            log_info("  ");
            log_info_ln(minfo.extension[i]);
        }
        log_info_ln();
    }

  if (firstTime) {
    wait_for_pos(60);
  } else
  {
    wait_for_pos(10);
  }     
    
}

void Sensor_gps::stop() {

  pinMode(PIN_GPS_POWER, INPUT_PULLUP);

}

bool Sensor_gps::measure_intern() {

  ivGPS.checkUblox();
  latitude_d  = 0.0000001 * ivGPS.getLatitude();
  longitude_d = 0.0000001 * ivGPS.getLongitude();
  altitude_m  = 0.001 * ivGPS.getAltitude();
  SIV = ivGPS.getSIV();

  //Debug output
  log_debug(F("Lat (degrees):  "));
  log_debug_ln(latitude_d, 7);
  log_debug(F("Long (degrees):  "));
  log_debug_ln(longitude_d, 7);
  log_debug(F("Alt (m):  "));
  log_debug_ln(altitude_m);
  log_debug(F("SIV:  "));
  log_debug_ln(SIV);

  //Find if it a value has changed enough
  return (
       true
     );
}

void Sensor_gps::send(CayenneLPP& lpp) { 
  //Add measurements and remember last transmit
  if (SIV >= GPS_MIN_SATELITES) {
      lpp.addGPS(SENSOR_GPS_POS_CHANNEL, latitude_d, longitude_d, altitude_m);
  }
  old_latitude_d = latitude_d;
  old_longitude_d = longitude_d;
  old_altitude_m = altitude_m;
  lpp.addDigitalInput(SENSOR_GPS_SIV_CHANNEL, SIV);
  old_SIV = SIV;
}

bool Sensor_gps::getModuleInfo(uint16_t maxWait)
{
    minfo.hwVersion[0] = 0;
    minfo.swVersion[0] = 0;
    for (int i = 0; i < 10; i++)
        minfo.extension[i][0] = 0;
    minfo.extensionNo = 0;

    // Let's create our custom packet
    uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

    // The next line creates and initialises the packet information which wraps around the payload
    ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

    // The structure of ubxPacket is:
    // uint8_t cls           : The message Class
    // uint8_t id            : The message ID
    // uint16_t len          : Length of the payload. Does not include cls, id, or checksum bytes
    // uint16_t counter      : Keeps track of number of overall bytes received. Some responses are larger than 255 bytes.
    // uint16_t startingSpot : The counter value needed to go past before we begin recording into payload array
    // uint8_t *payload      : The payload
    // uint8_t checksumA     : Given to us by the module. Checked against the rolling calculated A/B checksums.
    // uint8_t checksumB
    // sfe_ublox_packet_validity_e valid            : Goes from NOT_DEFINED to VALID or NOT_VALID when checksum is checked
    // sfe_ublox_packet_validity_e classAndIDmatch  : Goes from NOT_DEFINED to VALID or NOT_VALID when the Class and ID match the requestedClass and requestedID

    // sendCommand will return:
    // SFE_UBLOX_STATUS_DATA_RECEIVED if the data we requested was read / polled successfully
    // SFE_UBLOX_STATUS_DATA_SENT     if the data we sent was writted successfully (ACK'd)
    // Other values indicate errors. Please see the sfe_ublox_status_e enum for further details.

    // Referring to the u-blox M8 Receiver Description and Protocol Specification we see that
    // the module information can be read using the UBX-MON-VER message. So let's load our
    // custom packet with the correct information so we can read (poll / get) the module information.

    customCfg.cls = UBX_CLASS_MON; // This is the message Class
    customCfg.id = UBX_MON_VER;    // This is the message ID
    customCfg.len = 0;             // Setting the len (length) to zero let's us poll the current settings
    customCfg.startingSpot = 0;    // Always set the startingSpot to zero (unless you really know what you are doing)

    // Now let's send the command. The module info is returned in customPayload

    if (ivGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED)
        return (false); //If command send fails then bail

    // Now let's extract the module info from customPayload

    uint16_t position = 0;
    for (int i = 0; i < 30; i++)
    {
        minfo.swVersion[i] = customPayload[position];
        position++;
    }
    for (int i = 0; i < 10; i++)
    {
        minfo.hwVersion[i] = customPayload[position];
        position++;
    }

    while (customCfg.len >= position + 30)
    {
        for (int i = 0; i < 30; i++)
        {
            minfo.extension[minfo.extensionNo][i] = customPayload[position];
            position++;
        }
        minfo.extensionNo++;
        if (minfo.extensionNo > 9)
            break;
    }

    return (true); //Success!
}
#endif