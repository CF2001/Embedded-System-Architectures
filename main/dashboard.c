#include "dashboard.h"


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

    int i = 0;
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

        i++;
        if (i == 50)
            break;
        // 500 /10
        //vTaskDelay(500 / portTICK_PERIOD_MS);  // vTasDelay(tick) 1tck = 10 ms -> 100tck = 500ms -> 0.5 s
    }
    
    //exit:
    esp_http_client_cleanup(client);
    esp_http_client_close(client);
    vTaskDelete(NULL);
}