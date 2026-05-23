// BLE sync stub — NUS implementation removed.
// Replace this file with an ANCS implementation when iOS notification support is added.
#include "ble_sync.h"

ESP_EVENT_DEFINE_BASE(BLE_SYNC_EVENT_BASE);

esp_err_t ble_sync_init(void)      { return ESP_OK; }
esp_err_t ble_sync_send_status(int battery_percent, bool charging)
{
    (void)battery_percent; (void)charging; return ESP_OK;
}
esp_err_t ble_sync_set_enabled(bool enabled) { (void)enabled; return ESP_OK; }
bool      ble_sync_is_enabled(void)          { return false; }
