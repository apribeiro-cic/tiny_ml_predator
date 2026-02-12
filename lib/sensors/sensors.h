#ifndef SENSORS_H
#define SENSORS_H

#include "lib/aht20/aht20.h"
#include "hardware/i2c.h"
#include <math.h>

// I2C0 - Sensor AHT20 #1
#define I2C_PORT_0 i2c0
#define I2C_SDA_0 0
#define I2C_SCL_0 1

// I2C1 - Sensor AHT20 #2
#define I2C_PORT_1 i2c1
#define I2C_SDA_1 2
#define I2C_SCL_1 3

#define SEA_LEVEL_PRESSURE 102100.0

typedef struct {
    float aht_temp_1;   // Sensor 1
    float humidity_1;   // Sensor 1
    float aht_temp_2;   // Sensor 2
    float humidity_2;   // Sensor 2
} SensorReadings;

void init_i2c_sensor();
void init_aht20();
SensorReadings get_sensor_readings();

#endif