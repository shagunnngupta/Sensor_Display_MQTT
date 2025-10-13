#ifndef SIMULATION_HELPERS_H
#define SIMULATION_HELPERS_H

// Simulation helpers for BME280-Display-MQTT project
// 
// Since I don't have physical hardware, I created this simulation system
// to generate realistic data and show how the system would work in real life.
// This lets me demonstrate the code concepts without needing the actual components.

#ifdef SIMULATION_MODE
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <random>
#include <sstream>

// This class mimics the ST7789 display
// It keeps track of what would be shown on a real display
// and can save screenshots for documentation
class SimulatedDisplay {
public:
    static const int WIDTH = 240;   // My display is 240x240 pixels
    static const int HEIGHT = 240;
    static const int PIXELS = WIDTH * HEIGHT;
    
    // A virtual frame buffer to hold the "screen" data
    uint16_t frameBuffer[PIXELS];
    
    // Current cursor position and text properties
    int cursorX = 0;
    int cursorY = 0;
    int textSize = 1;
    uint16_t textColor = 0xFFFF;  // White text by default
    uint16_t bgColor = 0x0000;    // Black background
    
    void saveFrame(const std::string& filename) {
        // Creates a PPM image file of the current display state
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            std::cerr << "Could not open file for writing: " << filename << std::endl;
            return;
        }
        
        // PPM header
        outFile << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n";
        
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                uint16_t pixel = frameBuffer[y * WIDTH + x];
                // Convert RGB565 to RGB888
                int r = ((pixel >> 11) & 0x1F) * 255 / 31;
                int g = ((pixel >> 5) & 0x3F) * 255 / 63;
                int b = (pixel & 0x1F) * 255 / 31;
                
                outFile << r << " " << g << " " << b << " ";
            }
            outFile << "\n";
        }
        
        outFile.close();
        std::cout << "Display state saved to " << filename << std::endl;
    }
    
    // Record of display calls for generating documentation
    std::vector<std::string> displayLog;
    
    void logOperation(const std::string& operation) {
        displayLog.push_back(operation);
    }
    
    void saveLog(const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            std::cerr << "Could not open log file for writing: " << filename << std::endl;
            return;
        }
        
        outFile << "=== Display Operation Log ===\n";
        for (const auto& entry : displayLog) {
            outFile << entry << "\n";
        }
        
        outFile.close();
        std::cout << "Display log saved to " << filename << std::endl;
    }
};

// This class generates realistic environmental data
// to simulate what a real BME280 sensor would provide
class SimulatedBME280 {
private:
    // Random number generator to create realistic variations
    std::mt19937 rng;
    
    // I'm using normal distributions to make the data look realistic
    // (values cluster around a mean with natural-looking variations)
    std::normal_distribution<float> tempDist;  // Temperature distribution
    std::normal_distribution<float> humidDist; // Humidity distribution
    std::normal_distribution<float> presDist;  // Pressure distribution
    
public:
    SimulatedBME280() : 
        rng(std::time(nullptr)),  // Seed with current time
        // These parameters produce realistic indoor environmental readings
        tempDist(22.0f, 2.0f),     // Room temp around 22°C ± 2°C
        humidDist(60.0f, 10.0f),   // Indoor humidity ~60% ± 10%
        presDist(1013.25f, 5.0f)   // Standard atm pressure with small changes
    {}
    
    float getTemperature() {
        return tempDist(rng);
    }
    
    float getHumidity() {
        float h = humidDist(rng);
        return (h < 0.0f) ? 0.0f : (h > 100.0f) ? 100.0f : h;
    }
    
    float getPressure() {
        float p = presDist(rng);
        return (p < 900.0f) ? 900.0f : (p > 1100.0f) ? 1100.0f : p;
    }
    
    // Records of calls for documentation
    std::vector<std::tuple<float, float, float>> sensorReadings;
    
    void recordReading(float temp, float humid, float pres) {
        sensorReadings.push_back(std::make_tuple(temp, humid, pres));
    }
    
    void saveReadings(const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            std::cerr << "Could not open sensor log file for writing: " << filename << std::endl;
            return;
        }
        
        outFile << "=== BME280 Sensor Reading Log ===\n";
        outFile << "Temperature(°C),Humidity(%),Pressure(hPa)\n";
        
        for (const auto& reading : sensorReadings) {
            outFile << std::get<0>(reading) << ","
                   << std::get<1>(reading) << ","
                   << std::get<2>(reading) << "\n";
        }
        
        outFile.close();
        std::cout << "Sensor readings saved to " << filename << std::endl;
    }
};

// Simulated MQTT client
class SimulatedMQTT {
public:
    bool connected = false;
    std::string broker;
    int port;
    std::string clientId;
    std::vector<std::string> subscriptions;
    
    // For tracking published messages
    struct PublishedMessage {
        std::string topic;
        std::string payload;
        std::time_t timestamp;
    };
    
    std::vector<PublishedMessage> publishedMessages;
    
    // For tracking received messages (to simulate command responses)
    struct ReceivedMessage {
        std::string topic;
        std::string payload;
        std::time_t timestamp;
    };
    
    std::vector<ReceivedMessage> receivedMessages;
    
    bool connect(const std::string& clientId) {
        this->clientId = clientId;
        connected = true;
        std::cout << "MQTT: Connected to broker " << broker << " as " << clientId << std::endl;
        return true;
    }
    
    bool publish(const std::string& topic, const std::string& payload) {
        if (!connected) {
            std::cerr << "MQTT: Cannot publish, not connected" << std::endl;
            return false;
        }
        
        PublishedMessage msg;
        msg.topic = topic;
        msg.payload = payload;
        msg.timestamp = std::time(nullptr);
        publishedMessages.push_back(msg);
        
        std::cout << "MQTT: Published to " << topic << ": " << payload << std::endl;
        return true;
    }
    
    bool subscribe(const std::string& topic) {
        if (!connected) {
            std::cerr << "MQTT: Cannot subscribe, not connected" << std::endl;
            return false;
        }
        
        subscriptions.push_back(topic);
        std::cout << "MQTT: Subscribed to " << topic << std::endl;
        return true;
    }
    
    void disconnect() {
        if (connected) {
            connected = false;
            std::cout << "MQTT: Disconnected from broker" << std::endl;
        }
    }
    
    // For simulation: manually inject a received message
    void simulateReceivedMessage(const std::string& topic, const std::string& payload) {
        if (!connected) {
            std::cerr << "MQTT: Cannot receive message, not connected" << std::endl;
            return;
        }
        
        // Check if we're subscribed to this topic
        bool isSubscribed = false;
        for (const auto& sub : subscriptions) {
            if (sub == topic) {
                isSubscribed = true;
                break;
            }
        }
        
        if (!isSubscribed) {
            std::cerr << "MQTT: Ignoring message on topic " << topic << " (not subscribed)" << std::endl;
            return;
        }
        
        ReceivedMessage msg;
        msg.topic = topic;
        msg.payload = payload;
        msg.timestamp = std::time(nullptr);
        receivedMessages.push_back(msg);
        
        std::cout << "MQTT: Received message on " << topic << ": " << payload << std::endl;
        
        // Call the callback function if one is registered
        // (in simulation, we would implement this separately)
    }
    
    void saveLog(const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            std::cerr << "Could not open MQTT log file for writing: " << filename << std::endl;
            return;
        }
        
        // Write header
        outFile << "=== MQTT Communication Log ===\n\n";
        
        // Write connection info
        outFile << "Broker: " << broker << ":" << port << "\n";
        outFile << "Client ID: " << clientId << "\n";
        outFile << "Status: " << (connected ? "Connected" : "Disconnected") << "\n\n";
        
        // Write subscriptions
        outFile << "Subscriptions:\n";
        for (const auto& sub : subscriptions) {
            outFile << "  - " << sub << "\n";
        }
        outFile << "\n";
        
        // Write published messages
        outFile << "Published Messages:\n";
        for (const auto& msg : publishedMessages) {
            char timeStr[30];
            struct tm* timeinfo = localtime(&msg.timestamp);
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
            
            outFile << timeStr << " [" << msg.topic << "]: " << msg.payload << "\n";
        }
        outFile << "\n";
        
        // Write received messages
        outFile << "Received Messages:\n";
        for (const auto& msg : receivedMessages) {
            char timeStr[30];
            struct tm* timeinfo = localtime(&msg.timestamp);
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
            
            outFile << timeStr << " [" << msg.topic << "]: " << msg.payload << "\n";
        }
        
        outFile.close();
        std::cout << "MQTT log saved to " << filename << std::endl;
    }
};

// Export simulated objects for use in main code
extern SimulatedDisplay simDisplay;
extern SimulatedBME280 simSensor;
extern SimulatedMQTT simMqtt;

#endif // SIMULATION_MODE

#endif // SIMULATION_HELPERS_H
