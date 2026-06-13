# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project scope

- Primary firmware project: `arduino_code/ESP32-A2DP-SPEAKER`
- Build system is PlatformIO from the repository root via `platformio.ini`
- `platformio.ini` sets `src_dir = ./arduino_code/ESP32-A2DP-SPEAKER` and `lib_dir = ./arduino_code/libraries2`
- Two PlatformIO environments exist, but the A2DP speaker target is `env:esp32dev`
- `env:esp32s3` exists in `platformio.ini`, but the A2DP speaker code and README state the actual application is for classic ESP32 only, not ESP32-S3

## Common commands

Run from repository root unless noted.

### Build

```bash
pio run -e esp32dev
```

### Clean build

```bash
pio run -e esp32dev -t clean
pio run -e esp32dev
```

### Upload firmware

```bash
pio run -e esp32dev -t upload
```

### Serial monitor

```bash
pio device monitor -b 115200
```

### Upload and immediately monitor

```bash
pio run -e esp32dev -t upload && pio device monitor -b 115200
```

### Build the unused S3 environment only when working on that config

```bash
pio run -e esp32s3
```

## Tests and linting

- No project test suite is configured in this repo.
- No lint/format task is configured in `platformio.ini`.
- If you need validation, use `pio run -e esp32dev` as the baseline compile check.
- There is no single-test command because no unit/integration test harness is present.

## High-level architecture

### Runtime structure

The firmware is a single Arduino app centered on `main.ino`, which wires together several stateful modules.

- `main.ino` owns boot order, creates the TFT/UI objects, registers Bluetooth callbacks, and runs the polling loop.
- `userconfig.h` is the hardware contract: board pins, timing constants, LED behavior, I2C addresses, screen timeout, shake threshold, and Bluetooth device name.
- Most modules expose a simple init/update API and keep state in file-level statics.

### Audio path

Bluetooth audio enters through `BluetoothA2DPSink` in `src/bluetooth_manager.cpp`.

- `initBluetooth()` configures I2S pin mapping and starts the sink.
- `main.ino` attaches `read_data_stream` with `getA2DPSink()->set_stream_reader(read_data_stream, false)`.
- `src/audio_i2s.cpp` is the PCM output stage that receives decoded stream data and writes it to I2S for the PCM5102 DAC.
- Volume control exposed to the UI/buttons uses AVRCP volume (`0-127`) in `bluetooth_manager.cpp`, not a separate software mixer module.

### Control and UI flow

The UI is event-driven from callbacks plus lightweight polling.

- `main.ino` creates `EEUI eeui` and updates the screen through callback functions such as metadata, track-change, and volume-change handlers.
- Bluetooth metadata from AVRCP is parsed in `avrc_metadata_callback()` and forwarded through `setMetadataCallback()`.
- Track navigation in `nextTrack()` / `previousTrack()` also triggers the album-cover callback path so the screen art changes with transport actions.
- Connection/playback state is polled in `loop()` and only re-rendered when state changes.

### Input stack

There are two independent input paths.

1. `src/button_handler.cpp`
   - Handles the on-board BOOT button with OneButton.
   - Only responsibility is factory reset by multi-click.

2. `src/pca9554_handler.cpp`
   - Handles external keys through a PCA9554 I/O expander over I2C.
   - IO1: previous track on short press, volume down on long press/repeat.
   - IO2: play/pause.
   - IO3: next track on short press, volume up on long press/repeat.
   - `updatePCA9554()` returns whether user activity happened; `main.ino` uses that to reset the backlight timeout.

### Sensor and album art interaction

The motion feature is split across two modules.

- `src/album_cover_manager.cpp` owns the built-in cover table and current index.
- `src/qmi8658_handler.cpp` reads the QMI8658 accelerometer and, on shake, advances to the next cover and redraws it through the `EEUI` instance.
- Cover assets live in `arduino_code/ESP32-A2DP-SPEAKER/emos/` as generated LVGL image headers.

### Persistence

- `src/config_manager.cpp` uses `Preferences` with namespace `bluetooth`.
- Persistence is minimal: it stores only a `paired` boolean and clears that namespace during factory reset.
- The actual bonded-device removal happens in `factoryReset()` inside `src/bluetooth_manager.cpp` via ESP-IDF Bluetooth APIs.

### Display and backlight

- TFT/LVGL-facing UI code is abstracted behind `EEUI`, which is instantiated in `main.ino`.
- LVGL config is in `include/lv_conf.h`.
- `src/backlight_control.cpp` manages a simple inactivity timer for `SCREEN_BL_PIN`; screen wakeups are triggered by button activity.

## Repository structure notes

- Root-level `README.MD` is broad project documentation; the firmware actually compiled by PlatformIO is under `arduino_code/ESP32-A2DP-SPEAKER`.
- `arduino_code/libraries2` contains vendored libraries used through `lib_dir`; avoid editing there unless the task is explicitly about library internals.
- `.pio/` is build output and dependency cache.

## Practical guidance for edits

- For hardware changes, check `userconfig.h` first before changing module code.
- For Bluetooth transport/UI behavior, the hot path is `main.ino` + `src/bluetooth_manager.cpp` + `src/audio_i2s.cpp`.
- For button behavior, confirm whether the request refers to BOOT button handling or PCA9554 external keys; they are separate systems.
- For album art or shake behavior, inspect both `src/qmi8658_handler.cpp` and `src/album_cover_manager.cpp`.
- Keep changes scoped to `arduino_code/ESP32-A2DP-SPEAKER`; most other directories are vendored dependencies or unrelated experiments.
