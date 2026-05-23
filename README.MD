# S3Watch — Smartwatch Firmware for Waveshare ESP32-S3-Touch-AMOLED-2.06

Custom smartwatch firmware built on ESP-IDF 5.x and LVGL 9, targeting the [Waveshare ESP32-S3-Touch-AMOLED-2.06](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-2.06) development board.

<img src="ui_assets/s3watch_0.jpg" width="300"/> <img src="ui_assets/s3watch_1.jpg" width="300"/>

---

## Features

- **Watchface** — large digit clock (HH / MM / SS) with date, day-of-week, and battery icon with charge state
- **Wi-Fi + NTP time sync** — connects to saved networks on boot, syncs time via NTP, then releases the radio to save power
- **Wi-Fi settings** — scan for networks, connect with on-screen keyboard, configure NTP server
- **Notifications** — notification list and pop-up UI; push source requires BLE (currently disabled — see Known Limitations)
- **Step counter** — daily step count screen with configurable goal and progress ring; sensor task currently disabled (see Known Limitations)
- **Settings menu** — brightness, display timeout, sound on/off, notification volume, step goal, set time (roller pickers), Wi-Fi, NTP server, storage browser, flashlight, restore defaults
- **Audio alerts** — WAV/MP3 notification tones played via ES8311 codec; synthesized fallback tone if file not found
- **Display power management** — configurable auto-off timeout, wake on PMU power key, light-sleep compatible PM locks
- **Persistent settings** — stored as JSON on a dedicated SPIFFS partition; survives reboot
- **RTC persistence** — PCF85063A hardware RTC with NVS-backed time persistence in case of power loss

---

## Hardware

| Component | Part | Interface |
|---|---|---|
| MCU | ESP32-S3R8 (dual-core LX7 @ 240 MHz, 8 MB PSRAM) | — |
| Display | 2.06" AMOLED 410×502, SH8601 driver | QSPI |
| Touch | FT3168 capacitive controller (FT5x06-compatible) | I2C |
| IMU | QMI8658 6-axis (accel + gyro) | I2C |
| RTC | PCF85063A | I2C |
| PMU | AXP2101 (ALDO rails, charging, power key) | I2C |
| Audio | ES8311 codec + speaker | I2S |
| Flash | 32 MB external QSPI | — |
| Battery | 3.7 V Li-ion via MX1.25 header | — |

> **Note on ALDO rails:** The AXP2101 ALDO1–4 rails are disabled by default on cold boot. The firmware explicitly enables them before `bsp_display_start()` to ensure the FT3168 touch controller has power. The BSP cuts the rails when the display sleeps and re-enables them on wake.

---

## UI Navigation

The UI is gesture-driven from the main watchface:

```
         ┌─────────────────┐
         │   Notifications  │  ← swipe down from watchface
         └─────────────────┘
                  ▲
  swipe right ◄──┼──► swipe left
                  ▼
┌────────────┐ watchface ┌──────────────┐
│    Steps   │◄──────────│   Settings   │
└────────────┘           └──────────────┘
```

- **Watchface** — main screen; swipe in any cardinal direction to reach the adjacent panel
- **Notifications** — swipe down; shows notification list with app icon, title, and message
- **Steps** — swipe right; daily step count vs. goal with progress ring
- **Settings** — swipe left; leads to a menu with sub-screens for each setting category

---

## Project Structure

```
S3Watch/
├── main/                        # app_main, NVS init, boot sequence
├── components/
│   ├── gui/                     # All LVGL screens and UI logic
│   │   ├── src/
│   │   │   ├── ui.c                         # UI task, screen orchestration
│   │   │   ├── watchface.c                  # Main clock screen
│   │   │   ├── notifications.c              # Notification list and pop-up
│   │   │   ├── steps_screen.c               # Step counter screen
│   │   │   ├── batt_screen.c                # Battery detail screen
│   │   │   ├── settings_screen.c / settings_menu_screen.c
│   │   │   ├── setting_time_screen.c        # Roller-based time/date picker
│   │   │   ├── setting_timeout_screen.c
│   │   │   ├── brightness_screen.c
│   │   │   ├── setting_sound_screen.c
│   │   │   ├── setting_step_goal_screen.c
│   │   │   ├── setting_flashlight_screen.c
│   │   │   ├── setting_storage_screen.c     # Storage info + restore defaults
│   │   │   ├── wifi_screens.c               # Wi-Fi scan, connect, NTP config
│   │   │   ├── scroll_keyboard.c            # On-screen keyboard for text input
│   │   │   ├── storage_file_explorer.c      # SPIFFS file browser
│   │   │   └── lvgl_spiffs_fs.c             # LVGL FS driver for SPIFFS
│   │   ├── font/                # Pre-compiled LVGL fonts (26–160 px)
│   │   └── icons/               # Pre-compiled LVGL image assets
│   ├── bsp_extra/               # Board-level drivers not in upstream BSP
│   │   ├── src/bsp_board_extra.c  # I2C bus init, I2C bus recovery
│   │   ├── src/pcf85063a.c        # PCF85063A RTC driver (BCD encode/decode)
│   │   └── src/rtc_lib.c          # RTC abstraction, NVS persistence
│   ├── display_manager/         # Display on/off, timeout, wake-on-button, PM locks
│   ├── settings/                # Settings store (JSON on SPIFFS, debounced writes)
│   ├── wifi_manager/            # Wi-Fi init, scan, connect, auto-connect, radio release
│   ├── ntp_sync/                # SNTP time sync; fires on Wi-Fi connect, releases radio after sync
│   ├── audio_alert/             # ES8311 codec init, WAV/MP3 decode, synthesized fallback
│   ├── sensors/                 # QMI8658 step count wrapper (task currently disabled)
│   ├── ble_sync/                # BLE stub — NUS implementation stubbed out; BLE disabled
│   └── nimble-nordic-uart/      # Nordic UART Service GATT implementation (unused; BLE disabled)
├── ui_assets/                   # Source images and fonts (pre-conversion)
├── partitions.csv               # Custom partition table (app + storage SPIFFS)
├── sdkconfig.defaults           # Project Kconfig defaults
└── dependencies.lock            # Component manager lock file
```

---

## Building and Flashing

### Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/) with the [ESP-IDF Extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)
- ESP-IDF **v5.5.4** (install via the extension wizard or manually)
- USB-C cable connected to the board's USB port

### Steps

1. **Install the ESP-IDF extension** — open VS Code, go to Extensions, search for `ESP-IDF`, and install it. Run `ESP-IDF: Configure ESP-IDF extension` from the command palette and let it download ESP-IDF 5.5.4 and the toolchain.

2. **Open the project** — `File → Open Folder...` → select the `S3Watch` directory. The ESP-IDF status bar appears at the bottom of the window.

3. **Set the target** — click `Set Target` in the status bar (or `ESP-IDF: Set Target`) and choose `esp32s3`.

4. **Build** — click `Build Project` in the status bar or run `ESP-IDF: Build Project`. First build takes several minutes as managed components are fetched and compiled.

5. **Select the port** — connect the board via USB-C, then run `ESP-IDF: Select Serial Port` and pick the device (e.g., `/dev/cu.usbmodem*` on macOS, `COMx` on Windows).

6. **Flash** — run `ESP-IDF: Flash (UART)` to program the board, or `ESP-IDF: Flash and Monitor` to flash and immediately open the serial monitor.

### Command-line build (alternative)

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodem* flash monitor
```

---

## Configuration

Key options in [`sdkconfig.defaults`](sdkconfig.defaults):

| Option | Value | Purpose |
|---|---|---|
| `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240` | `y` | Run CPU at 240 MHz |
| `CONFIG_SPIRAM_MODE_OCT` | `y` | Octal PSRAM mode (8 MB) |
| `CONFIG_FREERTOS_HZ` | `200` | Lower tick rate to reduce idle overhead |
| `CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH` | `6144` | Extra stack for SPIFFS + cJSON in timer callbacks |
| `CONFIG_PM_ENABLE` | `y` | Dynamic frequency scaling + light sleep |
| `CONFIG_LV_DEF_REFR_PERIOD` | `15` | ~67 fps LVGL refresh |
| `CONFIG_ESP_WIFI_ENABLED` | `y` | Wi-Fi stack for NTP sync and settings |
| `CONFIG_BT_ENABLED` | `n` | BLE stack disabled |

---

## Partition Table

Defined in [`partitions.csv`](partitions.csv). Includes a dedicated `storage` SPIFFS partition used for settings JSON and audio assets.

---

## Dependencies

Managed via the ESP-IDF Component Manager:

| Component | Source | Purpose |
|---|---|---|
| `lvgl/lvgl` | Component registry | UI graphics library (v9.3.0) |
| `espressif/esp_lvgl_port` | Component registry | LVGL/ESP-IDF integration layer |
| `waveshare/esp32_s3_touch_amoled_2_06` | Component registry | Board BSP (display, touch, power) |
| `waveshare/qmi8658` | Component registry | IMU driver |
| `espressif/esp_codec_dev` | Component registry | Audio codec abstraction |
| `minimp3` | Vendored in `audio_alert/third_party/` | Lightweight MP3 decoder |

---

## Known Limitations

- **BLE disabled** — `CONFIG_BT_ENABLED=n`; the `ble_sync` and `nimble-nordic-uart` components exist as stubs for future ANCS/NUS support. The notifications UI is present but receives no data without a BLE source.
- **Step counter disabled** — the QMI8658 sensor task is commented out in `main.cpp`; the steps screen shows but does not update.
- **Wi-Fi radio released after NTP sync** — the radio is stopped once time is synced to save power. Re-opening Wi-Fi settings wakes the radio on demand.
- **No VBAT backup for RTC** — the PCF85063A loses power when the main battery is fully drained; time is recovered from NVS on next boot but may be slightly stale.
