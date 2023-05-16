#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_timer.h"      // High Resolution Timer (ESP Timer)
//#include <esp32/rom/ets_sys.h>
#include <rom/ets_sys.h>    // Note that this is busy-waiting – it does not allow other tasks to run, it just burns CPU cycles.

#include "wifi_station.h"

static const char *TAG =  "MEASURES";

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
   
    // Initializing GPIO25 as output.
    gpio_reset_pin(OUT_GPIO);
    gpio_set_direction(TRIGGER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIGGER_PIN, 0);
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

    //init_NVS();
    //wifi_init_sta();

    // if (wifi_connect_status)
    // {
    //     //clear_data_from_dashboard();
    //     // xTaskCreate(&ultrasonic_measure, "ultrasonic_measure", 2048, NULL, 5, NULL);
    //     // xTaskCreate(&send_data_to_dashboard, "send_data_to_dashboard", 8192, NULL, 6, NULL);
    // }
}
