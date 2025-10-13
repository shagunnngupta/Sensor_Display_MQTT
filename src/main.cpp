#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "bme280_driver.h"

// Hardware pins - I'm using the default I2C pins on ESP32
// You can change these if your wiring is different
#define SDA_PIN 21  // I2C data line for BME280 sensor
#define SCL_PIN 22  // I2C clock line for BME280 sensor
#define LED_PIN 2   // This is the built-in LED on most ESP32 boards

// WiFi stuff - don't forget to put your actual WiFi details here
// or the ESP32 won't connect to your network!
const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";

// MQTT settings - using HiveMQ's free public broker
// No authentication needed, but not secure for real applications
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32_Sensor_Client";
const char* mqtt_topic_publish = "sensor/bme280/data";
const char* mqtt_topic_subscribe = "sensor/bme280/commands";

// Creating the objects we need for the project
TFT_eSPI tft = TFT_eSPI();  // This handles our display
BME280_Driver bme280;       // My custom sensor driver (no high-level libraries!)
WiFiClient espClient;       // Handles WiFi connection
PubSubClient mqttClient(espClient); // Handles MQTT messaging

// Some global variables to track the system state
bool ledState = false;          // Is the LED on or off?
bool displayCleared = false;    // Has the display been cleared?
unsigned long lastUpdateTime = 0;       // When did we last update the display?
const unsigned long updateInterval = 2000; // Update every 2 seconds (not too fast, not too slow)

// A simple struct to hold all the sensor readings in one place
// Makes the code cleaner than having separate variables
struct SensorData {
  float temperature;  // in Celsius
  float humidity;     // in % relative humidity
  float pressure;     // in hPa (hectopascals)
} sensorData;

// Display colors for our UI - keeping it simple but with good contrast
// These make the UI look more professional than just using default colors
#define BACKGROUND TFT_BLACK    // Black background is easy on the eyes
#define TEXT_COLOR TFT_WHITE    // White text for good contrast
#define STATUS_COLOR TFT_GREEN  // Green for "good/connected" status
#define ERROR_COLOR TFT_RED     // Red for errors or "disconnected" status
#define TITLE_COLOR TFT_CYAN    // Cyan for titles and important readings

// --- Function prototypes ---
void setupWiFi();
void setupMQTT();
void setupDisplay();
void setupBME280();
void readSensorData();
void updateDisplay();
void publishSensorData();
void handleMQTTCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void drawButton(int x, int y, int w, int h, String label);

void setup() {
  // Start serial at 115200 baud - makes it easier to debug
  Serial.begin(115200);
  Serial.println("\n--- Sensor Display MQTT Integration Project ---");
  
  // Set up the LED so we can toggle it with MQTT commands
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED off
  
  // Start the I2C bus for communicating with the BME280 sensor
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize all our components one by one
  setupDisplay();  // First the display so we can show progress
  setupBME280();   // Then the sensor
  setupWiFi();     // Connect to WiFi
  setupMQTT();     // Connect to MQTT broker
  
  // Show initial sensor readings on the display
  readSensorData();
  updateDisplay();
}

void loop() {
  // Always check if we're still connected to MQTT
  // If not, try to reconnect (doesn't block if WiFi is down)
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();  // Process any incoming MQTT messages
  
  // Time to update our readings? We use millis() instead of delay()
  // so the ESP32 can still process MQTT messages between updates
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= updateInterval) {
    readSensorData();     // Get fresh sensor readings
    updateDisplay();      // Update the display with new values
    publishSensorData();  // Send the data to MQTT broker
    lastUpdateTime = currentTime;  // Reset the timer
  }
}

void setupWiFi() {
  tft.fillRect(0, 20, 240, 40, BACKGROUND);
  tft.setTextColor(TEXT_COLOR, BACKGROUND);
  tft.setCursor(10, 30);
  tft.print("Connecting to WiFi...");
  
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    tft.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    tft.fillRect(0, 20, 240, 40, BACKGROUND);
    tft.setCursor(10, 30);
    tft.setTextColor(STATUS_COLOR, BACKGROUND);
    tft.print("WiFi: Connected");
    tft.setCursor(10, 50);
    tft.print(WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed!");
    
    tft.fillRect(0, 20, 240, 40, BACKGROUND);
    tft.setCursor(10, 30);
    tft.setTextColor(ERROR_COLOR, BACKGROUND);
    tft.print("WiFi: Failed!");
  }
  delay(1000);
}

void setupMQTT() {
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(handleMQTTCallback);
}

void setupDisplay() {
  // Initialize the display
  tft.init();
  tft.setRotation(0); // Portrait orientation
  tft.fillScreen(BACKGROUND);
  
  // Draw title
  tft.setTextSize(2);
  tft.setTextColor(TITLE_COLOR, BACKGROUND);
  tft.setCursor(10, 5);
  tft.println("BME280 Sensor");
  
  // Draw line separator
  tft.drawLine(0, 25, 240, 25, TITLE_COLOR);
  
  // Initial message
  tft.setTextSize(1);
  tft.setTextColor(TEXT_COLOR, BACKGROUND);
  tft.setCursor(10, 40);
  tft.println("Starting system...");
  
  Serial.println("Display initialized");
}

void setupBME280() {
  tft.fillRect(0, 60, 240, 20, BACKGROUND);
  tft.setCursor(10, 70);
  
  // Initialize the BME280 sensor with our custom driver
  if (bme280.begin()) {
    Serial.println("BME280 sensor found and initialized!");
    tft.setTextColor(STATUS_COLOR, BACKGROUND);
    tft.print("BME280: OK");
  } else {
    Serial.println("Could not find BME280 sensor!");
    tft.setTextColor(ERROR_COLOR, BACKGROUND);
    tft.print("BME280: Not Found!");
  }
  delay(1000);
}

void readSensorData() {
  // Read data from our custom BME280 driver
  sensorData.temperature = bme280.readTemperature();
  sensorData.humidity = bme280.readHumidity();
  sensorData.pressure = bme280.readPressure();
  
  // Print to serial for debugging
  Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%, Pressure: %.2f hPa\n", 
                sensorData.temperature, 
                sensorData.humidity, 
                sensorData.pressure);
}

void updateDisplay() {
  // Clear the data area
  tft.fillRect(0, 90, 240, 110, BACKGROUND);
  
  // Show connection status
  tft.setCursor(10, 95);
  tft.setTextColor(mqttClient.connected() ? STATUS_COLOR : ERROR_COLOR, BACKGROUND);
  tft.print("MQTT: ");
  tft.println(mqttClient.connected() ? "Connected" : "Disconnected");
  
  // Display sensor data
  tft.setTextColor(TEXT_COLOR, BACKGROUND);
  tft.setCursor(10, 115);
  tft.print("Temperature: ");
  tft.setTextColor(TITLE_COLOR, BACKGROUND);
  tft.print(sensorData.temperature, 1);
  tft.println(" C");
  
  tft.setTextColor(TEXT_COLOR, BACKGROUND);
  tft.setCursor(10, 135);
  tft.print("Humidity: ");
  tft.setTextColor(TITLE_COLOR, BACKGROUND);
  tft.print(sensorData.humidity, 1);
  tft.println(" %");
  
  tft.setTextColor(TEXT_COLOR, BACKGROUND);
  tft.setCursor(10, 155);
  tft.print("Pressure: ");
  tft.setTextColor(TITLE_COLOR, BACKGROUND);
  tft.print(sensorData.pressure, 1);
  tft.println(" hPa");
  
  // Display LED status
  tft.setTextColor(TEXT_COLOR, BACKGROUND);
  tft.setCursor(10, 175);
  tft.print("LED Status: ");
  tft.setTextColor(ledState ? STATUS_COLOR : ERROR_COLOR, BACKGROUND);
  tft.println(ledState ? "ON" : "OFF");
  
  // Draw reset button
  drawButton(60, 200, 120, 30, "RESET");
}

void publishSensorData() {
  if (!mqttClient.connected()) {
    return;
  }
  
  // Create JSON-formatted string with sensor data
  char buffer[100];
  snprintf(buffer, sizeof(buffer), 
           "{\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f}",
           sensorData.temperature, 
           sensorData.humidity, 
           sensorData.pressure);
  
  // Publish to MQTT topic
  mqttClient.publish(mqtt_topic_publish, buffer);
  Serial.printf("Published to %s: %s\n", mqtt_topic_publish, buffer);
}

void handleMQTTCallback(char* topic, byte* payload, unsigned int length) {
  // Create a null-terminated string from the payload
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  Serial.printf("Message received on topic [%s]: %s\n", topic, message);
  
  // Handle different commands
  if (strcmp(message, "RESET") == 0) {
    Serial.println("Resetting display");
    displayCleared = true;
    tft.fillScreen(BACKGROUND);
    setupDisplay();
    updateDisplay();
  } 
  else if (strcmp(message, "LED_ON") == 0) {
    Serial.println("Turning LED ON");
    digitalWrite(LED_PIN, HIGH);
    ledState = true;
  } 
  else if (strcmp(message, "LED_OFF") == 0) {
    Serial.println("Turning LED OFF");
    digitalWrite(LED_PIN, LOW);
    ledState = false;
  }
  
  // Update the display to reflect any changes
  updateDisplay();
}

void reconnectMQTT() {
  // Try to connect to MQTT broker
  if (WiFi.status() != WL_CONNECTED) {
    return; // Can't connect to MQTT without WiFi
  }
  
  Serial.print("Connecting to MQTT broker...");
  
  if (mqttClient.connect(mqtt_client_id)) {
    Serial.println("connected");
    
    // Subscribe to command topic
    mqttClient.subscribe(mqtt_topic_subscribe);
    Serial.printf("Subscribed to topic: %s\n", mqtt_topic_subscribe);
  } else {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" will try again later");
  }
}

void drawButton(int x, int y, int w, int h, String label) {
  // Draw button outline
  tft.drawRect(x, y, w, h, TEXT_COLOR);
  
  // Fill button
  tft.fillRect(x+1, y+1, w-2, h-2, BACKGROUND);
  
  // Draw label
  tft.setTextColor(TEXT_COLOR, BACKGROUND);
  tft.setTextSize(1);
  
  // Center text
  int16_t x1, y1;
  uint16_t textWidth, textHeight;
  tft.getTextBounds(label, 0, 0, &x1, &y1, &textWidth, &textHeight);
  
  tft.setCursor(x + (w - textWidth) / 2, y + (h - textHeight) / 2 + textHeight);
  tft.print(label);
}
