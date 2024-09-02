#ifndef __SOFTAP_STA_H__
#define __SOFTAP_STA_H__

#include <string>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "lwip/err.h"
#include "lwip/sys.h"

class SoftApSta {
public:
    SoftApSta();
    ~SoftApSta();

    void Init();

    void SetUpSta(std::string ssid, std::string passwd);

    void SetUpAp(std::string ssid, std::string passwd);

private:
    /* FreeRTOS event group to signal when we are connected/disconnected */
    EventGroupHandle_t s_wifi_event_group;
    int s_retry_num;
    void softap_set_dns_addr(esp_netif_t *esp_netif_ap,esp_netif_t *esp_netif_sta);
    esp_netif_t *wifi_init_sta(void);
    esp_netif_t *wifi_init_softap(void);
    
    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
};

#endif
