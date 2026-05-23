#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Call once after wifi_manager_init(). Registers for WIFI_MGR_EVT_CONNECTED
// and syncs automatically each time WiFi connects.
esp_err_t ntp_sync_init(void);

// Trigger a sync immediately (must be connected).
esp_err_t ntp_sync_now(void);

// Get/set the NTP server hostname (persisted in settings).
const char *ntp_sync_get_server(void);
void        ntp_sync_set_server(const char *server);

#ifdef __cplusplus
}
#endif
