#include "temp_sensor_TC74.h"


esp_err_t tc74_init(i2c_port_t i2cPort, int sdaPin, int sclPin, uint32_t clkSpeedHz)
{
    int i2c_master_port = i2cPort;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sdaPin,                   // select SDA GPIO specific to your project
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = sclPin,                   // select SCL GPIO specific to your project
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clkSpeedHz,         // select frequency specific to your project
        .clk_flags = 0,                         // optional; you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t tc_74_free(i2c_port_t i2cPort)
{
    i2c_driver_delete(i2cPort);
    return ESP_OK;
}

esp_err_t tc74_standby(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut)
{
    size_t write_size = 2;
    uint8_t write_buffer[2] = {0X01, 0x01};    // CONFIG_register / standby value - SHDN bit enabled 

    return i2c_master_write_to_device(i2cPort, sensAddr, write_buffer, write_size, timeOut);
}


esp_err_t tc74_wakeup(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut)
{
    size_t write_size = 2;
    uint8_t write_buffer[2] = {0X01, 0x00};    // CONFIG_register / wakeup value 

    return i2c_master_write_to_device(i2cPort, sensAddr, write_buffer, write_size, timeOut);
}

bool tc74_is_temperature_ready(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut)
{
    uint8_t config_register = 0x01;     //  Bytes to send on the bus
    uint8_t temperature;                //  Buffer to store the bytes received on the bus
    bool tempReady = false;

    i2c_master_write_read_device(i2cPort, sensAddr, &config_register, 1, &temperature, 1, timeOut);

    if ((temperature & 0x40) != 0) {  // ready = 1 / not ready = 0
        tempReady = true;
    }

    return tempReady;
}

esp_err_t tc74_wakeup_and_read_temp(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData)
{
    tc74_wakeup(i2cPort, sensAddr, timeOut);

    while (!tc74_is_temperature_ready(i2cPort, sensAddr, timeOut)) 
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);    // wait for temperature value
    }

    return tc74_read_temp_after_cfg(i2cPort, sensAddr, timeOut, pData);
}

esp_err_t tc74_read_temp_after_cfg(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData)
{
    uint8_t read_command = 0x00;
   
    return i2c_master_write_read_device(i2cPort, sensAddr, &read_command, 1, pData, 1, timeOut);
}

esp_err_t tc74_read_temp_after_temp(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData)
{
    return i2c_master_read_from_device(i2cPort, sensAddr, pData, 1, timeOut);
}