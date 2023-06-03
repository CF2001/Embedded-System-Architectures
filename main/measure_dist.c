#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "temp_sensor_TC74.h"
#include "spi_25LC040A_eeprom.h"
#include "driver/i2c.h"
#include "esp_system.h"

#include "esp_timer.h"      // High Resolution Timer (ESP Timer)
//#include <esp32/rom/ets_sys.h>
#include <rom/ets_sys.h>    // Note that this is busy-waiting – it does not allow other tasks to run, it just burns CPU cycles.

#include "wifi_station.h"


#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG_HTTP = "HTTP_CLIENT";
char POST_api_key[] = "JKG5ZK4N29JTE8VR";

static const char *TAG =  "MEASURE_DIST";

#define LED_GREEN  17
#define LED_YELLOW 27
#define LED_RED 26

#define IN_GPIO 14
#define OUT_GPIO 25
#define TRIGGER_PIN OUT_GPIO    // envia o sinal de 10 us para o trigger 
#define ECHO_PIN IN_GPIO        // recebe a resposta de tempo do echo 

#define I2C_MASTER_SCL_IO   15
#define I2C_MASTER_SDA_IO   13
#define I2C_MASTER_FREQ_HZ  50000
#define TC74_SENSOR_ADDR    0x4D
#define CS_PIN              5
#define SCK_PIN             18
#define MOSI_PIN            23
#define MISO_PIN            19
#define CLK_SPEED_HZ        1000000

#define TRIGGER_LOW_DELAY 2     // 2us
#define TRIGGER_HIGH_DELAY 10   // 10us

#define MAX_DIST 450 // cm

bool validDistance = true;
float distance = 0;
float speedOfSound = 0;
//float temperature = 20;
float timeout = 0;

static const char *TAG = "TEMP_REGISTER";
static i2c_port_t i2c_port = I2C_NUM_0;
spi_device_handle_t spi_device;
spi_host_device_t masterHostId = VSPI_HOST;
TaskHandle_t xHandle = NULL;
bool end = false;
int8_t temperature_readings;

#define timeout_expired(start, len) ((esp_timer_get_time() - (start)) >= (len))


static void configure_pins(void)
{
    // Initializing GPIO14 as input 
    gpio_reset_pin(IN_GPIO);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT); 
   
    // Initializing GPIO25 and GPIO26 as output.
    gpio_reset_pin(OUT_GPIO);
    gpio_set_direction(TRIGGER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIGGER_PIN, 0);

    gpio_reset_pin(LED_GREEN);
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GREEN, 0);

    gpio_reset_pin(LED_YELLOW);
    gpio_set_direction(LED_YELLOW, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_YELLOW, 0);

    gpio_reset_pin(LED_RED);
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_RED, 0);
}

// URL https://api.thingspeak.com/channels/<channel_id>/feeds.<format>
static void clear_data_from_dashboard(void)
{
    // int channel_id = 2136416
    // char *format = "json";
    const char *url = "https://api.thingspeak.com/channels/2136416/feeds.json";
    esp_err_t err;

    esp_http_client_config_t config = {
		.url = url,
		.method = HTTP_METHOD_DELETE,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
    
    //esp_http_client_set_url(client, " api_key=LMQTSIQASRSQ9JST");
    esp_http_client_set_header(client, "api-key", "LMQTSIQASRSQ9JST");
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG_HTTP, "HTTP DELETE Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG_HTTP, "HTTP DELETE request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    esp_http_client_close(client);
}


// https://api.thingspeak.com/update?api_key=JKG5ZK4N29JTE8VR&field1=0
// POST FORMAT - https://api.thingspeak.com/update.<format>
static void send_data_to_dashboard(void *pvParameters)
{
    const char *url = "https://api.thingspeak.com";
    char data [] = "&field1=%0.04f";
    char post_data[200];
    esp_err_t err;

    esp_http_client_config_t config = {
		.url = url,
		.method = HTTP_METHOD_POST,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
    // formato do pacote de envio do post 
    esp_http_client_set_url(client, "/update?api_key=JKG5ZK4N29JTE8VR");
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    while(1)
    {
		strcpy(post_data, "");
        snprintf(post_data, sizeof(post_data), data, distance);
		ESP_LOGI(TAG_HTTP, "post = %s", post_data);
		esp_http_client_set_post_field(client, post_data, strlen(post_data)); 

		err = esp_http_client_perform(client);

		if (err == ESP_OK)
		{
			int status_code = esp_http_client_get_status_code(client);
			if (status_code == 200)
			{
				ESP_LOGI(TAG_HTTP, "Message sent Successfully");
			}
			else
			{
				ESP_LOGI(TAG_HTTP,"HTTP POST request failed: %s", esp_err_to_name(err));				
                //goto exit;
                break;
            }
		}
		else
		{
			ESP_LOGI(TAG_HTTP, "HTTP POST request failed: %s", esp_err_to_name(err));
            //goto exit;
            break;
        }

        // 500 /10
        vTaskDelay(500 / portTICK_PERIOD_MS);  // vTasDelay(tick) 1tck = 10 ms -> 100tck = 500ms -> 0.5 s
    }
    
    //exit:
    esp_http_client_cleanup(client);
    esp_http_client_close(client);
    vTaskDelete(NULL);
}

void process_data()
{
    if (distance >= 20)
    {
        gpio_set_level(LED_GREEN, 1);
        gpio_set_level(LED_YELLOW, 0);
        gpio_set_level(LED_RED, 0);

    }else if (distance < 20 && distance > 10)
    {
        gpio_set_level(LED_GREEN, 0);
        gpio_set_level(LED_YELLOW, 1);
        gpio_set_level(LED_RED, 0);

    }else if (distance <= 10) 
    {

        gpio_set_level(LED_GREEN, 0);
        gpio_set_level(LED_YELLOW, 0);
        gpio_set_level(LED_RED, 1);
    }

}

void read_temperature_task(void *param)
{
    esp_err_t ret_spi = spi_25LC040_init(masterHostId, CS_PIN, SCK_PIN, MOSI_PIN, MISO_PIN, CLK_SPEED_HZ, &spi_device);
    if (ret_spi != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI device: %d", ret_spi);
        return;
    }
    ESP_LOGI(TAG, "SPI device initialized successfully");

    // Enable write
    esp_err_t ret = spi_25LC040_write_enable(spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write enable");
        return;
    }

    // Write in status register
    ret = spi_25LC040_write_status(spi_device, 0x00); 
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write in status");
        return;
    }

    uint8_t page_data[16];
    int address = 0;
    TickType_t previousWakeTime;

    while (1) {
        previousWakeTime = xTaskGetTickCount();
        int32_t temperature_sum = 0;
        uint8_t temperature = 0;
        int num_readings = 3;

        // Wake up and do the first read
        esp_err_t err = tc74_wakeup_and_read_temp(i2c_port, TC74_SENSOR_ADDR, pdMS_TO_TICKS(100), &temperature);
        if (err == ESP_OK) {
            temperature_sum += (int8_t)temperature;
            // ESP_LOGI(TAG, "0 Temperature: %d°C", (int8_t)temperature);
        } else {
            ESP_LOGE(TAG, "Failed to read temperature: %d", err);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 ms between readings

        // Read the temperature 2 more times
        for (int i = 0; i < 2; i++) {
            temperature = 0;
            err = tc74_read_temp_after_temp(i2c_port, TC74_SENSOR_ADDR, pdMS_TO_TICKS(100), &temperature);

            if (err == ESP_OK) {
                temperature_sum += (int8_t)temperature;
                // ESP_LOGI(TAG, "%d Temperature: %d°C", i+1, (int8_t)temperature);
            } else {
                ESP_LOGE(TAG, "Failed to read temperature: %d", err);
            }

            vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 ms between readings
        }

        int8_t average_temperature = temperature_sum / num_readings;
        temperature_readings = average_temperature;
        ESP_LOGI(TAG, "Average Temperature: %d°C", average_temperature);

        // Put the sensor into standby mode
        esp_err_t standby_result = tc74_standby(i2c_port, TC74_SENSOR_ADDR, pdMS_TO_TICKS(100));
        if (standby_result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to put sensor into standby mode: %d", standby_result);
        }
        
        // Write to EEPROM
        if (address % 16 && address != 0) {
            esp_err_t ret = spi_25LC040_write_page(spi_device, address - 15, page_data, 16);

            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to write byte");
                return;
            }
        }
        
        
        if (address == 511) end = true;
        
        page_data[address%16] = average_temperature;

        vTaskDelay(pdMS_TO_TICKS(100));

        // Disable write
        ret = spi_25LC040_write_disable(spi_device);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write disable");
            return;
        }
        
        address++;
        
        vTaskDelayUntil(&previousWakeTime, pdMS_TO_TICKS(5000));
    }
}

/**
 *  Distance measurement every 0.5 s
*/
static void ultrasonic_measure(void *pvParameters)
{
    int8_t temperature = temperature_readings;
    speedOfSound = (((331.5 + (0.6 * temperature)) * 100) / 1000000); // m/s = m/s * 100 (cm/s) = cm/s / 1 000 000 (cm/us)
    ESP_LOGI(TAG, "Speed of Sound: %f", speedOfSound);
    timeout = (MAX_DIST*2 / speedOfSound) + 1;
    ESP_LOGI(TAG, "Timeout: %f", timeout);

    while(true)
    {
        validDistance = true;

        // Clears the trigger Pin
        gpio_set_level(TRIGGER_PIN, 0); 
        ets_delay_us(TRIGGER_LOW_DELAY);    // delay 2us    

        // Send a trigger pulse (10us) to the HC-SR04P
        gpio_set_level(TRIGGER_PIN, 1);
        ets_delay_us(TRIGGER_HIGH_DELAY);   // delay 10us
        gpio_set_level(TRIGGER_PIN, 0);

        // Wait for the echo signal to be high
        while (!gpio_get_level(ECHO_PIN));    // while ECHO is low

        // got echo, measuring
        int64_t echo_start = esp_timer_get_time();  // Get time in microseconds since boot.
        int64_t time = echo_start;
        while (gpio_get_level(ECHO_PIN))    // while is high
        {
            time = esp_timer_get_time(); 
            if (timeout_expired(echo_start, timeout))
            {
                validDistance = false;
                break;
            }   
        }

        uint32_t time_echo_isHigh;
        time_echo_isHigh = time - echo_start;   // us (microsec)

        ESP_LOGI(TAG, "Time echo is High: %ld", time_echo_isHigh);
        distance = ((speedOfSound * time_echo_isHigh) / 2); 

        if (validDistance)
        {
            ESP_LOGI(TAG, "Distance: %0.04f cm", distance);
            process_data();
        }else{
            ESP_LOGI(TAG, "Invalid Distance !! ");
            gpio_set_level(LED_GREEN, 1);
            gpio_set_level(LED_YELLOW, 1);
            gpio_set_level(LED_RED, 1);
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // 0,5 s
    }

    vTaskDelete(NULL);
}

void app_main()
{
    configure_pins();

    // Initialize I2C device
    esp_err_t ret_i2c = tc74_init(i2c_port, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
    if (ret_i2c != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C device: %d", ret_i2c);
        return;
    }
    ESP_LOGI(TAG, "I2C device initialized successfully");
    
    
    if (ret_i2c == ESP_OK) {
        xTaskCreate(read_temperature_task, "Read Temp", 2048, NULL, 5, &xHandle);
    }

    xTaskCreate(&ultrasonic_measure, "ultrasonic_measure", 2048, NULL, 6, NULL);

    // init_NVS();
    // wifi_init_sta();

    // if (wifi_connect_status)
    // {
    //     //clear_data_from_dashboard();
    //     xTaskCreate(&send_data_to_dashboard, "send_data_to_dashboard", 8192, NULL, 6, NULL); 
    // }

}
