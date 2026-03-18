# T3 LoRa32 Firmware

This project is an ESP-IDF firmware for testing LoRa communication and OLED display on a T3 LoRa32 device.

## Dependencies

- **ESP-IDF v5.5.3**: The ESP32 development framework.
- **Hardware**: T3 LoRa32 ESP32 board.

## Building the Project

1.  **Set up ESP-IDF**: Ensure you have ESP-IDF v5.5.3 installed and the environment set up (e.g., run `. export.sh`). See the [ESP-IDF Installation Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) for details.

2.  **Configure the project**:
    ```bash
    idf.py menuconfig
    ```

3.  **Build the firmware**:
    ```bash
    idf.py build
    ```

4.  **Flash and monitor**:
    ```bash
    idf.py flash monitor
    ```

## Project Structure

- `main/main.c`: Main application entry point, handles LoRa initialization and transmission.
- `main/i2c/`: I2C driver implementation.
- `main/lora/`: LoRa driver implementation.
- `main/oled/`: OLED display driver implementation.
- `CMakeLists.txt`: ESP-IDF build configuration.
- `sdkconfig`: ESP-IDF project configuration.
