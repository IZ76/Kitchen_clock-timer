int BCD2DEC(int x) { return ((x)>>4)*10+((x)&0xf); }
int DEC2BCD(int x) { return (((x)/10)<<4)+((x)%10); }
byte hour=21, minute=55, month=8, day=28, lastDay=28, dayOfWeek=4;
int second=1;
int year=2018;
int corr=0;

#define I2CStart(x)   Wire.beginTransmission(x)
#define I2CStop()     Wire.endTransmission()
#define I2CWrite(x)   Wire.write(x)
#define I2CRead()     Wire.read()
#define I2CReq(x,y)   Wire.requestFrom(x,y)
#define I2CReady      while(!Wire.available()) {};

#define DS1307_I2C_ADDRESS 0x68
#define DS1307_I2C_ADDRESS_EEPROM 0x57
#define DS1307_TIME    0x00
#define DS1307_DOW     0x03
#define DS1307_DATE    0x04
#define DS1307_MEM     0x08

//--------------------------------------
void  setRTCDateTime() {
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_TIME);
  I2CWrite(DEC2BCD(second));
  I2CWrite(DEC2BCD(minute));
  I2CWrite(DEC2BCD(hour));
  I2CWrite(DEC2BCD(dayOfWeek));
  I2CWrite(DEC2BCD(day));
  I2CWrite(DEC2BCD(month));
  I2CWrite(DEC2BCD(year-2000));
  I2CStop();
}
//--------------------------------------
void setRTCTime() {
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_TIME);
  I2CWrite(DEC2BCD(second));
  I2CWrite(DEC2BCD(minute));
  I2CWrite(DEC2BCD(hour));
  I2CStop();
}
//--------------------------------------
void setRTCTimeNotSec() {
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_TIME+0x01);
  I2CWrite(DEC2BCD(minute));
  I2CWrite(DEC2BCD(hour));
  I2CStop();
}
//--------------------------------------
void setRTCDate() {
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_DATE);
  I2CWrite(DEC2BCD(day));
  I2CWrite(DEC2BCD(month));
  I2CWrite(DEC2BCD(year-2000));
  I2CStop();
}
//--------------------------------------
void setRTCDoW() {
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_DOW);
  I2CWrite(DEC2BCD(dayOfWeek));
  I2CStop();
}
//--------------------------------------
void getRTCDateTime(void) {
  int v;
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_TIME);
  I2CStop();
  I2CReq(DS1307_I2C_ADDRESS, 7);
  I2CReady;
  v=I2CRead()&0x7f;
  second=BCD2DEC(v);
  v=I2CRead()&0x7f;
  minute=BCD2DEC(v);
  v=I2CRead()&0x3f;
  hour=BCD2DEC(v);
  v=I2CRead()&0x07;
  dayOfWeek=BCD2DEC(v);
  v=I2CRead()&0x3f;
  day=BCD2DEC(v);
  v=I2CRead()&0x3f;
  month=BCD2DEC(v);
  v=I2CRead()&0xff;
  year=BCD2DEC(v)+2000;
  I2CStop();
}
//--------------------------------------
void writeRTCMem(unsigned char addr, byte val) {
  if(addr<6) {
    I2CStart(DS1307_I2C_ADDRESS);
    I2CWrite(DS1307_MEM+addr);
    I2CWrite(val);
    I2CStop();
  } else {
    Wire.beginTransmission(DS1307_I2C_ADDRESS_EEPROM);
    Wire.write((int)((addr-6) >> 8));
    Wire.write((int)((addr-6) & 0xFF));
    Wire.write(val);
    Wire.endTransmission();
  }
}
//--------------------------------------
byte readRTCMem(byte addr) {
  if(addr<6){
    I2CStart(DS1307_I2C_ADDRESS);
    I2CWrite(DS1307_MEM+addr);
    I2CStop();
    I2CReq(DS1307_I2C_ADDRESS, 1);
    I2CReady;
    unsigned char v=I2CRead();
    I2CStop();
    return v;
  } else {
    byte v=0xFF;
    Wire.beginTransmission(DS1307_I2C_ADDRESS_EEPROM);
    Wire.write((int)((addr-6) >> 8));
    Wire.write((int)((addr-6) & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom(DS1307_I2C_ADDRESS_EEPROM, 3);
    if (Wire.available()) v=Wire.read();
    return v; 
  }
}
