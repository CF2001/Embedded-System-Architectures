#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_timer.h"
#include <esp32/rom/ets_sys.h>

#include "wifi_station.h"

#define IN_GPIO 14
#define OUT_GPIO 25
#define TRIGGER_PIN OUT_GPIO    // envia o sinal de 10 us para o trigger 
#define ECHO_PIN IN_GPIO        // recebe a resposta de tempo do echo 

#define TRIGGER_LOW_DELAY 4     // 4us = 0.004 ms 
#define TRIGGER_HIGH_DELAY 10   // 10us

#define ROUNDTRIP_M 5800.0f

float distance;


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
    while(true)
    {
        // Send a trigger pulse (10us) to the HC-SR04P
        gpio_set_level(TRIGGER_PIN, 0); // low = 4 us
        //vTaskDelay(pdMS_TO_TICKS((uint32_t) TRIGGER_LOW_DELAY ));  // ms -> tick (1tick = 10 ms)  ; 4us = 0.004 ms
        ets_delay_us(TRIGGER_LOW_DELAY);
        gpio_set_level(TRIGGER_PIN, 1);
        ets_delay_us(TRIGGER_HIGH_DELAY);
        gpio_set_level(TRIGGER_PIN, 0);

        // Wait for the echo signal
        // int64_t start = esp_timer_get_time();
        while (!gpio_get_level(ECHO_PIN));   // enquato estiver high 

        // got echo, measuring
        int64_t echo_start = esp_timer_get_time();
        int64_t time = echo_start;
        while (gpio_get_level(ECHO_PIN))
        {
            time = esp_timer_get_time();
        }

        uint32_t time_us;
        time_us = time - echo_start;
        distance = (time_us / ROUNDTRIP_M) * 100;
        
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
