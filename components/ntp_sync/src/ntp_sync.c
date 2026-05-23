#include "ntp_sync.h"
#include "wifi_manager.h"
#include "rtc_lib.h"
#include "settings.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <string.h>

static void wifi_release_task(void *arg)
{
    (void)arg;
    wifi_manager_release();
    vTaskDelete(NULL);
}

static const char *TAG = "NTP_SYNC";

static void on_sntp_sync(struct timeval *tv)
{
    (void)tv;
    // SNTP sets the system time; now mirror it to the hardware RTC.
    time_t now = time(NULL);
    struct tm t;
    gmtime_r(&now, &t);
    // Sanity check — ignore if year is clearly wrong
    if (t.tm_year < 125 || t.tm_year > 199) {
        ESP_LOGW(TAG, "SNTP gave suspicious year %d, ignoring", t.tm_year + 1900);
        return;
    }
    rtc_set_time(&t);
    ESP_LOGI(TAG, "RTC synced from NTP: %04d-%02d-%02d %02d:%02d:%02d UTC",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec);

    // Sync done — shut WiFi radio off to save power.
    // esp_wifi_stop() must not be called from the SNTP/lwIP callback context
    // (it tears down lwIP from within lwIP). Use a one-shot task instead.
    xTaskCreate(wifi_release_task, "wifi_rel", 2048, NULL, 5, NULL);
}

static void wifi_connected_cb(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    (void)arg; (void)base; (void)id; (void)data;
    ESP_LOGI(TAG, "WiFi connected — triggering NTP sync");
    ntp_sync_now();
}

esp_err_t ntp_sync_init(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, settings_get_ntp_server());
    sntp_set_time_sync_notification_cb(on_sntp_sync);
    // Don't call esp_sntp_init() yet — wait until WiFi is up.
    esp_event_handler_register(WIFI_MANAGER_EVENT_BASE,
                               WIFI_MGR_EVT_CONNECTED,
                               wifi_connected_cb, NULL);
    ESP_LOGI(TAG, "Initialized (server: %s)", settings_get_ntp_server());
    return ESP_OK;
}

esp_err_t ntp_sync_now(void)
{
    if (!wifi_manager_is_connected()) return ESP_ERR_INVALID_STATE;
    // Update server name in case it changed since init
    esp_sntp_setservername(0, settings_get_ntp_server());
    if (!esp_sntp_enabled()) {
        esp_sntp_init();
    } else {
        sntp_restart();
    }
    return ESP_OK;
}

const char *ntp_sync_get_server(void) { return settings_get_ntp_server(); }

void ntp_sync_set_server(const char *server)
{
    settings_set_ntp_server(server);
    esp_sntp_setservername(0, server);
}
