#ifndef WIFI_STATION_H_
#define WIFI_STATION_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_SSID      "Redmi Note 11 Pro+ 5G"
#define WIFI_PASS      "eduroamdemo"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

static const char *TAG_WIFI_STA = "wifi station";

extern int wifi_connect_status;

/**
 *  Initialize NVS - Non-volatile storage partition of the flash memory.
 * 
 *  The Wi-Fi credentials are stored in the NVS partition as a key-value pair, where the SSID is 
 *  stored as the key and the password is stored as the value.
 *  The function ensures that the flash memory is ready to store new credentials.
*/
void init_NVS(void);

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void wifi_init_sta(void);

#endif

