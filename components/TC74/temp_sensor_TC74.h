#pragma once
#include "driver/i2c.h"

#define TC74_READ_COMMAND           0x00            /* Command read for TC74*/ 
#define TC74_CONFIG_REGISTER        0x01
#define TC74_CONFIG_STANDBY         0x01
#define TC74_CONFIG_WAKEUP          0x00
#define I2C_MASTER_TX_BUF_DISABLE   0               /* I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0               /* I2C master doesn't need buffer */

/**
 *  @brief i2c master initialization
 * 
 *  @param i2cPort       I2C port to configure
 *  @param sdaPin        GPIO number used for I2C master data
 *  @param sclPin        GPIO number used for I2C master clock 
 *  @param clkSpeedHz    I2C master clock frequency
 * 
 *  @return ESP_OK Success
 *         ESP_ERR_INVALID_ARG Parameter error
 *         ESP_FAIL Driver installation error
*/
esp_err_t tc74_init(i2c_port_t i2cPort, int sdaPin, int sclPin, uint32_t clkSpeedHz);

/**
 *  @brief release resources used by the I2C driver when communication ends
 * 
 *      @param i2cPort   I2C master i2c port number - I2C port to delete
 * 
 *      @return ESP_OK Success
 *              ESP_ERR_INVALID_ARG Parameter error
*/
esp_err_t tc_74_free(i2c_port_t i2cPort);


esp_err_t tc74_standby(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut);

esp_err_t tc74_wakeup(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut);

bool tc74_is_temperature_ready(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut);

/**
 *  @brief
 * 
 *      @param i2cPort
 *      @param sensAddr
 *      @param timeOut
 *      @param pData
 * 
 *      @return 
*/
esp_err_t tc74_wakeup_and_read_temp(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData);


esp_err_t tc74_read_temp_after_cfg(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData);


/**
 *  @brief
 * 
 *      @param i2cPort
 *      @param sensAddr
 *      @param timeOut
 *      @param pData
 * 
 *      @return 
*/
esp_err_t tc74_read_temp_after_temp(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData);