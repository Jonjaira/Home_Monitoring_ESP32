#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "cJSON.h"
#include "lvgl.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

// External UI label declarations
extern lv_obj_t * ui_KitchenTempVal;
extern lv_obj_t * ui_KitchenHumidityVal;
extern lv_obj_t * ui_KitchenTempGradient;
extern lv_obj_t * ui_KitchenHumidityGradient;
extern lv_obj_t * ui_KitchenPresenceIndicator;

extern lv_obj_t * ui_LivingroomTempVal;
extern lv_obj_t * ui_LivingroomHumidityVal;
extern lv_obj_t * ui_LivingroomTempGradient;
extern lv_obj_t * ui_LivingroomHumidityGradient;
extern lv_obj_t * ui_LivingRoomPresenceIndicator;

extern lv_obj_t * ui_MasterBedroomTempVal;
extern lv_obj_t * ui_MasterBedroomHumidityVal;
extern lv_obj_t * ui_MasterBedroomTempGradient;
extern lv_obj_t * ui_MasterBedroomHumidityGradient;
extern lv_obj_t * ui_MasterBedroomPresenceIndicator;

extern lv_obj_t * ui_BedroomTempVal;
extern lv_obj_t * ui_BedroomHumidityVal;
extern lv_obj_t * ui_BedroomTempGradient;
extern lv_obj_t * ui_BedroomHumidityGradient;
extern lv_obj_t * ui_BedroomPresenceIndicator;

extern lv_obj_t * ui_LaundryRoomTempVal;
extern lv_obj_t * ui_LaundryRoomHumidityVal;
extern lv_obj_t * ui_LaundryRoomTempGradient;
extern lv_obj_t * ui_LaundryRoomHumidityGradient;
extern lv_obj_t * ui_LaundryRoomPresenceIndicator;

extern lv_obj_t * ui_OfficeTempVal;
extern lv_obj_t * ui_OfficeHumidityVal;
extern lv_obj_t * ui_OfficeTempGradient;
extern lv_obj_t * ui_OfficeHumidityGradient;
extern lv_obj_t * ui_OfficePresenceIndicator;

#define PORT                        3333
#define MAX_CLIENTS                 5

static const char *TAG = "example";
const char *rooms[] = {"Kitchen", "LivingRoom", "MainBedroom", "BedRoom", "Laundry", "Office"};

// Function to update labels based on room data
void update_label(const char *room, char *temp_str, int32_t temp, char *humidity_str, int32_t humidity, int occupancy) {
    lv_obj_t *temp_label = NULL;
    lv_obj_t *humidity_label = NULL;
    lv_obj_t *temp_gradient = NULL;
    lv_obj_t *humidity_gradient = NULL;
    lv_obj_t *presence_indicator = NULL;

    if (strcmp(room, "Kitchen") == 0) {
        temp_label = ui_KitchenTempVal;
        humidity_label = ui_KitchenHumidityVal;
        temp_gradient = ui_KitchenTempGradient;
        humidity_gradient = ui_KitchenHumidityGradient;
        presence_indicator = ui_KitchenPresenceIndicator;
    } else if (strcmp(room, "LivingRoom") == 0) {
        temp_label = ui_LivingroomTempVal;
        humidity_label = ui_LivingroomHumidityVal;
        temp_gradient = ui_LivingroomTempGradient;
        humidity_gradient = ui_LivingroomHumidityGradient;
        presence_indicator = ui_LivingRoomPresenceIndicator;
    } else if (strcmp(room, "MainBedroom") == 0) {
        temp_label = ui_MasterBedroomTempVal;
        humidity_label = ui_MasterBedroomHumidityVal;
        temp_gradient = ui_MasterBedroomTempGradient;
        humidity_gradient = ui_MasterBedroomHumidityGradient;
        presence_indicator = ui_MasterBedroomPresenceIndicator;
    } else if (strcmp(room, "BedRoom") == 0) {
        temp_label = ui_BedroomTempVal;
        humidity_label = ui_BedroomHumidityVal;
        temp_gradient = ui_BedroomTempGradient;
        humidity_gradient = ui_BedroomHumidityGradient;
        presence_indicator = ui_BedroomPresenceIndicator;
    } else if (strcmp(room, "Laundry") == 0) {
        temp_label = ui_LaundryRoomTempVal;
        humidity_label = ui_LaundryRoomHumidityVal;
        temp_gradient = ui_LaundryRoomTempGradient;
        humidity_gradient = ui_LaundryRoomHumidityGradient;
        presence_indicator = ui_LaundryRoomPresenceIndicator;
    } else if (strcmp(room, "Office") == 0) {
        temp_label = ui_OfficeTempVal;
        humidity_label = ui_OfficeHumidityVal;
        temp_gradient = ui_OfficeTempGradient;
        humidity_gradient = ui_OfficeHumidityGradient;
        presence_indicator = ui_OfficePresenceIndicator;
    }

    ESP_LOGI(TAG, "Temp: %s", temp_str);
    ESP_LOGI(TAG, "Hum: %s", humidity_str);
    ESP_LOGI(TAG, "Occup: %d", occupancy);

    // Update temperature, humidity, and occupancy status
    lv_label_set_text(temp_label, temp_str);
    lv_label_set_text(humidity_label, humidity_str);
    lv_bar_set_value(temp_gradient, temp, LV_ANIM_OFF);
    lv_bar_set_value(humidity_gradient, humidity, LV_ANIM_OFF);

    if (occupancy) {
        lv_obj_add_state(presence_indicator, LV_STATE_CHECKED);  // Assume LV_STATE_USER_1 indicates occupancy
    } else {
        lv_obj_clear_state(presence_indicator, LV_STATE_CHECKED); // Assume clearing the state indicates no occupancy
    }
}

// Function to parse and log JSON data
void parse_json_data(const char *json_data) {
    cJSON *json = cJSON_Parse(json_data);
    if (json == NULL) {
        ESP_LOGE(TAG, "Invalid JSON format");
        return;
    }

    for (int i = 0; i < sizeof(rooms)/sizeof(rooms[0]); i++) {
        cJSON *room = cJSON_GetObjectItem(json, rooms[i]);
        if (room) {
            cJSON *temp_sensor = cJSON_GetObjectItem(room, "TempSensor");
            if (temp_sensor) {
                cJSON *temperature = cJSON_GetObjectItem(temp_sensor, "Temperature");
                cJSON *humidity = cJSON_GetObjectItem(temp_sensor, "Humidity");
                cJSON *occupancy = cJSON_GetObjectItem(room, "Presence");

                // Convert values to strings
                char temp_str[16];
                char humidity_str[16];
                snprintf(temp_str, sizeof(temp_str), "%d", temperature->valueint);
                snprintf(humidity_str, sizeof(humidity_str), "%d", humidity->valueint);

                // Update corresponding labels, pass occupancy status
                update_label(rooms[i],
                             temp_str,
                             temperature->valueint,
                             humidity_str,
                             humidity->valueint,
                             !strcmp("true", occupancy->valuestring) ? 1 : 0);  // Default to 0 if occupancy not present
            }
        }
    }
    cJSON_Delete(json);
}

// Function to handle receiving data
static void receive_data(const int sock) {
    int len;
    char rx_buffer[512];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            rx_buffer[len] = 0; // Null-terminate received data
            ESP_LOGI(TAG, "Received JSON: \e[93m%s\e[0m", rx_buffer);
            parse_json_data(rx_buffer); // Parse JSON data
        }
    } while (len > 0);
}

void handle_client_task(void *pvParameters) {
    int sock = (int)pvParameters;
    receive_data(sock);
    shutdown(sock, 0);
    close(sock);
    vTaskDelete(NULL);
}

void tcp_server_task(void *pvParameters) {
    ESP_ERROR_CHECK(esp_netif_init());

    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = IPPROTO_IP;
    struct sockaddr_storage dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(PORT);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, MAX_CLIENTS);
    if (err != 0) {
        ESP_LOGE(TAG, "Error during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {
        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        if (xTaskCreate(handle_client_task, "handle_client", 4096, (void *)sock, tskIDLE_PRIORITY, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create client handling task");
            close(sock);
        }
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}
