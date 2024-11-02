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

extern lv_obj_t * ui_LivingroomTempVal;
extern lv_obj_t * ui_LivingroomHumidityVal;
extern lv_obj_t * ui_LaundryRoomTempGradient;
extern lv_obj_t * ui_LaundryRoomHumidityGradient;

extern lv_obj_t * ui_MasterBedroomTempVal;
extern lv_obj_t * ui_MasterBedroomHumidityVal;
extern lv_obj_t * ui_MasterBedroomTempGradient;
extern lv_obj_t * ui_MasterBedroomHumidityGradient;

extern lv_obj_t * ui_BedroomTempVal;
extern lv_obj_t * ui_BedroomHumidityVal;
extern lv_obj_t * ui_BedroomTempGradient;
extern lv_obj_t * ui_BedroomHumidityGradient;

extern lv_obj_t * ui_LaundryRoomTempVal;
extern lv_obj_t * ui_LaundryRoomHumidityVal;
extern lv_obj_t * ui_LaundryRoomTempGradient;
extern lv_obj_t * ui_LaundryRoomHumidityGradient;

extern lv_obj_t * ui_OfficeTempVal;
extern lv_obj_t * ui_OfficeHumidityVal;
extern lv_obj_t * ui_OfficeTempGradient;
extern lv_obj_t * ui_OfficeHumidityGradient;

#define PORT                        3333
#define MAX_CLIENTS                 5

static const char *TAG = "example";
const char *rooms[] = {"Kitchen", "LivingRoom", "MainBedroom", "BedRoom", "Laundry", "Office"};

// Function to update labels based on room data
void update_label(const char *room, char *temp_str, int32_t temp, const char *humidity_str, int32_t humidity) {
    if (strcmp(room, "Kitchen") == 0) {
        lv_label_set_text(ui_KitchenTempVal, temp_str);
        lv_label_set_text(ui_KitchenHumidityVal, humidity_str);
        lv_bar_set_value(ui_KitchenTempGradient, temp, LV_ANIM_OFF);
        lv_bar_set_value(ui_KitchenHumidityGradient, humidity, LV_ANIM_OFF);
    } else if (strcmp(room, "LivingRoom") == 0) {
        lv_label_set_text(ui_LivingroomTempVal, temp_str);
        lv_label_set_text(ui_LivingroomHumidityVal, humidity_str);
        lv_bar_set_value(ui_LaundryRoomTempGradient, temp, LV_ANIM_OFF);
        lv_bar_set_value(ui_LaundryRoomHumidityGradient, humidity, LV_ANIM_OFF);
    } else if (strcmp(room, "MainBedroom") == 0) {
        lv_label_set_text(ui_MasterBedroomTempVal, temp_str);
        lv_label_set_text(ui_MasterBedroomHumidityVal, humidity_str);
        lv_bar_set_value(ui_MasterBedroomTempGradient, temp, LV_ANIM_OFF);
        lv_bar_set_value(ui_MasterBedroomHumidityGradient, humidity, LV_ANIM_OFF);
    } else if (strcmp(room, "BedRoom") == 0) {
        lv_label_set_text(ui_BedroomTempVal, temp_str);
        lv_label_set_text(ui_BedroomHumidityVal, humidity_str);
        lv_bar_set_value(ui_BedroomTempGradient, temp, LV_ANIM_OFF);
        lv_bar_set_value(ui_BedroomHumidityGradient, humidity, LV_ANIM_OFF);
    } else if (strcmp(room, "Laundry") == 0) {
        lv_label_set_text(ui_LaundryRoomTempVal, temp_str);
        lv_label_set_text(ui_LaundryRoomHumidityVal, humidity_str);
        lv_bar_set_value(ui_LaundryRoomTempGradient, temp, LV_ANIM_OFF);
        lv_bar_set_value(ui_LaundryRoomHumidityGradient, humidity, LV_ANIM_OFF);
    } else if (strcmp(room, "Office") == 0) {
        lv_label_set_text(ui_OfficeTempVal, temp_str);
        lv_label_set_text(ui_OfficeHumidityVal, humidity_str);
        lv_bar_set_value(ui_OfficeTempGradient, temp, LV_ANIM_OFF);
        lv_bar_set_value(ui_OfficeHumidityGradient, humidity, LV_ANIM_OFF);
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

                // Convert values to strings
                char temp_str[16];
                char humidity_str[16];
                snprintf(temp_str, sizeof(temp_str), "%d", temperature->valueint);
                snprintf(humidity_str, sizeof(humidity_str), "%d", humidity->valueint);

                // Update corresponding labels
                update_label(rooms[i],
                             temp_str,
                             temperature->valueint,
                             humidity_str,
                             humidity->valueint);
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
            ESP_LOGI(TAG, "Received JSON: %s", rx_buffer);
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
