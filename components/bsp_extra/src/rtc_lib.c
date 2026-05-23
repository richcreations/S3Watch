#include "rtc_lib.h"
#include "pcf85063a.h"
#include <time.h>
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static struct tm current_time;
static esp_timer_handle_t rtc_timer;

static const char *weekdays[]      = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
static const char *weekdaysshort[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
static const char *months[]        = {"January","February","March","April","May","June","July","August","September","October","November","December"};

#define NVS_NS  "rtc"
#define NVS_KEY "timestamp"
// Minimum plausible timestamp: 2025-01-01 00:00:00 UTC
#define MIN_VALID_TS ((int64_t)1735689600)

static void nvs_save_time(const struct tm *t)
{
    struct tm tmp = *t;
    time_t ts = mktime(&tmp);
    if (ts < (time_t)MIN_VALID_TS) return;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_i64(h, NVS_KEY, (int64_t)ts);
    if (nvs_commit(h) != ESP_OK) {
        ESP_LOGW("rtc_lib", "NVS commit failed");
    }
    nvs_close(h);
}

static bool nvs_load_time(struct tm *out)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    int64_t ts = 0;
    esp_err_t err = nvs_get_i64(h, NVS_KEY, &ts);
    nvs_close(h);
    if (err != ESP_OK || ts < MIN_VALID_TS) return false;
    time_t t_val = (time_t)ts;
    struct tm *lt = gmtime(&t_val);
    if (!lt) return false;
    *out = *lt;
    return true;
}

static bool time_is_valid(const struct tm *t)
{
    // tm_year is years since 1900; accept 2025 (125) onward
    return t->tm_year >= 125;
}

static uint32_t s_nvs_tick = 0;

static void rtc_update_task(void *arg)
{
    pcf85063a_get_time(&current_time);
    // Save to NVS once per minute so flash wear is minimal
    if (++s_nvs_tick >= 60) {
        s_nvs_tick = 0;
        if (time_is_valid(&current_time)) {
            nvs_save_time(&current_time);
        }
    }
}

esp_err_t rtc_start(void)
{
    esp_err_t ret = pcf85063a_init();
    if (ret != ESP_OK) return ret;

    // Populate current_time immediately so callers don't see midnight
    pcf85063a_get_time(&current_time);

    // If the RTC lost power (all zeros / pre-2025), restore from NVS
    if (!time_is_valid(&current_time)) {
        struct tm saved;
        if (nvs_load_time(&saved)) {
            pcf85063a_set_time(&saved);
            current_time = saved;
        }
    }

    const esp_timer_create_args_t args = {
        .callback = rtc_update_task,
        .name = "rtc_timer",
    };
    ret = esp_timer_create(&args, &rtc_timer);
    if (ret != ESP_OK) return ret;

    return esp_timer_start_periodic(rtc_timer, 1000000);
}

esp_err_t rtc_get_time(struct tm *time)
{
    *time = current_time;
    return ESP_OK;
}

esp_err_t rtc_set_time(const struct tm *time)
{
    esp_err_t ret = pcf85063a_set_time(time);
    if (ret == ESP_OK) {
        current_time = *time;
        nvs_save_time(time);
    }
    return ret;
}

void rtc_suspend(void)
{
    if (rtc_timer) {
        esp_timer_stop(rtc_timer);
    }
    // Flush current time to NVS before the I2C bus loses power
    if (time_is_valid(&current_time)) {
        nvs_save_time(&current_time);
    }
}

void rtc_resume(void)
{
    // ALDOs are back up by now, but give the PCF85063A a moment to stabilise
    vTaskDelay(pdMS_TO_TICKS(30));
    pcf85063a_get_time(&current_time);
    if (!time_is_valid(&current_time)) {
        struct tm saved;
        if (nvs_load_time(&saved)) {
            pcf85063a_set_time(&saved);
            current_time = saved;
        }
    }
    if (rtc_timer) {
        esp_timer_start_periodic(rtc_timer, 1000000);
    }
}

int rtc_get_hour(void)   { return current_time.tm_hour; }
int rtc_get_minute(void) { return current_time.tm_min; }
int rtc_get_second(void) { return current_time.tm_sec; }
int rtc_get_day(void)    { return current_time.tm_mday; }
int rtc_get_month(void)  { return current_time.tm_mon + 1; }
int rtc_get_year(void)   { return current_time.tm_year + 1900; }

const char *rtc_get_weekday_string(void)       { return weekdays[current_time.tm_wday]; }
const char *rtc_get_weekday_short_string(void) { return weekdaysshort[current_time.tm_wday]; }
const char *rtc_get_month_string(void)         { return months[current_time.tm_mon]; }
