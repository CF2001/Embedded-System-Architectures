#include "wifi_station.h"

int wifi_connect_status = 0;
static int s_retry_num = 0;

void init_NVS(void)
{
    //printf("____ init_NVS ____\n");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    //printf("____ wifi_event_handler ____ \n");

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        printf("WiFi connecting ... \n");
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        printf("WiFi lost connection ... \n");
        wifi_connect_status = 0;

        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_WIFI, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG_WIFI,"connect to the AP fail");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        printf("WiFi got IP ... \n");
        wifi_connect_status = 1;

        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    //printf("____ wifi_INIT_STA ____ \n");

    s_wifi_event_group = xEventGroupCreate();

    // 1 - Wi-Fi/LwIP Init Phase
    ESP_ERROR_CHECK(esp_netif_init());                  // Initialize the underlying TCP/IP stack.
    ESP_ERROR_CHECK(esp_event_loop_create_default());   // event loop - system events can be sent to event task     
    esp_netif_create_default_wifi_sta();                // Creates default WIFI STA. In case of any init error this API aborts.

    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_initiation));   // Initialize WiFi Allocate resource for WiFi driver.

    // 2 -Wi-Fi Configuration Phase
    // event_base / event_id / event_handler / *event_handler_arg
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    //ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configuration));

    // 3- Wi-Fi Start Phase
    ESP_ERROR_CHECK(esp_wifi_start());  

    ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");

    /** Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /** xEventGroupWaitBits() returns the bits before the call returned, 
    *   hence we can test which event actually happened. 
    */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_WIFI, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG_WIFI, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
    }
}