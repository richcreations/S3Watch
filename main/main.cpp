#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#include "display_manager.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lvgl.h"
#include "sensors.h"
#include "settings.h"
#include "ui.h"
#include "esp_sleep.h"
#include "esp_pm.h"
#include "audio_alert.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "ntp_sync.h"

static const char *TAG = "MAIN";

/*
#define CONFIG_LV_USE_LOG 1
#define CONFIG_LV_LOG_LEVEL LV_LOG_LEVEL_INFO  // or DEBUG/TRACE

static void lvgl_log_cb(lv_log_level_t level, const char *buf)
{
    // buf usually already ends with '\n'; don't add another.
    switch (level) {
    case LV_LOG_LEVEL_ERROR: ESP_LOGE("LVGL", "%s", buf); break;
    case LV_LOG_LEVEL_WARN:  ESP_LOGW("LVGL", "%s", buf); break;
    case LV_LOG_LEVEL_USER:  // falls through to INFO
    case LV_LOG_LEVEL_INFO:  ESP_LOGI("LVGL", "%s", buf); break;
    //case LV_LOG_LEVEL_DEBUG: ESP_LOGD("LVGL", "%s", buf); break;
    case LV_LOG_LEVEL_TRACE: ESP_LOGV("LVGL", "%s", buf); break;
    default:                 ESP_LOGI("LVGL", "%s", buf); break;
    }
}*/

extern "C" void app_main(void) {

  // esp_log_level_set("lcd_panel.io.spi", ESP_LOG_DEBUG);

  // NVS must be initialised before any component (RTC, BLE) uses it
  esp_err_t nvs_err = nvs_flash_init();
  if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "NVS partition damaged, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvs_err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvs_err);

  // Create default event loop for component event handlers
  esp_event_loop_create_default();

  // Block light-sleep during boot and UI/display bring-up
  display_manager_pm_early_init();

  // Enable Dynamic Frequency Scaling + automatic light sleep so CPU idles low
  // BLE remains active; display_manager controls light-sleep via a PM lock
  // Defer PM config until after BSP and BLE init

  // Initialize I2C bus first so AXP2101 can be configured before the BSP
  // tries to talk to the FT5x06 touch controller on the same bus.
  // bsp_i2c_init() is idempotent — subsequent calls from bsp_display_start()
  // will no-op via its i2c_initialized guard.
  bsp_i2c_init();
  bsp_extra_init();   // initializes AXP2101, sets bsp_power s_ready = true

  // On cold boot the AXP2101 ALDO LDO outputs start disabled. The BSP only
  // enables them in bsp_display_wake(), not during the initial bsp_display_start().
  // Explicitly enable ALDO1-4 here so the FT5x06 touch controller has power before
  // bsp_display_start() tries to initialize it over I2C.
  bsp_power_enable_aldo1(true);
  bsp_power_enable_aldo2(true);
  bsp_power_enable_aldo3(true);
  bsp_power_enable_aldo4(true);
  vTaskDelay(pdMS_TO_TICKS(50)); // let rails stabilize

  bsp_display_start();

  settings_init();

  wifi_manager_init();
  ntp_sync_init();
  // Try saved networks — if one connects, ntp_sync fires automatically,
  // syncs the RTC, then calls wifi_manager_release() to stop the radio.
  wifi_manager_auto_connect();

  // UI task chama display_manager_init() após criar o ecrã

  // UI e BLE subscrevem eventos diretamente; sem acoplamento no main

  //sensors_init();

  // Run the UI at a slightly higher priority so LVGL remains responsive
  xTaskCreate(ui_task, "ui", 8000, NULL, 4, NULL);
  
  // Sensor sampling can run at a lower priority without affecting UX
  //xTaskCreate(sensors_task, "sensors", 4096, NULL, 3, NULL);

  // Play a subtle startup tone once the system is up
  audio_alert_play_startup();

  // Enable PM — light sleep allowed; display_manager blocks it while screen is on
  esp_pm_config_t pm_cfg = {
      .max_freq_mhz = 240,
      .min_freq_mhz = 80,
      .light_sleep_enable = true,
  };
  ESP_ERROR_CHECK(esp_pm_configure(&pm_cfg));
}
