#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_DISPLAY_TIMEOUT_10S 10000
#define SETTINGS_DISPLAY_TIMEOUT_20S 20000
#define SETTINGS_DISPLAY_TIMEOUT_30S 30000
#define SETTINGS_DISPLAY_TIMEOUT_1MIN 60000

void settings_init(void);
void settings_set_brightness(uint8_t level);
uint8_t settings_get_brightness(void);
void settings_set_display_timeout(uint32_t timeout);
uint32_t settings_get_display_timeout(void);
void settings_set_sound(bool enabled);
bool settings_get_sound(void);
void settings_set_bluetooth_enabled(bool enabled);
bool settings_get_bluetooth_enabled(void);

// Notification volume (0-100)
void settings_set_notify_volume(uint8_t vol_percent);
uint8_t settings_get_notify_volume(void);

// Persist settings to SPIFFS JSON and load from it
bool settings_save(void);
bool settings_load(void);

// Step goal (daily steps target)
void settings_set_step_goal(uint32_t steps);
uint32_t settings_get_step_goal(void);

// NTP server hostname
void        settings_set_ntp_server(const char *server);
const char *settings_get_ntp_server(void);

// Time format: true = 24h, false = 12h
void settings_set_time_24h(bool enabled);
bool settings_get_time_24h(void);

// WiFi permission: true = user has allowed WiFi (NTP syncs daily, releases after)
void settings_set_wifi_enabled(bool enabled);
bool settings_get_wifi_enabled(void);

// Watchface style: 0=face1, 1=face2
void settings_set_watchface_style(int style);
int  settings_get_watchface_style(void);

// Watchface background: 0=background_wf_2, 1=background_wf
void settings_set_watchface_bg(int bg);
int  settings_get_watchface_bg(void);

// Restore factory defaults and persist
bool settings_reset_defaults(void);

// Maintenance: format SPIFFS storage partition
bool settings_format_spiffs(void);

#ifdef __cplusplus
}
#endif
