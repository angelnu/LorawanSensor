#include "config.h"
#ifdef SENSOR_ACCELEROMETER_LIS3DH

#include "accelerometer_LIS3DH.h"
#include <Wire.h>
#include <Adafruit_LIS3DH.h>

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

void init_accelerometer(bool firstTime){
  if (firstTime) {
    if (! lis.begin(0x19)) {   // change this to 0x19 for alternative i2c address
      log_error_ln(F("Failed to init accelerometer LIS3DH"));
      while (1) yield();
    }
    log_info_ln("Accelerometer LIS3DH found!");
  }
}

void stop_accelerometer() {

}

bool measure_accelerometer(){

  lis.read();      // get X Y and Z data at once
  // Then print out the raw data
  Serial.print("X:  "); Serial.print(lis.x);
  Serial.print("  \tY:  "); Serial.print(lis.y);
  Serial.print("  \tZ:  "); Serial.print(lis.z);

  /* Or....get a new sensor event, normalized */
  sensors_event_t event;
  lis.getEvent(&event);

  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("\t\tX: "); Serial.print(event.acceleration.x);
  Serial.print(" \tY: "); Serial.print(event.acceleration.y);
  Serial.print(" \tZ: "); Serial.print(event.acceleration.z);
  Serial.println(" m/s^2 ");

  Serial.println();
  return false;
}

void send_accelerometer(CayenneLPP& lpp) {

}

#endif