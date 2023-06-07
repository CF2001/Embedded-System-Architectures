#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_timer.h"              // High Resolution Timer (ESP Timer)
#include <rom/ets_sys.h>            // Note that this is busy-waiting – it does not allow other tasks to run, it just burns CPU cycles.
#include "esp_http_client.h"
#include "esp_log.h"

#include "../components/wifi/wifi_station.h"
#include "../components/TC74/temp_sensor_TC74.h"


#define LED_GREEN  17
#define LED_YELLOW 27
#define LED_RED 26

#define IN_GPIO 14
#define OUT_GPIO 25
#define TRIGGER_PIN OUT_GPIO    // envia o sinal de 10 us para o trigger 
#define ECHO_PIN IN_GPIO        // recebe a resposta de tempo do echo 

#define TRIGGER_LOW_DELAY 2     // 2us
#define TRIGGER_HIGH_DELAY 10   // 10us

#define I2C_MASTER_SCL_IO   12
#define I2C_MASTER_SDA_IO   13
#define I2C_MASTER_FREQ_HZ  50000
#define TC74_SENSOR_ADDR    0x4D    // default value -  1001 101b

#define MAX_DIST 450 // cm

#define timeout_expired(start, len) ((esp_timer_get_time() - (start)) >= (len))

int8_t temperature_readings;

bool validDistance = true;
float distance = 0;
float mean_distances = 0;

bool sendDataToDash = false;

static const char *TAG_DIST =  "MEASURE_DIST";

static const char *TAG_HTTP = "HTTP_CLIENT";

static const char *TAG_TEMP = "TEMP_VALUE";

char writeAPIKey[] = "JKG5ZK4N29JTE8VR";
char deleteAPIKey[] = "CWA3NW2LPZJ8479U";

static i2c_port_t i2cPort = I2C_NUM_0;     /*!< I2C port 0 */


/**
 *  Configuring pins as input or output
*/
void configure_pins(void)
{
    // Initializing GPIO14 as input - receive the echo 
    gpio_reset_pin(IN_GPIO);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT); 
   
    // Initializing GPIO25 as output - send trigger
    gpio_reset_pin(OUT_GPIO);
    gpio_set_direction(TRIGGER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIGGER_PIN, 0);

    // Initializing GPIO17 as output - Led green 
    gpio_reset_pin(LED_GREEN);
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GREEN, 0);

    // Initializing GPIO27 as output - Led yellow
    gpio_reset_pin(LED_YELLOW);
    gpio_set_direction(LED_YELLOW, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_YELLOW, 0);

    // Initializing GPIO26 as output - Led red
    gpio_reset_pin(LED_RED);
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_RED, 0);
}


/**
 *  Clear a Channel Feed.
 * 
 *  HTTP Method : DELETE
 *  URL : https://api.thingspeak.com/channels/<channel_id>/feeds.<format>
 * 
 *  URL Parameters :  
 *                  <channel_id>  -  (Required) Channel ID for the channel of interest, specified as an integer.
 *                  <format>      -  (Required) Format for the HTTP response, specified as json or xml.
 * 
 *  Body Parameters: (Required) Specify User API Key, which you can find in your profile. This key is different from the channel API keys
 * 
 *  Content-Type : application/x-www-form-urlencoded
 *  
*/
void clear_data_from_dashboard(void)
{
    const char *url = "https://api.thingspeak.com/channels/2136416/feeds.json";
    esp_err_t err;

    esp_http_client_config_t config = {
		.url = url,
		.method = HTTP_METHOD_DELETE,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_header(client, "api_key", deleteAPIKey);
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


/**
 *  Write a Channel Feed.
*/
void send_data_to_dashboard(void *pvParameters)
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
        if (sendDataToDash)
        {
            sendDataToDash = false;

            strcpy(post_data, "");
            snprintf(post_data, sizeof(post_data), data, mean_distances);
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
                    ESP_LOGE(TAG_HTTP,"HTTP POST request failed: %s", esp_err_to_name(err));				
                }
            }
            else
            {
                ESP_LOGE(TAG_HTTP, "HTTP POST request failed: %s", esp_err_to_name(err));
            }

            // 500 /10
            //vTaskDelay(500 / portTICK_PERIOD_MS);  // vTasDelay(tick) 1tck = 10 ms -> 100tck = 500ms -> 0.5 s
        }
    }

    esp_http_client_cleanup(client);
    esp_http_client_close(client);
    vTaskDelete(NULL);
}

/**
 *  Detection of an object at short, medium and long range.
*/
void process_data(bool validDist)
{
    if (validDist)
    {
        ESP_LOGI(TAG_DIST, " Distance: %0.04f cm ", distance);

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

    }else{
            ESP_LOGE(TAG_DIST, " Invalid Distance. ");

            gpio_set_level(LED_GREEN, 1);
            gpio_set_level(LED_YELLOW, 1);
            gpio_set_level(LED_RED, 1);
    }
}

/**
 *  Temperature measurement every 0.1 s
*/
void temperature_measure(void *pvParameters)
{
    uint8_t temperature = 0;

    // First reading
    esp_err_t err = tc74_wakeup_and_read_temp(i2cPort, TC74_SENSOR_ADDR, pdMS_TO_TICKS(100)/ portTICK_PERIOD_MS, &temperature);

    if (err == ESP_OK) {
        temperature_readings = (int8_t)temperature;
        ESP_LOGI(TAG_TEMP, "Temperature: %d°C", (int8_t)temperature);
    } else {
        ESP_LOGE(TAG_TEMP, "Failed to read temperature: %d", err);
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100 ms = 0.1s between readings

    while (true) 
    {
        // Continuing to read the values
        err = tc74_read_temp_after_temp(i2cPort, TC74_SENSOR_ADDR, pdMS_TO_TICKS(100)/portTICK_PERIOD_MS, &temperature);

        if (err == ESP_OK) {
            temperature_readings = (int8_t)temperature;
            //ESP_LOGI(TAG_TEMP, "Temperature: %d°C", (int8_t)temperature);
        } else {
            ESP_LOGE(TAG_TEMP, "Failed to read temperature: %d", err);
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100 ms = 0.1s between readings
    }

    tc_74_free(i2cPort);
    vTaskDelete(NULL);
}


/**
 *  Distance measurement every 0.5 s
*/
void ultrasonic_measure(void *pvParameters)
{
    float timeout = 0;
    float speedOfSound = 0;

    float sumDistances = 0;
    float nMeasures = 0;

    vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100 ms = 0.1s between readings

    while(true)
    {
        validDistance = true;

        ESP_LOGI(TAG_DIST, "temperature: %dºC", temperature_readings);
        speedOfSound = (((331.5 + (0.6 * temperature_readings)) * 100) / 1000000); // m/s = m/s * 100 (cm/s) = cm/s / 1 000 000 (cm/us)
        // ESP_LOGI(TAG_DIST, "Speed of Sound: %f", speedOfSound);
        timeout = (MAX_DIST*2 / speedOfSound) + 1;
        // ESP_LOGI(TAG_DIST, "Timeout: %f", timeout);


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

        //ESP_LOGI(TAG_DIST, "Time echo is High: %ld", time_echo_isHigh);
        distance = ((speedOfSound * time_echo_isHigh) / 2); 

        process_data(validDistance);

        sumDistances += distance;   // guardar 10 medicoes 
        nMeasures++;

        //ESP_LOGI(TAG_DIST, "nMeasures: %f", nMeasures);
        //ESP_LOGI(TAG_DIST, "sumDistances: %f", sumDistances);
        if (nMeasures == 5)
        {
            mean_distances = sumDistances / 5;  // determinar a media das 5 medicos em 2.5 s
            sendDataToDash = true;
            //ESP_LOGI(TAG_DIST, "mean_distances: %0.04f cm", mean_distances);
            sumDistances = 0;
            nMeasures = 0;

            ESP_LOGI(TAG_DIST, "sumDistances: %f", sumDistances);
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // 0,5 s
    }

    vTaskDelete(NULL);
}


void app_main(void)
{
    configure_pins();

    // Initialize I2C device
    esp_err_t err = tc74_init(i2cPort, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TEMP, "Failed to initialize I2C device: %d", err);
        return;
    }
    ESP_LOGI(TAG_TEMP, "I2C device initialized successfully");
    
    
    if (err == ESP_OK) 
    {
        xTaskCreate(&temperature_measure, "temperature_measure", 2048, NULL, 6, NULL);  // temperature  - every 0.1 s
        xTaskCreate(&ultrasonic_measure, "ultrasonic_measure", 2048, NULL, 6, NULL);    // distances - every 0.5 s
    }

    init_NVS();
    wifi_init_sta();

    if (wifi_connect_status)
    {
        //clear_data_from_dashboard();
        xTaskCreate(&send_data_to_dashboard, "send_data_to_dashboard", 8192, NULL, 6, NULL); 
    }
}
