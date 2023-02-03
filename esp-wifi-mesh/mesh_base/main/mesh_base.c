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
#define MESH_ID_SIZE 6
static const uint8_t MESH_ID[MESH_ID_SIZE] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00};
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;
static esp_netif_t *netif_sta = NULL;


void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
  mesh_addr_t id = {0,};
  static uint16_t last_layer = 0;

  switch (event_id) {
    case MESH_EVENT_STARTED: {
      esp_mesh_get_id(&id);
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
      is_mesh_connected = false;
      mesh_layer = esp_mesh_get_layer();
    }
    break;

    case MESH_EVENT_STOPPED: {
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
      is_mesh_connected = false;
      mesh_layer = esp_mesh_get_layer();
    }
    break;

    case MESH_EVENT_CHILD_CONNECTED: {
      mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
               child_connected->aid,
               MAC2STR(child_connected->mac));
    }
    break;

    case MESH_EVENT_CHILD_DISCONNECTED: {
      mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
               child_disconnected->aid,
               MAC2STR(child_disconnected->mac));
    }
    break;

    case MESH_EVENT_ROUTING_TABLE_ADD: {
      mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
      ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
               routing_table->rt_size_change,
               routing_table->rt_size_new,
               mesh_layer);
    }
    break;

    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
      mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
      ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
               routing_table->rt_size_change,
               routing_table->rt_size_new,
               mesh_layer);
    }
    break;

    case MESH_EVENT_NO_PARENT_FOUND: {
      mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
               no_parent->scan_times);
    }
    break;

    case MESH_EVENT_PARENT_CONNECTED: {
      bool is_root;
      mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
      esp_mesh_get_id(&id);
      mesh_layer = connected->self_layer;
      memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
      is_root = esp_mesh_is_root();
      ESP_LOGI(MESH_TAG,
               "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR", duty:%d",
               last_layer,
               mesh_layer,
               MAC2STR(mesh_parent_addr.addr),
               is_root ? "<ROOT>":"",
               MAC2STR(id.addr),
               connected->duty);
      last_layer = mesh_layer;

      is_mesh_connected = true;
      if (is_root) {
        /* Stop DHCP client (only if enabled in interface object) */
        esp_netif_dhcpc_stop(netif_sta);
        /* Start DHCP client (only if enabled in interface object) */
        esp_netif_dhcpc_start(netif_sta);
      }
    }
    break;

    case MESH_EVENT_PARENT_DISCONNECTED: {
      /* At the moment mesh_event_disconnect_t is an alias for
      * wifi_event_sta_disconnected_t
      */
      mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
      ESP_LOGI(MESH_TAG,
               "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
               disconnected->reason); // wifi_err_reason_t in esp_wifi_types.h
      is_mesh_connected = false;
      mesh_layer = esp_mesh_get_layer();
    }
    break;

    case MESH_EVENT_LAYER_CHANGE: {
      mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
      mesh_layer = layer_change->new_layer;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer,
                 mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>":"");
      last_layer = mesh_layer;
    }
    break;

    case MESH_EVENT_ROOT_ADDRESS: {
      mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
               MAC2STR(root_addr->addr));
    }
    break;

    case MESH_EVENT_VOTE_STARTED: {
      mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
               vote_started->attempts,
               vote_started->reason,
               MAC2STR(vote_started->rc_addr.addr));
    }
    break;

    case MESH_EVENT_VOTE_STOPPED: {
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
    }
    break;

    case MESH_EVENT_ROOT_SWITCH_REQ: {
      mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
               switch_req->reason,
               MAC2STR( switch_req->rc_addr.addr));
    }
    break;

    case MESH_EVENT_ROOT_SWITCH_ACK: {
      /* new root */
      mesh_layer = esp_mesh_get_layer();
      esp_mesh_get_parent_bssid(&mesh_parent_addr);
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"",
              mesh_layer,
              MAC2STR(mesh_parent_addr.addr));
    }
    break;

    case MESH_EVENT_TODS_STATE: {
      mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d",
              *toDs_state);
    }
    break;

    case MESH_EVENT_ROOT_FIXED: {
      mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
               root_fixed->is_fixed ? "fixed" : "not fixed");
    }
    break;

    case MESH_EVENT_ROOT_ASKED_YIELD: {
      mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
      ESP_LOGI(MESH_TAG,
               "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
               MAC2STR(root_conflict->addr),
               root_conflict->rssi,
               root_conflict->capacity);
    }
    break;

    case MESH_EVENT_CHANNEL_SWITCH: {
      mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d",
               channel_switch->channel);
    }
    break;

    case MESH_EVENT_SCAN_DONE: {
      mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
               scan_done->number);
    }
    break;

    case MESH_EVENT_NETWORK_STATE: {
      mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
               network_state->is_rootless);
    }
    break;

    case MESH_EVENT_STOP_RECONNECTION: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    }
    break;

    case MESH_EVENT_FIND_NETWORK: {
      mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
               find_network->channel,
               MAC2STR(find_network->router_bssid));
    }
    break;

    case MESH_EVENT_ROUTER_SWITCH: {
      mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
               router_switch->ssid,
               router_switch->channel,
               MAC2STR(router_switch->bssid));
    }
    break;

    case MESH_EVENT_PS_PARENT_DUTY: {
      mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d",
               ps_duty->duty);
    }
    break;

    case MESH_EVENT_PS_CHILD_DUTY: {
      mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
      ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, "MACSTR", duty:%d",
               ps_duty->child_connected.aid-1,
               MAC2STR(ps_duty->child_connected.mac),
               ps_duty->duty);
    }
    break;

    default:
      ESP_LOGE(MESH_TAG, ">>>>>  unknown id:%ld", event_id);
      break;
    }
}

void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));

}

void app_main(void)
{
  /* Needed here otherwise an ESP_ERR_NVS_NOT_INITIALIZED occur
  * at Wifi init
  */
  ESP_ERROR_CHECK(nvs_flash_init());

  /* From https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-reference/network/esp-wifi-mesh.html?highlight=esp_event_loop_create_default#writing-an-esp-wifi-mesh-application
  *
  * The prerequisites for starting ESP-WIFI-MESH is to initialize LwIP and Wi-Fi.
  * The necessary prerequisite steps before ESP-WIFI-MESH itself
  * can be initialized:
  */

  /* 1. Initialize the underlying TCP/IP stack.  */
  ESP_ERROR_CHECK(esp_netif_init());

  /* 2. event initialization.
  * Default event loops are explained in
  * https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-reference/system/esp_event.html?highlight=esp_event_loop_create_default#default-event-loop
  */
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  /* Creates default STA and AP network interfaces for esp-mesh.
  * The two argument has to be intended as output.
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

  /* Mesh initialization: 
  *    - Check whether Wi-Fi is started.
  *    - Initialize mesh global variables with default values.
  *  This API shall be called after Wi-Fi is started.
  */
  ESP_ERROR_CHECK(esp_mesh_init());

  ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));

  /*  set mesh topology selected by the user
  * during the project menuconfig execution
  * Available topology are:
  *   - MESH_TOPO_TREE: default
  *   - MESH_TOPO_CHAIN supports up to 1000 layers
  */
  ESP_ERROR_CHECK(esp_mesh_set_topology(CONFIG_MESH_TOPOLOGY));

  /*  set mesh max layer according to the topology */
  ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));

  /* Set vote percentage threshold for approval of being a root (default:0.9)
  *  During the networking, only obtaining vote percentage
  *  reaches this threshold, the device could be a root.
  */
  ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));

  /* Set the number of queue, configure the RX queue size.
  * default:32.
  * If this size is too large, and esp_mesh_recv_toDS() isn’t called in time,
  * there is a risk that a great deal of memory is occupied by the pending packets.
  * If this size is too small, it will impact the efficiency on upstream.
  * How to decide this value depends on the specific application scenarios.*/
  ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));

  /* Disable mesh PowerSaving function */
  ESP_ERROR_CHECK(esp_mesh_disable_ps());
  ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));

  /****************************/
  /*    MESH CONFIGURATION    */
  mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
  /* mesh ID */
  memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, MESH_ID_SIZE);
  /* router */
  cfg.channel = CONFIG_MESH_CHANNEL;
  cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
  memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
  memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,
           strlen(CONFIG_MESH_ROUTER_PASSWD));
  /* mesh softAP */
  ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
  cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
  cfg.mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
  memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
           strlen(CONFIG_MESH_AP_PASSWD));
  ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
  /*****************************/

  /* mesh start */
  ESP_ERROR_CHECK(esp_mesh_start());

  ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%ld, %s<%d>%s, ps:%d\n",
      esp_get_minimum_free_heap_size(),
      esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed",
      esp_mesh_get_topology(),
      esp_mesh_get_topology() ? "(chain)":"(tree)",
      esp_mesh_is_ps_enabled());
}
