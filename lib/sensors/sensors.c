#include "sensors.h"

void init_i2c_sensor() {
    // Inicializa I2C0 para primeiro sensor AHT20
    i2c_init(I2C_PORT_0, 400 * 1000);
    gpio_set_function(I2C_SDA_0, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_0, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_0);
    gpio_pull_up(I2C_SCL_0);
    
    // Inicializa I2C1 para segundo sensor AHT20
    i2c_init(I2C_PORT_1, 400 * 1000);
    gpio_set_function(I2C_SDA_1, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_1, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_1);
    gpio_pull_up(I2C_SCL_1);
}

void init_aht20() {
    // Inicializa sensor 1 (I2C0)
    aht20_reset(I2C_PORT_0);
    aht20_init(I2C_PORT_0);
    
    // Inicializa sensor 2 (I2C1)
    aht20_reset(I2C_PORT_1);
    aht20_init(I2C_PORT_1);
}

SensorReadings get_sensor_readings() {
    SensorReadings data;
    AHT20_Data aht1, aht2;

    // Lê sensor 1 (I2C0)
    if (aht20_read(I2C_PORT_0, &aht1)) {
        data.aht_temp_1 = aht1.temperature;
        data.humidity_1 = aht1.humidity;
    } else {
        data.aht_temp_1 = 0;
        data.humidity_1 = 0;
    }

    // Lê sensor 2 (I2C1)
    if (aht20_read(I2C_PORT_1, &aht2)) {
        data.aht_temp_2 = aht2.temperature;
        data.humidity_2 = aht2.humidity;
    } else {
        data.aht_temp_2 = 0;
        data.humidity_2 = 0;
    }

    // Debug (descomente conforme necessário):
    // printf("AHT20 #1 - Temp: %.2f C, Humidity: %.2f %%\n", data.aht_temp_1, data.humidity_1);
    // printf("AHT20 #2 - Temp: %.2f C, Humidity: %.2f %%\n\n", data.aht_temp_2, data.humidity_2);

    return data;
}