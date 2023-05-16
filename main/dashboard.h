#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_http_client.h"

static const char *TAG_HTTP = "HTTP_CLIENT";
char POST_api_key[] = "JKG5ZK4N29JTE8VR";
//char GET_api_key[] = "JKG5ZK4N29JTE8VR";

static void clear_data_from_dashboard(void);

static void send_data_to_dashboard(void *pvParameters);