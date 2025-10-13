#ifndef BME280_DRIVER_H
#define BME280_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

// BME280 can have two different I2C addresses depending on how SDO pin is connected
// Most modules use 0x76 (SDO to GND), but some use 0x77 (SDO to VCC)
#define BME280_ADDRESS_PRIMARY     0x76  // Most common address
#define BME280_ADDRESS_SECONDARY   0x77  // Alternative address

// These are all the registers we need to interact with the BME280
// I got these from the datasheet - it's like a map of the sensor's memory
#define BME280_REG_ID              0xD0  // Chip ID register - should return 0x60
#define BME280_REG_RESET           0xE0  // Writing 0xB6 here resets the sensor
#define BME280_REG_STATUS          0xF3  // Status register - tells us if it's busy
#define BME280_REG_CTRL_MEAS       0xF4  // Control register for temp & pressure
#define BME280_REG_CONFIG          0xF5  // Configuration register
#define BME280_REG_CTRL_HUM        0xF2  // Control register for humidity

// Data registers that contain the raw sensor readings
#define BME280_REG_PRESS_MSB       0xF7  // Pressure data [19:12]
#define BME280_REG_PRESS_LSB       0xF8  // Pressure data [11:4]
#define BME280_REG_PRESS_XLSB      0xF9  // Pressure data [3:0]
#define BME280_REG_TEMP_MSB        0xFA  // Temperature data [19:12]
#define BME280_REG_TEMP_LSB        0xFB  // Temperature data [11:4]
#define BME280_REG_TEMP_XLSB       0xFC  // Temperature data [3:0]
#define BME280_REG_HUM_MSB         0xFD  // Humidity data [15:8]
#define BME280_REG_HUM_LSB         0xFE  // Humidity data [7:0]

// These registers hold calibration data that's unique to each BME280 sensor
// We need to read these values first, then use them in formulas to get accurate readings
// This is the tricky part that most libraries hide from you!
#define BME280_REG_DIG_T1          0x88  // Temperature calibration data
#define BME280_REG_DIG_T2          0x8A
#define BME280_REG_DIG_T3          0x8C
#define BME280_REG_DIG_P1          0x8E  // Pressure calibration data
#define BME280_REG_DIG_P2          0x90
#define BME280_REG_DIG_P3          0x92
#define BME280_REG_DIG_P4          0x94
#define BME280_REG_DIG_P5          0x96
#define BME280_REG_DIG_P6          0x98
#define BME280_REG_DIG_P7          0x9A
#define BME280_REG_DIG_P8          0x9C
#define BME280_REG_DIG_P9          0x9E
#define BME280_REG_DIG_H1          0xA1  // Humidity calibration data
#define BME280_REG_DIG_H2          0xE1  // Note how these aren't in sequence!
#define BME280_REG_DIG_H3          0xE3  // Bosch really made it confusing...
#define BME280_REG_DIG_H4          0xE4  // H4 and H5 are actually split across
#define BME280_REG_DIG_H5          0xE5  // multiple registers in a weird way
#define BME280_REG_DIG_H6          0xE7

// Sensor configuration values - keeping these simple for stability
// You could increase these for more accuracy, but it uses more power
#define BME280_TEMP_OSR            0x01  // Temperature oversampling x1 (basic accuracy)
#define BME280_PRES_OSR            0x01  // Pressure oversampling x1
#define BME280_HUM_OSR             0x01  // Humidity oversampling x1
#define BME280_MODE                0x03  // Normal mode (continuous readings)

// This structure holds all the calibration coefficients for the BME280
// Each sensor has unique values that we have to read from its memory
// We'll use these values in complex formulas from the datasheet
typedef struct {
    // Temperature compensation values
    uint16_t dig_T1;  // Always positive
    int16_t  dig_T2;  // Can be negative
    int16_t  dig_T3;  // Can be negative
    
    // Pressure compensation values
    uint16_t dig_P1;  // Always positive
    int16_t  dig_P2;  // Can be negative
    int16_t  dig_P3;  // Can be negative
    int16_t  dig_P4;  // Can be negative
    int16_t  dig_P5;  // Can be negative
    int16_t  dig_P6;  // Can be negative
    int16_t  dig_P7;  // Can be negative
    int16_t  dig_P8;  // Can be negative
    int16_t  dig_P9;  // Can be negative
    
    // Humidity compensation values
    uint8_t  dig_H1;  // Always positive
    int16_t  dig_H2;  // Can be negative
    uint8_t  dig_H3;  // Always positive
    int16_t  dig_H4;  // Can be negative, stored weird
    int16_t  dig_H5;  // Can be negative, stored weird
    int8_t   dig_H6;  // Can be negative
} BME280_CalibrationData;

class BME280_Driver {
private:
    TwoWire *wire;             // Pointer to the I2C interface
    uint8_t deviceAddress;     // I2C address of the BME280 (0x76 or 0x77)
    BME280_CalibrationData calibData;  // Holds all the calibration coefficients
    int32_t t_fine;            // Temperature fine-resolution value, used in other calculations

    // These are our low-level I2C functions to talk to the sensor
    // I'm implementing these myself instead of using a library
    uint8_t readRegister(uint8_t reg);  // Read a single register
    void readRegisters(uint8_t reg, uint8_t *buffer, uint8_t length); // Read multiple registers
    void writeRegister(uint8_t reg, uint8_t value);  // Write to a register
    void readCalibrationData();  // Read all calibration data from the sensor
    
    // These are the complex compensation formulas straight from the BME280 datasheet
    // They convert raw ADC values to actual temperature, pressure, and humidity
    int32_t compensateTemperature(int32_t adcTemp);  // Also updates t_fine
    uint32_t compensatePressure(int32_t adcPres);    // Needs t_fine from temperature
    uint32_t compensateHumidity(int32_t adcHum);     // Needs t_fine from temperature

public:
    // Create a new BME280 driver, optionally specifying I2C interface and address
    BME280_Driver(TwoWire *w = &Wire, uint8_t addr = BME280_ADDRESS_PRIMARY);
    
    // Basic functions to initialize and check the sensor
    bool begin();         // Initialize the sensor, returns true if found
    void reset();         // Soft-reset the sensor
    uint8_t getChipID();  // Should return 0x60 if it's a BME280
    
    // The main functions you'll use to get sensor readings
    float readTemperature(); // Get temperature in °C
    float readPressure();    // Get pressure in hPa (divide by 100 from Pa)
    float readHumidity();    // Get relative humidity in %
    
    // Check if the sensor is currently taking a measurement
    bool isMeasuring();  // Returns true if a measurement is in progress
};

#endif // BME280_DRIVER_H
