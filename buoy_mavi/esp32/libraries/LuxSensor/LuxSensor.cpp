#include "LuxSensor.h"

LuxSensor::LuxSensor(){
    this->setup();
}

LuxSensor::~LuxSensor(){

}

void LuxSensor::setup()
{
  Serial.println("SensorLux Class - Setup");
  //Wire.begin(I2C_ADRESS);
}

float LuxSensor::getValue()
{
  //readReg(0x10, this->buf, 2);              //Register I2C_ADRESS 0x10
  readReg(I2C_ADRESS, this->buf, 2);
  this->data = buf[0] << 8 | buf[1];
  this->Lux = (((float)this->data )/1.2);
  Serial.print("LUX:");
  Serial.print(this->Lux);
  Serial.print("lx");
  Serial.print("\n");
  delay(500);
  return this->Lux;
}

uint8_t LuxSensor::readReg(uint8_t reg, const void* pBuf, size_t size_uint8)
{
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t * _pBuf = (uint8_t *)pBuf;
  Wire.beginTransmission(I2C_ADRESS);
  Wire.write(&reg, 1);
  if ( Wire.endTransmission() != 0) {
    return 0;
  }
  delay(20);
  Wire.requestFrom(I2C_ADRESS, (uint8_t) size_uint8);
  for (uint16_t i = 0; i < size_uint8; i++) {
    _pBuf[i] = Wire.read();
  }
  return size_uint8;
}