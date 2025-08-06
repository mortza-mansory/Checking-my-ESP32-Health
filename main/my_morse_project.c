//Wifi Testing
// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "driver/uart.h"
// #include "esp_http_server.h"
// #include "driver/gpio.h"

// // تگ‌های لاگ برای بخش‌های مختلف
// static const char *TAG_WIFI = "WIFI_TUI";
// static const char *TAG_WEB = "WEB_SERVER";

// // === پین LED ===
// #define LED_PIN GPIO_NUM_2

// // تعریف حالت‌های مختلف برنامه
// typedef enum {
//     STATE_INIT,
//     STATE_SCANNING,
//     STATE_DISPLAY_LIST,
//     STATE_GETTING_PASSWORD,
//     STATE_CONNECTING,
//     STATE_CONNECTED,
//     STATE_CONNECTION_FAILED
// } app_state_t;

// // متغیرهای سراسری برای مدیریت وضعیت
// static app_state_t current_state = STATE_INIT;
// static bool needs_redraw = true;
// static wifi_ap_record_t *ap_records;
// static uint16_t ap_count = 0;
// static int selected_index = 0;
// static char selected_ssid[33] = {0};
// static char password_buffer[65] = {0};
// static int password_len = 0;
// static char ip_address_str[16] = "N/A"; // برای ذخیره آدرس IP

// // === توابع وب‌سرور (کپی شده از گزینه ۱) ===

// static esp_err_t root_get_handler(httpd_req_t *req) {
//     const char* html_page = 
//         "<html><head><title>ESP32 LED Control</title>"
//         "<style>body{font-family:sans-serif;text-align:center;padding-top:50px; background-color:#2c3e50; color:white;}"
//         ".btn{display:inline-block;padding:15px 30px;font-size:24px;margin:10px;text-decoration:none;border-radius:10px;}"
//         ".on{background-color:#27ae60;color:white;}"
//         ".off{background-color:#c0392b;color:white;}"
//         "</style></head>"
//         "<body><h1>ESP32 LED Control Panel</h1>"
//         "<p>Control the onboard LED from this page.</p>"
//         "<a href='/led/on' class='btn on'>Turn LED ON</a>"
//         "<a href='/led/off' class='btn off'>Turn LED OFF</a>"
//         "</body></html>";
//     httpd_resp_send(req, html_page, strlen(html_page));
//     return ESP_OK;
// }

// static esp_err_t led_on_handler(httpd_req_t *req) {
//     gpio_set_level(LED_PIN, 1);
//     ESP_LOGI(TAG_WEB, "LED turned ON");
//     httpd_resp_set_status(req, "302 Found");
//     httpd_resp_set_hdr(req, "Location", "/");
//     httpd_resp_send(req, NULL, 0);
//     return ESP_OK;
// }

// static esp_err_t led_off_handler(httpd_req_t *req) {
//     gpio_set_level(LED_PIN, 0);
//     ESP_LOGI(TAG_WEB, "LED turned OFF");
//     httpd_resp_set_status(req, "302 Found");
//     httpd_resp_set_hdr(req, "Location", "/");
//     httpd_resp_send(req, NULL, 0);
//     return ESP_OK;
// }

// static const httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
// static const httpd_uri_t led_on_uri = { .uri = "/led/on", .method = HTTP_GET, .handler = led_on_handler };
// static const httpd_uri_t led_off_uri = { .uri = "/led/off", .method = HTTP_GET, .handler = led_off_handler };

// static void start_webserver(void) {
//     httpd_handle_t server = NULL;
//     httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//     ESP_LOGI(TAG_WEB, "Starting web server...");
//     if (httpd_start(&server, &config) == ESP_OK) {
//         httpd_register_uri_handler(server, &root_uri);
//         httpd_register_uri_handler(server, &led_on_uri);
//         httpd_register_uri_handler(server, &led_off_uri);
//         ESP_LOGI(TAG_WEB, "Web server started successfully.");
//     } else {
//         ESP_LOGE(TAG_WEB, "Error starting web server!");
//     }
// }


// // === توابع مدیریت رابط کاربری (UI Functions) ===

// void ui_clear_screen() { printf("\033[2J\033[H"); }

// void ui_display_wifi_list() {
//     ui_clear_screen();
//     printf("========== ESP32 WiFi Scanner ==========\n\n");
//     printf("Please select a WiFi network:\n");
//     printf("----------------------------------------\n");
//     for (int i = 0; i < ap_count; i++) {
//         if (i == selected_index) printf("\033[7m");
//         printf(" %-2d: %-32s (%d)\n", i + 1, (char *)ap_records[i].ssid, ap_records[i].rssi);
//         if (i == selected_index) printf("\033[0m");
//     }
//     printf("----------------------------------------\n");
//     printf("Use UP/DOWN arrows to navigate, ENTER to select.\n");
// }

// void ui_display_password_prompt() {
//     ui_clear_screen();
//     printf("========== ESP32 WiFi Connect ==========\n\n");
//     printf("Connecting to network: %s\n", selected_ssid);
//     printf("Please enter the password: %s\n", password_buffer);
// }


// // === توابع مدیریت وای‌فای (WiFi Functions) ===

// static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
//         ESP_LOGI(TAG_WIFI, "WiFi scan finished.");
//         esp_wifi_scan_get_ap_num(&ap_count);
//         if (ap_count > 0) {
//             if(ap_records) free(ap_records);
//             ap_records = (wifi_ap_record_t *)malloc(ap_count * sizeof(wifi_ap_record_t));
//             esp_wifi_scan_get_ap_records(&ap_count, ap_records);
//         }
//         current_state = STATE_DISPLAY_LIST;
//         needs_redraw = true;
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         ESP_LOGI(TAG_WIFI, "WiFi disconnected.");
//         if (current_state == STATE_CONNECTING) {
//             current_state = STATE_CONNECTION_FAILED;
//             needs_redraw = true;
//         }
//     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         sprintf(ip_address_str, IPSTR, IP2STR(&event->ip_info.ip));
//         ESP_LOGI(TAG_WIFI, "Got IP address: %s", ip_address_str);
//         current_state = STATE_CONNECTED;
//         needs_redraw = true;
//         // === تغییر کلیدی: وب‌سرور را اینجا استارت می‌زنیم! ===
//         start_webserver();
//     }
// }

// void wifi_init_and_scan() {
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_create_default_wifi_sta();
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_start());
//     wifi_scan_config_t scan_config = { .show_hidden = true };
//     ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, false));
//     current_state = STATE_SCANNING;
// }

// void wifi_connect() {
//     wifi_config_t wifi_config = { 0 };
//     strcpy((char*)wifi_config.sta.ssid, selected_ssid);
//     strcpy((char*)wifi_config.sta.password, password_buffer);
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//     esp_wifi_connect();
//     current_state = STATE_CONNECTING;
//     needs_redraw = true;
// }


// // === تابع اصلی برنامه (Main App) ===

// void app_main(void) {
//     // راه‌اندازی‌های اولیه
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     uart_config_t uart_config = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_DEFAULT };
//     ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0));
//     ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));

//     // راه‌اندازی پین LED
//     gpio_reset_pin(LED_PIN);
//     gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

//     // شروع فرآیند با اسکن وای‌فای
//     wifi_init_and_scan();

//     while (1) {
//         if (needs_redraw) {
//             switch (current_state) {
//                 case STATE_SCANNING:
//                     ui_clear_screen();
//                     printf("Scanning for WiFi networks, please wait...\n");
//                     break;
//                 case STATE_DISPLAY_LIST:
//                     ui_display_wifi_list();
//                     break;
//                 case STATE_GETTING_PASSWORD:
//                     ui_display_password_prompt();
//                     break;
//                 case STATE_CONNECTING:
//                     ui_clear_screen();
//                     printf("Connecting to %s...\n", selected_ssid);
//                     break;
//                 // === تغییر نمایش حالت اتصال موفق ===
//                 case STATE_CONNECTED:
//                     ui_clear_screen();
//                     printf("========== WiFi Connected! ==========\n\n");
//                     printf("  Network: %s\n", selected_ssid);
//                     printf("  IP Address: %s\n\n", ip_address_str);
//                     printf("Web Server is RUNNING.\n");
//                     printf("Open a web browser and go to the IP address\n");
//                     printf("to control the onboard LED.\n");
//                     // دیگر تسک را حذف نمی‌کنیم
//                     break;
//                 case STATE_CONNECTION_FAILED:
//                     ui_clear_screen();
//                     printf("\nCONNECTION FAILED!\nCould not connect to %s.\nPress any key to return to list...", selected_ssid);
//                     break;
//                 default: break;
//             }
//             needs_redraw = false;
//         }

//         // فقط زمانی ورودی کاربر را پردازش کن که هنوز وصل نشده‌ایم
//         if (current_state != STATE_CONNECTED) {
//             char c = 0;
//             if (uart_read_bytes(UART_NUM_0, &c, 1, pdMS_TO_TICKS(20)) > 0) {
//                 needs_redraw = true;
//                 if (current_state == STATE_DISPLAY_LIST) {
//                     if (c == '\033') { // Arrow keys
//                         char seq[2];
//                         if (uart_read_bytes(UART_NUM_0, seq, 2, pdMS_TO_TICKS(5)) == 2 && seq[0] == '[') {
//                             if (seq[1] == 'A') selected_index = (selected_index > 0) ? selected_index - 1 : ap_count - 1;
//                             else if (seq[1] == 'B') selected_index = (selected_index < ap_count - 1) ? selected_index + 1 : 0;
//                         }
//                     } else if (c == '\r' || c == '\n') { // Enter
//                         strncpy(selected_ssid, (char*)ap_records[selected_index].ssid, 32);
//                         current_state = STATE_GETTING_PASSWORD;
//                     }
//                 } else if (current_state == STATE_GETTING_PASSWORD) {
//                     if (c >= 32 && c <= 126 && password_len < 64) {
//                         password_buffer[password_len++] = c;
//                         password_buffer[password_len] = '\0';
//                     } else if ((c == '\b' || c == 127) && password_len > 0) {
//                         password_buffer[--password_len] = '\0';
//                     } else if (c == '\r' || c == '\n') { // Enter
//                         wifi_connect();
//                     }
//                 } else if (current_state == STATE_CONNECTION_FAILED) {
//                     password_len = 0;
//                     memset(password_buffer, 0, sizeof(password_buffer));
//                     selected_index = 0;
//                     current_state = STATE_DISPLAY_LIST;
//                 }
//             }
//         }
//         vTaskDelay(pdMS_TO_TICKS(50));
//     }
// }







//====================================================================================================================
//Blututh Testing



#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#define SPP_TAG "SPP_LED_CONTROL_V2"
#define DEVICE_NAME "ESP32-Fixed-Test"

#define LED_PIN GPIO_NUM_2

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            ESP_LOGI(SPP_TAG, "SPP_INIT_EVT");
            esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "SPP_SERVER");
            break;

        case ESP_SPP_SRV_OPEN_EVT: 
            ESP_LOGI(SPP_TAG, "Client Connected!");
            gpio_set_level(LED_PIN, 1); 
            break;

        case ESP_SPP_CLOSE_EVT: 
            ESP_LOGI(SPP_TAG, "Client Disconnected!");
            gpio_set_level(LED_PIN, 0); 
            break;
        
        default:
            break;
    }
}


void app_main(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE)); 

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Initialize controller failed: %s", esp_err_to_name(ret));
        return;
    }
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Enable controller failed: %s", esp_err_to_name(ret));
        return;
    }
    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Initialize bluedroid failed: %s", esp_err_to_name(ret));
        return;
    }
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Enable bluedroid failed: %s", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_dev_set_device_name(DEVICE_NAME)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Set device name failed: %s", esp_err_to_name(ret));
        return;
    }
    if ((ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Set scan mode failed: %s", esp_err_to_name(ret));
        return;
    }


    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "SPP register failed: %s", esp_err_to_name(ret));
        return;
    }
    
    esp_spp_cfg_t spp_cfg = {
        .mode = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = true,
        .tx_buffer_size = 0,
    };
    if ((ret = esp_spp_enhanced_init(&spp_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "SPP init failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(SPP_TAG, "Setup finished. Waiting for connection...");
}