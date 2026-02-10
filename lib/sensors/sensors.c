#include "sensors.h"


void init_i2c_sensor() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}


void init_aht20() {
    aht20_reset(I2C_PORT);
    aht20_init(I2C_PORT);
}


SensorReadings get_sensor_readings() {
    SensorReadings data;
    AHT20_Data aht;


    if (aht20_read(I2C_PORT, &aht)) {
        data.aht_temp = aht.temperature;
        data.humidity = aht.humidity;
    } else {
        data.aht_temp = 0;
        data.humidity = 0;
    }


    return data;
}