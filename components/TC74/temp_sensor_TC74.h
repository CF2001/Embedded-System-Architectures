#pragma once
#include "driver/i2c.h"

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
 *      @param i2cPort  I2C port to delete
 * 
 *      @return ESP_OK Success
 *              ESP_ERR_INVALID_ARG Parameter error
*/
esp_err_t tc_74_free(i2c_port_t i2cPort);


/**
 *  @brief  Enable standby mode 
 * 
 *      @param i2cPort      I2C port number to perform the transfer on
 *      @param sensAddr     I2C device’s 7-bit address
 *      @param timeOut      Maximum ticks to wait before issuing a timeout
 * 
 *      @return ESP_OK Success
 *              ESP_ERR_INVALID_ARG Parameter error
 *              ESP_FAIL Sending command error, slave hasn’t ACK the transfer.
 *              ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *              ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
*/
esp_err_t tc74_standby(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut);

/**
 *  @brief  Start configurations to read the first value
 * 
 *      @param i2cPort      I2C port number to perform the transfer on
 *      @param sensAddr     I2C device’s 7-bit address
 *      @param timeOut      Maximum ticks to wait before issuing a timeout
 * 
 *      @return ESP_OK Success
 *              ESP_ERR_INVALID_ARG Parameter error
 *              ESP_FAIL Sending command error, slave hasn’t ACK the transfer.
 *              ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *              ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
*/
esp_err_t tc74_wakeup(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut);

/**
 *  @brief  Indicates if the temperature is ready to be read
 * 
 *      @param i2cPort      I2C port number to perform the transfer on
 *      @param sensAddr     I2C device’s 7-bit address
 *      @param timeOut      Maximum ticks to wait before issuing a timeout
 * 
 *      @return True if temperature is ready to be read, false otherwise
*/
bool tc74_is_temperature_ready(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut);

/**
 *  @brief Read the first temperature value.
 * 
 *      @param i2cPort      I2C port to configure
 *      @param sensAddr     I2C device’s 7-bit address
 *      @param timeOut      Maximum ticks to wait before issuing a timeout.
 *      @param pData        read temperature
 * 
 *      @return 
*/
esp_err_t tc74_wakeup_and_read_temp(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData);


/**
 *  @brief  Read temperature value after configure register.
 * 
 *      @param i2cPort  I2C port number to perform the transfer on
 *      @param sensAddr I2C device’s 7-bit address
 *      @param timeOut  Maximum ticks to wait before issuing a timeout.
 *      @param pData    Buffer to store the bytes received on the bus
 * 
 *       @return    ESP_OK Success
 *                  ESP_ERR_INVALID_ARG Parameter error
 *                  ESP_FAIL Sending command error, slave hasn’t ACK the transfer.
 *                  ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *                  ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
*/
esp_err_t tc74_read_temp_after_cfg(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData);


/**
 *  @brief  Read temperature value after other readings.
 * 
 *      @param i2cPort  I2C port number to perform the transfer on
 *      @param sensAddr I2C device’s 7-bit address
 *      @param timeOut  Maximum ticks to wait before issuing a timeout.
 *      @param pData    Buffer to store the bytes received on the bus
 * 
*       @return    ESP_OK Success
*                  ESP_ERR_INVALID_ARG Parameter error
*                  ESP_FAIL Sending command error, slave hasn’t ACK the transfer.
*                  ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
*                  ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
*/
esp_err_t tc74_read_temp_after_temp(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData);