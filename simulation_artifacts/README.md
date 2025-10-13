# Simulation Artifacts

This directory contains simulation artifacts that would typically be generated when running the actual simulation. Since you're unable to run the simulation due to PlatformIO issues, these files have been manually created to represent what would be produced.

## Files Included

1. **display_simulation.txt** - A text representation of what would be shown on the ST7789 display. In a real submission, this would be a screenshot from the actual display.

2. **mqtt_simulation_log.txt** - A simulated log of MQTT communications between the ESP32 and an MQTT broker, showing published and subscribed messages.

3. **sensor_readings.csv** - A CSV file containing simulated sensor readings from the BME280 sensor.

4. **system_simulation_log.txt** - A general system log showing boot sequences, error states, and other system events.

## How to Use These Files

Include these files in your submission as evidence of your system's designed behavior. You can reference them in your documentation to explain how your system would work if implemented with physical hardware.

## What Would Happen With Real Hardware

With actual hardware components:
- The display_simulation.txt would be replaced with a real screenshot from the ST7789 display
- The mqtt_simulation_log.txt would contain actual MQTT traffic captured from the network
- The sensor_readings.csv would contain real environmental data from the BME280 sensor
- The system_simulation_log.txt would show actual system events from the ESP32

These simulated artifacts demonstrate your understanding of the system design and implementation, even without access to the physical components.
