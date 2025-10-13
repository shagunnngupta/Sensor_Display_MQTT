#include "bme280_driver.h"

// Constructor - just store the I2C interface and address for later use
BME280_Driver::BME280_Driver(TwoWire *w, uint8_t addr) {
    wire = w;  // Which I2C interface to use
    deviceAddress = addr;  // Which address the sensor is on
    t_fine = 0;  // We'll calculate this later when reading temp
}

bool BME280_Driver::begin() {
    // First make sure we can talk to the sensor
    uint8_t chipId = getChipID();
    if (chipId != 0x60) {
        // Hmm, not getting the right ID. Either it's not connected
        // or it's not a BME280 (might be a BMP280 which is similar)
        return false;  // 0x60 is the BME280's unique ID
    }
    
    // Let's start fresh by resetting the sensor
    reset();
    delay(10);  // Give it a moment to reboot
    
    // The status register bit 0 is set while the device is copying
    // NVM data to image registers, so let's wait until it's done
    while (readRegister(BME280_REG_STATUS) & 0x01) {
        delay(10);  // Keep checking every 10ms
    }
    
    // Now read all the factory calibration data from the sensor
    // This is super important - each BME280 has unique values!
    readCalibrationData();
    
    // Now let's configure the sensor for our needs
    // First set humidity oversampling (this register must be written first!)
    writeRegister(BME280_REG_CTRL_HUM, BME280_HUM_OSR);
    
    // Now set temperature and pressure oversampling, and sensor mode
    // We combine these settings into a single register value
    uint8_t config = (BME280_TEMP_OSR << 5) | (BME280_PRES_OSR << 2) | BME280_MODE;
    writeRegister(BME280_REG_CTRL_MEAS, config);
    
    // We could set filter coefficients and standby time here,
    // but we'll just use defaults to keep things simple
    writeRegister(BME280_REG_CONFIG, 0x00);
    
    return true;  // Everything looks good!
}

void BME280_Driver::reset() {
    // The datasheet says writing 0xB6 to the reset register
    // triggers a power-on-reset procedure
    writeRegister(BME280_REG_RESET, 0xB6);
    // Now the sensor will reboot itself
}

uint8_t BME280_Driver::getChipID() {
    // This register contains a fixed value that identifies the chip
    // BME280 should return 0x60
    return readRegister(BME280_REG_ID);
}

float BME280_Driver::readTemperature() {
    // Temperature data is stored across 3 registers (20 bits total)
    // We need to read all 3 and combine them
    uint8_t buffer[3];
    readRegisters(BME280_REG_TEMP_MSB, buffer, 3);
    
    // Combine the 3 bytes into a 20-bit value
    // The data is stored as MSB, LSB, XLSB (4 bits)
    int32_t adcTemp = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);
    
    // The raw ADC value isn't useful - we need to run it through
    // the compensation formula from the datasheet
    int32_t tempComp = compensateTemperature(adcTemp);
    
    // The compensated value is in 0.01°C units, so we divide by 100
    // to get degrees Celsius
    return tempComp / 100.0f;
}

float BME280_Driver::readPressure() {
    // Important! Pressure calculation depends on temperature
    // so we need to make sure t_fine is calculated first
    if (t_fine == 0) {
        readTemperature();  // This will set t_fine for us
    }
    
    // Pressure data is also stored across 3 registers (20 bits total)
    uint8_t buffer[3];
    readRegisters(BME280_REG_PRESS_MSB, buffer, 3);
    
    // Combine the bytes just like for temperature
    int32_t adcPres = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);
    
    // The pressure compensation formula is even more complex
    // and needs the t_fine value we calculated in readTemperature()
    uint32_t presComp = compensatePressure(adcPres);
    
    // Pressure is in Pascals, but people usually work with hPa
    // so let's convert (1 hPa = 100 Pa)
    return presComp / 100.0f; // Pa to hPa (hectopascals)
}

float BME280_Driver::readHumidity() {
    // Just like pressure, humidity calculation also needs temperature first
    if (t_fine == 0) {
        readTemperature();  // This will set t_fine
    }
    
    // Humidity data is stored in 2 registers (16 bits total)
    uint8_t buffer[2];
    readRegisters(BME280_REG_HUM_MSB, buffer, 2);
    
    // Combine the two bytes
    int32_t adcHum = (buffer[0] << 8) | buffer[1];
    
    // Now apply the humidity compensation formula
    uint32_t humComp = compensateHumidity(adcHum);
    
    // The formula gives humidity in Q22.10 format
    // (22 integer bits, 10 fractional bits)
    // To get percent, we divide by 1024 (2^10)
    return humComp / 1024.0f;  // Convert to %RH
}

bool BME280_Driver::isMeasuring() {
    // Bit 3 in the status register tells us if the sensor is
    // currently doing a measurement
    uint8_t status = readRegister(BME280_REG_STATUS);
    return (status & 0x08) != 0;  // Check if bit 3 is set
}

// === Low-level I2C communication functions ===
// This is where we're really getting our hands dirty with direct hardware access

// Read a single byte from a register
uint8_t BME280_Driver::readRegister(uint8_t reg) {
    uint8_t value;
    
    // I2C works by first sending the address of the register we want to read
    wire->beginTransmission(deviceAddress);
    wire->write(reg);  // "I want to read from this register"
    wire->endTransmission();
    
    // Then we request one byte from the device
    wire->requestFrom(deviceAddress, (uint8_t)1);
    value = wire->read();  // Get the byte
    
    return value;
}

// Read multiple bytes from consecutive registers
void BME280_Driver::readRegisters(uint8_t reg, uint8_t *buffer, uint8_t length) {
    // Same as above, but we request multiple bytes
    wire->beginTransmission(deviceAddress);
    wire->write(reg);  // Start from this register
    wire->endTransmission();
    
    // Ask for 'length' bytes
    wire->requestFrom(deviceAddress, length);
    
    // Read all the bytes into our buffer
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = wire->read();
    }
}

// Write a value to a register
void BME280_Driver::writeRegister(uint8_t reg, uint8_t value) {
    wire->beginTransmission(deviceAddress);
    wire->write(reg);    // "I want to write to this register"
    wire->write(value);  // "...and this is the value"
    wire->endTransmission();
    
    // This is how we configure the sensor - by writing specific
    // values to specific registers
}

void BME280_Driver::readCalibrationData() {
    uint8_t buffer[24];
    
    // Read temperature and pressure calibration data (registers 0x88-0x9F)
    readRegisters(BME280_REG_DIG_T1, buffer, 24);
    
    calibData.dig_T1 = (buffer[1] << 8) | buffer[0];
    calibData.dig_T2 = (buffer[3] << 8) | buffer[2];
    calibData.dig_T3 = (buffer[5] << 8) | buffer[4];
    
    calibData.dig_P1 = (buffer[7] << 8) | buffer[6];
    calibData.dig_P2 = (buffer[9] << 8) | buffer[8];
    calibData.dig_P3 = (buffer[11] << 8) | buffer[10];
    calibData.dig_P4 = (buffer[13] << 8) | buffer[12];
    calibData.dig_P5 = (buffer[15] << 8) | buffer[14];
    calibData.dig_P6 = (buffer[17] << 8) | buffer[16];
    calibData.dig_P7 = (buffer[19] << 8) | buffer[18];
    calibData.dig_P8 = (buffer[21] << 8) | buffer[20];
    calibData.dig_P9 = (buffer[23] << 8) | buffer[22];
    
    // Read humidity calibration data (registers are not sequential)
    calibData.dig_H1 = readRegister(BME280_REG_DIG_H1);
    
    readRegisters(BME280_REG_DIG_H2, buffer, 7);
    calibData.dig_H2 = (buffer[1] << 8) | buffer[0];
    calibData.dig_H3 = buffer[2];
    
    calibData.dig_H4 = (buffer[3] << 4) | (buffer[4] & 0x0F);
    calibData.dig_H5 = (buffer[5] << 4) | (buffer[4] >> 4);
    calibData.dig_H6 = (int8_t)buffer[6];
}

// Temperature compensation formula from BME280 datasheet
int32_t BME280_Driver::compensateTemperature(int32_t adcTemp) {
    int32_t var1, var2, temperature;
    
    var1 = ((((adcTemp >> 3) - ((int32_t)calibData.dig_T1 << 1))) * 
            ((int32_t)calibData.dig_T2)) >> 11;
            
    var2 = (((((adcTemp >> 4) - ((int32_t)calibData.dig_T1)) * 
              ((adcTemp >> 4) - ((int32_t)calibData.dig_T1))) >> 12) * 
            ((int32_t)calibData.dig_T3)) >> 14;
            
    t_fine = var1 + var2;
    temperature = (t_fine * 5 + 128) >> 8;
    
    return temperature;
}

// Pressure compensation formula from BME280 datasheet
uint32_t BME280_Driver::compensatePressure(int32_t adcPres) {
    int64_t var1, var2, p;
    
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calibData.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calibData.dig_P5) << 17);
    var2 = var2 + (((int64_t)calibData.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calibData.dig_P3) >> 8) + 
           ((var1 * (int64_t)calibData.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calibData.dig_P1) >> 33;
    
    if (var1 == 0) {
        return 0; // Avoid division by zero
    }
    
    p = 1048576 - adcPres;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calibData.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calibData.dig_P8) * p) >> 19;
    
    p = ((p + var1 + var2) >> 8) + (((int64_t)calibData.dig_P7) << 4);
    return (uint32_t)p;
}

// Humidity compensation formula from BME280 datasheet
uint32_t BME280_Driver::compensateHumidity(int32_t adcHum) {
    int32_t v_x1_u32r;
    
    v_x1_u32r = (t_fine - ((int32_t)76800));
    
    v_x1_u32r = (((((adcHum << 14) - (((int32_t)calibData.dig_H4) << 20) -
                   (((int32_t)calibData.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                (((((((v_x1_u32r * ((int32_t)calibData.dig_H6)) >> 10) *
                   (((v_x1_u32r * ((int32_t)calibData.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                   ((int32_t)2097152)) * ((int32_t)calibData.dig_H2) + 8192) >> 14));
    
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                             ((int32_t)calibData.dig_H1)) >> 4));
    
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    
    return (uint32_t)(v_x1_u32r >> 12);
}
