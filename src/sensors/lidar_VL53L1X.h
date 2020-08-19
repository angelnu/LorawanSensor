#include <CayenneLPP.h>

#define DEFAULT_MIN_PERCENTAGE_DISTANCE_2_SEND 5;
#define LIDAR_MAX_DISTANCE_DM 40.00
#define DEFAULT_DISTANCE_OFFSET 100

void init_lidar(bool firstTime);

void stop_lidar();

bool measure_lidar();

void send_lidar(CayenneLPP& lpp);