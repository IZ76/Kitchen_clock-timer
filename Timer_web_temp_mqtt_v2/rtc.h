int BCD2DEC(int x) { return ((x)>>4)*10+((x)&0xf); }
int DEC2BCD(int x) { return (((x)/10)<<4)+((x)%10); }
byte hour=21, minute=55, second=1, month=8, day=28, lastDay=28, dayOfWeek=4;
int year=2018;
//--------------------------------------
void  setRTCDateTime() {
  Wire.beginTransmission(0x68); //DS_I2C_ADDRESS
  Wire.write(0x00); // DS_TIME
  Wire.write(DEC2BCD(second));
  Wire.write(DEC2BCD(minute));
  Wire.write(DEC2BCD(hour));
  Wire.write(DEC2BCD(dayOfWeek));
  Wire.write(DEC2BCD(day));
  Wire.write(DEC2BCD(month));
  Wire.write(DEC2BCD(year-2000));
  Wire.endTransmission();
}
//--------------------------------------
void getRTCDateTime(void) {
  int v;
  Wire.beginTransmission(0x68); //DS_I2C_ADDRESS
  Wire.write(0x00); // DS_TIME
  Wire.endTransmission();
  Wire.requestFrom(0x68, 7);
  if(Wire.available()) {
    v = Wire.read() & 0x7f;
    second = BCD2DEC(v);
    v = Wire.read() & 0x7f;
    minute = BCD2DEC(v);
    v = Wire.read() & 0x3f;
    hour = BCD2DEC(v);
    v = Wire.read() & 0x07;
    dayOfWeek = BCD2DEC(v);
    v = Wire.read() & 0x3f;
    day = BCD2DEC(v);
    v = Wire.read() & 0x3f;
    month = BCD2DEC(v);
    v = Wire.read() & 0xff;
    year = BCD2DEC(v) + 2000; 
    Wire.endTransmission();
  }
}
