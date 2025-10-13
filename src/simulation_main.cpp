#ifdef SIMULATION_MODE

#include "simulation_helpers.h"
#include <iostream>
#include <thread>
#include <chrono>

// Create our simulation objects - these replace the real hardware
SimulatedDisplay simDisplay;  // Instead of the ST7789 LCD
SimulatedBME280 simSensor;   // Instead of the real BME280 sensor
SimulatedMQTT simMqtt;       // Instead of a real MQTT connection

// This runs instead of the Arduino setup() and loop() when in simulation mode
int main() {
    // Welcome message
    std::cout << "=== BME280 Sensor Display MQTT Simulator ===\n";
    std::cout << "This shows how the system would work with real hardware\n";
    std::cout << "Running through a typical scenario with sensor readings and MQTT commands\n\n";
    
    // Set up the MQTT broker info (same as in the real code)
    simMqtt.broker = "broker.hivemq.com";  // Free public broker
    simMqtt.port = 1883;                   // Standard MQTT port
    
    // First, let's initialize everything (like in setup())
    std::cout << "Booting up the system...\n";
    
    // First we'd initialize the display
    std::cout << "Starting the display...\n";
    simDisplay.logOperation("Initialize display");
    simDisplay.logOperation("Set rotation to 0 (Portrait mode)");
    simDisplay.logOperation("Fill screen with black background");
    simDisplay.logOperation("Draw title bar: BME280 Sensor");
    simDisplay.logOperation("Draw separator line below title");
    
    // Now connect to WiFi
    std::cout << "Trying to connect to WiFi network...\n";
    simDisplay.logOperation("Show 'Connecting to WiFi...' on screen");
    
    // Wait a bit - WiFi connection takes time in real life
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // WiFi connected!
    std::cout << "WiFi connected successfully!\n";
    simDisplay.logOperation("Update to show 'WiFi: Connected'");
    simDisplay.logOperation("Display IP address: 192.168.1.100");
    
    // Now connect to MQTT broker
    std::cout << "Connecting to MQTT broker at " << simMqtt.broker << "...\n";
    simMqtt.connect("ESP32_Sensor_Client");  // Same client ID as real code
    
    // Subscribe to the command topic to receive instructions
    std::cout << "Subscribing to command topic...\n";
    simMqtt.subscribe("sensor/bme280/commands");  // Same topic as real code
    
    // Simulate BME280 sensor initialization
    std::cout << "Initializing BME280 sensor...\n";
    simDisplay.logOperation("Display 'BME280: OK'");
    
    // Now let's run our main loop - just like the loop() function in Arduino
    // We'll run for 10 cycles (in real life this would run forever)
    const int totalIterations = 10;
    std::cout << "\nStarting main loop - will run for " << totalIterations << " cycles\n";
    
    for (int i = 0; i < totalIterations; i++) {
        std::cout << "\n----- Cycle " << (i + 1) << " of " << totalIterations << " -----\n";
        
        // Read new data from our sensor
        float temperature = simSensor.getTemperature();
        float humidity = simSensor.getHumidity();
        float pressure = simSensor.getPressure();
        
        // Save the readings to our log (for the assignment submission)
        simSensor.recordReading(temperature, humidity, pressure);
        
        // Print what we'd see on the Serial Monitor
        std::cout << "Sensor readings:\n";
        std::cout << "  Temperature: " << temperature << "°C\n";
        std::cout << "  Humidity: " << humidity << "%\n";
        std::cout << "  Pressure: " << pressure << " hPa\n";
        
        // Log what would happen on the display
        simDisplay.logOperation("Clear sensor data area");
        simDisplay.logOperation("Show MQTT connection status: Connected");
        simDisplay.logOperation("Update temperature reading: " + std::to_string(temperature) + " °C");
        simDisplay.logOperation("Update humidity reading: " + std::to_string(humidity) + " %");
        simDisplay.logOperation("Update pressure reading: " + std::to_string(pressure) + " hPa");
        simDisplay.logOperation("Show LED status: OFF");
        
        // Now publish our data to MQTT (as JSON, just like in the real code)
        std::stringstream json;
        json << "{\"temperature\":" << temperature 
             << ",\"humidity\":" << humidity 
             << ",\"pressure\":" << pressure << "}";
        
        std::cout << "Publishing to MQTT topic: sensor/bme280/data\n";
        simMqtt.publish("sensor/bme280/data", json.str());  // Same as real code
        
        // Every third cycle, simulate receiving an MQTT command
        if (i % 3 == 2) {
            if (i % 2 == 0) {
                std::cout << "Simulating MQTT command: LED_ON\n";
                simMqtt.simulateReceivedMessage("sensor/bme280/commands", "LED_ON");
                simDisplay.logOperation("Update LED status: ON");
            } else {
                std::cout << "Simulating MQTT command: LED_OFF\n";
                simMqtt.simulateReceivedMessage("sensor/bme280/commands", "LED_OFF");
                simDisplay.logOperation("Update LED status: OFF");
            }
        }
        
        // If at the middle point, simulate a display reset command
        if (i == totalIterations / 2) {
            std::cout << "Simulating MQTT command: RESET\n";
            simMqtt.simulateReceivedMessage("sensor/bme280/commands", "RESET");
            simDisplay.logOperation("Reset display");
            simDisplay.logOperation("Redraw interface");
        }
        
        // Wait before next cycle
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
    
    // Save simulation logs and artifacts
    std::cout << "\nSimulation complete. Saving artifacts...\n";
    
    // Save display frame as an image
    simDisplay.saveFrame("display_simulation.ppm");
    
    // Save display operation log
    simDisplay.saveLog("display_operations.log");
    
    // Save sensor readings
    simSensor.saveReadings("sensor_readings.csv");
    
    // Save MQTT communication log
    simMqtt.saveLog("mqtt_communication.log");
    
    std::cout << "\nSimulation artifacts saved. Use these files for your assignment submission.\n";
    std::cout << "1. display_simulation.ppm - A simulated screenshot of the display\n";
    std::cout << "2. display_operations.log - Log of all display operations\n";
    std::cout << "3. sensor_readings.csv - Record of all sensor readings\n";
    std::cout << "4. mqtt_communication.log - MQTT communication transcript\n\n";
    
    return 0;
}

#endif // SIMULATION_MODE
