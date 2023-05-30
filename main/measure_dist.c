#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_timer.h"      // High Resolution Timer (ESP Timer)
//#include <esp32/rom/ets_sys.h>
#include <rom/ets_sys.h>    // Note that this is busy-waiting – it does not allow other tasks to run, it just burns CPU cycles.

#include "wifi_station.h"


#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG_HTTP = "HTTP_CLIENT";
char POST_api_key[] = "JKG5ZK4N29JTE8VR";

static const char *TAG =  "MEASURES";

#define LED_GREEN  17
#define LED_YELLOW 27
#define LED_RED 26

#define IN_GPIO 14
#define OUT_GPIO 25
#define TRIGGER_PIN OUT_GPIO    // envia o sinal de 10 us para o trigger 
#define ECHO_PIN IN_GPIO        // recebe a resposta de tempo do echo 

#define TRIGGER_LOW_DELAY 2     // 2us
#define TRIGGER_HIGH_DELAY 10   // 10us

#define ROUNDTRIP_M 5800.0f

float distance = 0;
float speedOfSound = 0;
float temperature = 20;


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

static void ultrasonic_measure(void *pvParameters)
{
    speedOfSound = (((331.5 + (0.6 * temperature)) * 100) / 1000000); // m/s = m/s * 100 (cm/s) = cm/s / 1 000 000 (cm/us)
    //ESP_LOGI(TAG, "Speed of Sound: %f", speedOfSound);

    while(true)
    {
        // Clears the trigger Pin
        gpio_set_level(TRIGGER_PIN, 0); 
        ets_delay_us(TRIGGER_LOW_DELAY);    // delay 2us    

        // Send a trigger pulse (10us) to the HC-SR04P
        gpio_set_level(TRIGGER_PIN, 1);
        ets_delay_us(TRIGGER_HIGH_DELAY);   // delay 10us
        gpio_set_level(TRIGGER_PIN, 0);

        // Wait for the echo signal
        // int64_t start = esp_timer_get_time();
        while (!gpio_get_level(ECHO_PIN));   // enquato estiver high 

        // got echo, measuring
        int64_t echo_start = esp_timer_get_time();  // Get time in microseconds since boot.
        int64_t time = echo_start;
        while (gpio_get_level(ECHO_PIN))    // while is high
        {
            time = esp_timer_get_time();    
        }

        uint32_t time_echo_isHigh;
        time_echo_isHigh = time - echo_start;   // us (microsec)

        ESP_LOGI(TAG, "Speed of Sound: %f", speedOfSound);
        ESP_LOGI(TAG, "Time echo is High: %ld", time_echo_isHigh);
        distance = ((speedOfSound * time_echo_isHigh) / 2); 

        process_data();

        //distance = (time_us / ROUNDTRIP_M) * 100;
        
        printf("Distance: %0.04f cm\n", distance);

        vTaskDelay(pdMS_TO_TICKS(500)); // 0,5 s
    }

    vTaskDelete(NULL);
}

void app_main()
{
    configure_pins();

    xTaskCreate(&ultrasonic_measure, "ultrasonic_measure", 2048, NULL, 5, NULL);
    //xTaskCreate(&process_data, "process_data", 2048, NULL, 5, NULL);

    // init_NVS();
    // wifi_init_sta();

    // if (wifi_connect_status)
    // {
    //     //clear_data_from_dashboard();
    //     xTaskCreate(&send_data_to_dashboard, "send_data_to_dashboard", 8192, NULL, 6, NULL); 
    // }

    //gpio_set_level(LED_GREEN, 0);
    //gpio_set_level(LED_YELLOW, 0);
    //gpio_set_level(LED_RED, 0);
}
