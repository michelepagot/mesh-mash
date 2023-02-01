#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"


/*******************************************************
 *                Variable Definitions
 *******************************************************/
static const char *MESH_TAG = "mesh_main";
static esp_netif_t *netif_sta = NULL;


void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));

}

void app_main(void)
{
  printf("Hello world!\n");

  /* Needed here otherwise an ESP_ERR_NVS_NOT_INITIALIZED occur
  * at Wifi init
  */
  ESP_ERROR_CHECK(nvs_flash_init());
  /* From https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-reference/network/esp-wifi-mesh.html?highlight=esp_event_loop_create_default#writing-an-esp-wifi-mesh-application
  *
  *  The prerequisites for starting ESP-WIFI-MESH is to initialize LwIP and Wi-Fi.
  * The necessary prerequisite steps before ESP-WIFI-MESH itself
  * can be initialized:
  */

  /* 1. Initialize the underlying TCP/IP stack.  */
  ESP_ERROR_CHECK(esp_netif_init());
  /* 2. event initialization. Default event loops
  * are explained in
  * https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-reference/system/esp_event.html?highlight=esp_event_loop_create_default#default-event-loop
  */
  ESP_ERROR_CHECK(esp_event_loop_create_default());


  /* Creates default STA and AP network interfaces for esp-mesh.
  * The two argument as to be intended as output.
  * Both netifs are almost identical to the default station and softAP,
  * but with DHCP client and server disabled.
  * Please note that the DHCP client is typically enabled only
  * if the device is promoted to a root node.
  *
  * Only station instance saved for further manipulation,
  * soft AP instance ignored.
  */
  ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL));

  /*  wifi initialization */
  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&config));

  /* IP_EVENT_STA_GOT_IP : https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-guides/wifi.html?highlight=ip_event_sta_got_ip#ip-event-sta-got-ip
  * arise when the DHCP client successfully gets the IPV4 address from the DHCP server,
  * or when the IPV4 address is changed.
  * The event means that everything is ready and the application can begin its tasks
  * (e.g., creating sockets).
  */
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

  /* Set the WiFi API configuration storage type. */
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
  ESP_ERROR_CHECK(esp_wifi_start());
}
