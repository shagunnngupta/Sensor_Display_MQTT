# Embedded Mini Project: Sensor-Display-MQTT Integration

This project implements a complete embedded system that integrates a BME280 environmental sensor, ST7789 TFT display, and MQTT communication on an ESP32 development board.

## Project Overview

The system performs the following tasks:
1. Reads temperature, humidity, and pressure data from a BME280 sensor using a custom low-level driver
2. Displays the sensor data on a 240x240 ST7789 TFT display with a simple user interface
3. Publishes sensor data to an MQTT broker
4. Subscribes to command topics to control the system remotely
5. Provides visual feedback of system status and sensor readings

## Hardware Components

- **MCU**: ESP32 Development Board
- **Sensor**: BME280 Temperature, Humidity, and Pressure Sensor
- **Display**: ST7789 1.9-inch 240x240 TFT LCD
- **Connections**:
  - **BME280 to ESP32**:
    - VCC → 3.3V
    - GND → GND
    - SDA → GPIO 21
    - SCL → GPIO 22
  - **ST7789 TFT to ESP32**:
    - VCC → 3.3V
    - GND → GND
    - CS → GPIO 5
    - DC → GPIO 2
    - MOSI → GPIO 23
    - SCK → GPIO 18
    - RST → GPIO 4
    - BLK → 3.3V

## Software Architecture

### Custom BME280 Sensor Driver
Instead of using high-level libraries, we implemented a custom driver directly communicating with the BME280 sensor via I2C. The driver handles:
- Sensor initialization and configuration
- Raw data reading from sensor registers
- Compensation and calibration using datasheet formulas
- Conversion to human-readable values (°C, %, hPa)

### Display Interface
Using TFT_eSPI library for the ST7789 display to create:
- Sensor readings visualization
- Connection status indicators
- Interactive button for display reset

### MQTT Integration
Using the PubSubClient library for MQTT communication:
- Publishing sensor data every 2 seconds
- Subscribing to commands for remote control
- Handling disconnections and reconnections

## MQTT Topics

- **Publish**: `sensor/bme280/data` - JSON formatted sensor data
- **Subscribe**: `sensor/bme280/commands` - Commands to control the system

### Supported Commands
- `RESET` - Clears and redraws the display
- `LED_ON` - Turns on the built-in LED
- `LED_OFF` - Turns off the built-in LED

## Project Setup

1. Configure the Wi-Fi credentials in `main.cpp`
2. Upload the code to the ESP32
3. Connect to the MQTT broker using any MQTT client (e.g., MQTT Explorer)
4. Publish commands to the subscribed topic to interact with the device

## Development Challenges

1. **BME280 Driver Implementation**: 
   - Understanding the datasheet and implementing the complex compensation formulas
   - Calibrating the sensor for accurate readings
   - Dealing with the non-sequential register layout

2. **Display Integration**:
   - Optimizing refresh rate to prevent flickering
   - Creating an intuitive UI that fits in a small display area

3. **MQTT Reliability**:
   - Handling connection failures gracefully
   - Managing reconnection attempts while keeping the system responsive
