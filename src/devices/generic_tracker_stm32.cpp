#ifdef SENSOR_GENERIC_TRACKER_SMT32

#include "common_stm32.h"
#include "sensor.h"
#include "sleep.h"
#include <Wire.h>
#include <VL53L1X.h>

#define DEFAULT_MIN_PERCENTAGE_V_2_SEND 1;
#define MAX_VOLTAGE_V 3.3
#define DEFAULT_MIN_PERCENTAGE_T_2_SEND 10;
#define MAX_TEMP_C 100.0
#define DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND 5;
#define MAX_DISTANCE_DM 40.00


void set_device_specific_config(device_config_device_t& specific_device_config) {
  specific_device_config.min_percentage_v_2_send = DEFAULT_MIN_PERCENTAGE_V_2_SEND;
  specific_device_config.min_percentage_t_2_send = DEFAULT_MIN_PERCENTAGE_T_2_SEND;
  specific_device_config.min_percentage_distance_2_send = DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND;
}

VL53L1X lidar;
void init_lidar(bool firstTime){
  log_debug_ln("Init LIDAR");
  pinMode(PIN_LIDAR_POWER, OUTPUT);
  digitalWrite(PIN_LIDAR_POWER, HIGH);
  delay(2);

  lidar.setTimeout(500);
  if (!lidar.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1);
  }
  lidar.setDistanceMode(VL53L1X::Long);
  lidar.setMeasurementTimingBudget(50000);
  lidar.startContinuous(1000);

}
void stop_lidar(){

  digitalWrite(PIN_LIDAR_POWER, LOW);
}

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
// Extend the class for getModuleInfo
class SFE_UBLOX_GPS_ADD : public SFE_UBLOX_GPS
{
public:
    boolean getModuleInfo(uint16_t maxWait = 1100); //Queries module, texts

    struct minfoStructure // Structure to hold the module info (uses 341 bytes of RAM)
    {
        char swVersion[30];
        char hwVersion[10];
        uint8_t extensionNo = 0;
        char extension[10][30];
    } minfo;
};
SFE_UBLOX_GPS_ADD myGPS;
long lastTime = 0; //Simple local timer. Limits amount if I2C traffic to Ublox module.
void init_gps(bool firstTime) {
  if (myGPS.begin() == false) //Connect to the Ublox module using Wire port
  {
    log_error_ln(F("Ublox GPS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

  myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  //myGPS.setNMEAOutputPort(Serial);
  //myGPS.saveConfiguration(); //Save the current settings to flash and BBR

  
  if (firstTime) {
    Serial.print(F("Po{lling module info"));
      if (myGPS.getModuleInfo(1100) == false) // Try to get the module info
      {
          Serial.print(F("getModuleInfo failed! Freezing..."));
          while (1)
              ;
      }

      log_info_ln();
      log_info_ln(F("GPS Module Info : "));
      log_info(F("Soft version: "));
      log_info_ln(myGPS.minfo.swVersion);
      log_info(F("Hard version: "));
      log_info_ln(myGPS.minfo.hwVersion);
      log_info(F("Extensions:"));
      log_info_ln(myGPS.minfo.extensionNo);
      for (int i = 0; i < myGPS.minfo.extensionNo; i++)
      {
          log_info("  ");
          log_info_ln(myGPS.minfo.extension[i]);
      }
      log_info_ln();
    }
}

void stop_gps() {

}

void init_sensors(bool firstTime=false) {
  Wire.begin();
  //Wire.setClock(400000); // use 400 kHz I2C 
  //init_lidar(firstTime);
  init_gps(firstTime);
}

void stop_sensors() {
  stop_lidar();
  stop_gps();
  return; //TBD
  Wire.end();
  pinMode(PB6, OUTPUT);
  digitalWrite(PB6, LOW);
  pinMode(PB7, OUTPUT);
  digitalWrite(PB7, LOW);
}

//Called once during setup
void sensor_setup() {

  allInput();

  analogReadResolution(ADC_RESOLUTION);

  init_sensors( /*firstTime*/ true);
}



uint8_t skippedMeasurements = 0;
float old_battery_v = 0;
float old_temp_c = 0;
float old_distance_dm = 0;
float old_peak_signal_mcps=0;
float old_ambient_light_mcps=0;
bool sensor_measure(CayenneLPP& lpp){

  //Init sensor
  init_sensors();
    
  //Read sensors
  uint32_t VRef = readVref();
  float battery_v = 1.0 * VRef / 1000;
  float temp_c = readTempSensor(VRef);

  float distance_dm = 0.01;//TBD * lidar.read(); //Use dm to use at best the 2 decimals in Cayene analog
  float peak_signal_mcps = lidar.ranging_data.peak_signal_count_rate_MCPS;
  float ambient_light_mcps = lidar.ranging_data.ambient_count_rate_MCPS;
  myGPS.checkUblox();
  float latitude_d  = 0.0000001 * myGPS.getLatitude();
  float longitude_d = 0.0000001 * myGPS.getLongitude();
  float altitude_m  = 0.001 * myGPS.getAltitude();
  byte SIV = myGPS.getSIV();

  //Adjust distances
  if (distance_dm > MAX_DISTANCE_DM)
    distance_dm = MAX_DISTANCE_DM;
  switch(lidar.ranging_data.range_status){
    case lidar.RangeValid:
      break;
    case lidar.MinRangeFail:
      distance_dm = 0;
      break;
    case lidar.SignalFail:
    case lidar.RangeValidMinRangeClipped:
    case lidar.OutOfBoundsFail:
      distance_dm = MAX_DISTANCE_DM+1;
      break;
    default:
     distance_dm = -lidar.ranging_data.range_status;
  }

  //Stop sensor
  stop_sensors();
    
  //Debug output
  log_debug(F("BATTERY V: "));
  log_debug_ln(battery_v, 3);
  log_debug(F("TEMP C: "));
  log_debug_ln(temp_c, 1);
  log_debug(F("DISTANCE (dm): "));
  log_debug_ln(distance_dm, 1);
  log_debug(F("PEAK SIGNAL (MCPS): "));
  log_debug_ln(peak_signal_mcps, 1);
  log_debug(F("AMBIENT LIGHT (MCPS): "));
  log_debug_ln(ambient_light_mcps, 1);


  log_debug(F("Lat (degrees):  "));
  log_debug_ln(latitude_d, 7);
  log_debug(F("Long (degrees):  "));
  log_debug_ln(longitude_d, 7);
  log_debug(F("Alt (m):  "));
  log_debug_ln(altitude_m);
  log_debug(F("SIV:  "));
  log_debug_ln(SIV);

  //Find if it a value has changed enough
  bool enough_change = false;

  if ((100.0*(abs(battery_v - old_battery_v) / MAX_VOLTAGE_V) >= device_config.device.min_percentage_v_2_send) ||
      (100.0*(abs(temp_c - old_temp_c) / MAX_TEMP_C) >= device_config.device.min_percentage_t_2_send) ||
      (100.0*(abs(distance_dm - old_distance_dm) / MAX_DISTANCE_DM) >= device_config.device.min_percentage_distance_2_send) ||
      (skippedMeasurements >= device_config.max_skiped_measurements))
  {
    enough_change = true;
  }

  if (enough_change) {
    skippedMeasurements = 0; //Reset
    log_debug_ln(F("Sending measurement"));
    log_flush();
    lpp.reset();
    lpp.addDigitalInput(SENSOR_VERSION_CHANNEL, device_config.version);
    lpp.addAnalogInput(SENSOR_BATTERY_CHANNEL, battery_v);
    old_battery_v = battery_v;
    lpp.addTemperature(SENSOR_TEMP_CHANNEL, temp_c);
    old_temp_c = temp_c;
    lpp.addAnalogInput(SENSOR_DISTANCE_CHANNEL, distance_dm);
    old_distance_dm = distance_dm;
    lpp.addAnalogInput(SENSOR_PEAK_SIGNAL_CHANNEL, peak_signal_mcps);
    old_peak_signal_mcps = peak_signal_mcps;
    lpp.addLuminosity(SENSOR_LUMINOSITY_CHANNEL, ambient_light_mcps);
    old_ambient_light_mcps = ambient_light_mcps;
    lpp.addGPS(SENSOR_GPS_CHANNEL, latitude_d, longitude_d, altitude_m);
    old_ambient_light_mcps = ambient_light_mcps;
  } else {
    skippedMeasurements ++;

    log_debug(F("Skipped sending measurement: "));
    log_debug_ln(skippedMeasurements);
    log_flush();
  }

  return enough_change;
}

boolean SFE_UBLOX_GPS_ADD::getModuleInfo(uint16_t maxWait)
{
    myGPS.minfo.hwVersion[0] = 0;
    myGPS.minfo.swVersion[0] = 0;
    for (int i = 0; i < 10; i++)
        myGPS.minfo.extension[i][0] = 0;
    myGPS.minfo.extensionNo = 0;

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

    if (sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED)
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
