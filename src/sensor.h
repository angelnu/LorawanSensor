#ifndef sensor_h
#define sensor_h

#include <CayenneLPP.h>

//Called once during setup
void sensor_setup();

bool sensor_measure(CayenneLPP& lpp);

#endif // sensor_h
