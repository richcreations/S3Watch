# S3Watch — Smartwatch Firmware for Waveshare ESP32-S3-Touch-AMOLED-2.06

Custom smartwatch firmware built on ESP-IDF 5.x and LVGL 9, targeting the [Waveshare ESP32-S3-Touch-AMOLED-2.06](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-2.06) development board.

<img src="ui_assets/s3watch_0.jpg" width="300"/> <img src="ui_assets/s3watch_1.jpg" width="300"/>

---

## Features

- **Two watchfaces** — selectable via settings; Face 1: large stacked HH/MM with coloured digits; Face 2: horizontal HH:MM with subscript AM/PM and SS, full date (mm/dd/yyyy), and full weekday name
- **Background selector** — three swappable background images, independently configurable per session
- **12/24-hour mode** — AM/PM label shown/hidden automatically; control screen clock respects the setting
- **Wi-Fi + NTP time sync** — connects on demand, syncs via NTP, then immediately releases the radio to save power; re-syncs daily at 3 AM if Wi-Fi permission is enabled
- **Wi-Fi permission toggle** — master switch in the control screen; when off, the radio never activates (NTP sync suppressed)
- **Wi-Fi settings** — scan for networks, connect with on-screen keyboard, configure NTP server
- **Notifications** — notification list and pop-up UI; push source requires BLE (currently disabled — see Known Limitations)
- **Step counter** — daily step count screen with configurable goal and progress ring; sensor task currently disabled (see Known Limitations)
- **Settings menu** — brightness, display timeout, sound on/off, notification volume, step goal, set time (roller pickers), Wi-Fi, NTP server, watchface style, background, storage browser, flashlight, restore defaults
- **Control screen** — quick-access panel with 8-button grid: Brightness, Silence, Flashlight, Battery, Wi-Fi toggle, Settings
- **Flashlight** — full-screen white display at 100% brightness; swipe right to exit and restore previous brightness
- **Audio alerts** — WAV/MP3 notification tones played via ES8311 codec; synthesized fallback tone if file not found
- **Display power management** — configurable auto-off timeout, wake on PMU power key, light-sleep compatible PM locks
- **Persistent settings** — stored as JSON on a dedicated SPIFFS partition; survives reboot
- **RTC persistence** — PCF85063A hardware RTC with NVS-backed time persistence in case of power loss

---

## Power Management

The firmware is designed to maximise time spent in light sleep when the display is off, and to let dynamic frequency scaling (DFS) drop the CPU to 40 MHz between events when the display is on.

| Source | Display ON | Display OFF |
|---|---|---|
| LVGL task (`taskLVGL`) | 33 ms refresh (30 fps) | **suspended** via `vTaskSuspend()` |
| Display manager task | 50 ms poll | 1 s poll (PMU wake-button check) |
| Back-button task | 20 ms poll | **blocked indefinitely** — unblocked by display-on callback |
| AXP2101 PMU monitor | 2000 ms | 2000 ms |
| Audio codec / I2S | open only during playback | closed |
| Wi-Fi radio | off unless NTP sync | off |

**Measured behaviour** (via `CONFIG_PM_PROFILING`):

| State | Time in light sleep | Time at 40 MHz | Time at 240 MHz |
|---|---|---|---|
| Display OFF | **~99%** | — | <1% |
| Display ON, idle | 0% (blocked by lock) | **~71%** | ~26% |

CPU configured for DFS between 40 MHz and 240 MHz with light sleep enabled (`esp_pm_configure`). CPU is powered down in light sleep (`CONFIG_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP`).

A single `ESP_PM_NO_LIGHT_SLEEP` lock — held only while the display is active — gates sleep. The audio codec is opened on each playback and closed immediately after, so its I2S `APB_FREQ_MAX` lock isn't held continuously. The BSP's I2S RX (microphone) channel is left initialised but not enabled, since the firmware never records — an enabled I2S channel holds an APB lock even when idle.

> **Note:** The AXP2101 INT pin (GPIO 35) is held LOW by the PMIC when USB power is connected. Level-triggered GPIO wakeup on this pin causes a sleep/wake spin loop and WDT reset. GPIO wakeup from the PMU is therefore not used; the display manager polls the PMU at 1 s intervals instead.

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
│    Steps   │◄──────────│   Controls   │
└────────────┘           └──────────────┘
```

- **Watchface** — main screen; swipe in any cardinal direction to reach the adjacent panel
- **Notifications** — swipe down; notification list with app icon, title, and message
- **Steps** — swipe right; daily step count vs. goal with progress ring
- **Controls** — swipe left; quick-access grid (brightness, silence, flashlight, battery, Wi-Fi, settings); each button opens a detail screen or sub-menu

---

## Project Structure

```
S3Watch/
├── main/                        # app_main, NVS init, boot sequence
├── components/
│   ├── gui/                     # All LVGL screens and UI logic
│   │   ├── src/
│   │   │   ├── ui.c                         # UI task, screen orchestration
│   │   │   ├── watchface.c                  # Clock screen (face 1 + face 2)
│   │   │   ├── notifications.c              # Notification list and pop-up
│   │   │   ├── steps_screen.c               # Step counter screen
│   │   │   ├── batt_screen.c                # Battery detail screen
│   │   │   ├── settings_screen.c            # Control screen (quick-access grid)
│   │   │   ├── settings_menu_screen.c       # Settings menu list
│   │   │   ├── setting_time_screen.c        # Roller-based time/date picker
│   │   │   ├── setting_timeout_screen.c
│   │   │   ├── brightness_screen.c
│   │   │   ├── setting_sound_screen.c
│   │   │   ├── setting_step_goal_screen.c
│   │   │   ├── setting_flashlight_screen.c  # Full-screen white flashlight
│   │   │   ├── setting_watchface_screen.c   # Watchface style + background picker
│   │   │   ├── setting_storage_screen.c     # Storage info + restore defaults
│   │   │   ├── wifi_screens.c               # Wi-Fi scan, connect, NTP config
│   │   │   ├── scroll_keyboard.c            # On-screen keyboard for text input
│   │   │   ├── storage_file_explorer.c      # SPIFFS file browser
│   │   │   └── lvgl_spiffs_fs.c             # LVGL FS driver for SPIFFS
│   │   ├── font/                # Pre-compiled LVGL fonts (26–160 px, including custom number fonts)
│   │   └── icons/               # Pre-compiled LVGL image assets (icons + backgrounds)
│   ├── bsp_extra/               # Board-level drivers not in upstream BSP
│   │   ├── src/bsp_board_extra.c  # I2C bus init, I2C bus recovery
│   │   ├── src/pcf85063a.c        # PCF85063A RTC driver (BCD encode/decode)
│   │   └── src/rtc_lib.c          # RTC abstraction, NVS persistence
│   ├── display_manager/         # Display on/off, timeout, wake-on-button, PM locks
│   ├── settings/                # Settings store (JSON on SPIFFS, debounced writes)
│   ├── wifi_manager/            # Wi-Fi init, scan, connect, auto-connect, radio release
│   ├── ntp_sync/                # SNTP time sync; fires on Wi-Fi connect, releases radio after; daily re-sync
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
| `CONFIG_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP` | `y` | Power down CPU core during light sleep |
| `CONFIG_LV_DEF_REFR_PERIOD` | `33` | ~30 fps LVGL refresh |
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
| `esp32_s3_touch_amoled_2_06` | [`github.com/richcreations/esp32_s3_touch_amoled_2_06`](https://github.com/richcreations/esp32_s3_touch_amoled_2_06) | Board BSP (display, touch, power) |
| `waveshare/qmi8658` | Component registry | IMU driver |
| `espressif/esp_codec_dev` | Component registry | Audio codec abstraction |
| `minimp3` | Vendored in `audio_alert/third_party/` | Lightweight MP3 decoder |

---

## Known Limitations

- **BLE disabled** — `CONFIG_BT_ENABLED=n`; the `ble_sync` and `nimble-nordic-uart` components exist as stubs for future ANCS/NUS support. The notifications UI is present but receives no data without a BLE source.
- **Step counter disabled** — the QMI8658 sensor task is commented out in `main.cpp`; the steps screen shows but does not update.
- **Wi-Fi radio released after NTP sync** — the radio is stopped once time is synced to save power. Re-opening Wi-Fi settings wakes the radio on demand.
- **No VBAT backup for RTC** — the PCF85063A loses power when the main battery is fully drained; time is recovered from NVS on next boot but may be slightly stale.
- **PMU GPIO wakeup not used** — the AXP2101 INT pin (GPIO 35) is held LOW when USB power is connected, making level-triggered wakeup unsafe (causes WDT reset). The display manager polls instead.
- **Watchface width variability** — `font_numbers_120` has non-monospaced digits ('4' is 85 px wide vs '1' at 63 px). For digit combinations heavy in '4's, the time row may slightly overflow the 410 px screen width.
