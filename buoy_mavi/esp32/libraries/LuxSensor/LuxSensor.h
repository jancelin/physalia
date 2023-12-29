#pragma once
#include "Arduino.h"
#include "Wire.h"

#define I2C_ADRESS 0x23                 //I2C Address 0x23

class LuxSensor {
public:
	LuxSensor();
	~LuxSensor();

public:
	void setup();
	float getValue();
    void setPin(int pin);

private:
    float Lux;
    uint8_t buf[4] = {0};
    uint16_t data, data1;
    uint8_t readReg(uint8_t reg, const void* pBuf, size_t size);
    
    
};  