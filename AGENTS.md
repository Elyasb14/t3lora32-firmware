# AGENTS.md - T3 LoRa32 Firmware

## Project Overview
This is an ESP32 firmware project for testing LoRa communication and OLED display on a T3 LoRa32 device. The project uses ESP-IDF v5.5.3.

## Project Structure
- `main/main.c`: Main application entry point, handles LoRa initialization and transmission
- `main/i2c/`: I2C driver implementation
- `main/lora/`: LoRa driver implementation  
- `main/oled/`: OLED display driver implementation
- `CMakeLists.txt`: ESP-IDF build configuration
- `sdkconfig`: ESP-IDF project configuration

## Build Commands
The project uses ESP-IDF build system. Standard ESP-IDF commands:
- `idf.py build` - Build the project
- `idf.py flash` - Flash to device
- `idf.py monitor` - Monitor serial output
- `idf.py menuconfig` - Configure project options

### Device Specific Commands
For the T3 LoRa32 device connected to `/dev/tty.usbserial-576A0039151`:
- `idf.py -p /dev/tty.usbserial-576A0039151 flash monitor` - Build, flash, and monitor (Recommended)

## Code Style
- Follow ESP-IDF conventions
- Include appropriate headers
- Use FreeRTOS task functions properly
- Handle hardware initialization in `app_main()`

## Key Dependencies
- ESP-IDF v5.5.3
- FreeRTOS (included in ESP-IDF)
- SPI driver for LoRa
- I2C driver for OLED

## Note
This is a test/development firmware for LoRa communication with OLED display on ESP32.
