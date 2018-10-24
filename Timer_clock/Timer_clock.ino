#include <Wire.h>
#include <ESP8266WiFi.h>
#include "rtc.h"
#include "TM1637.h"
#define TM1637_CLK       13 
#define TM1637_DIO       2
#define BUTTON_1         14    // Пин клавиши 1
#define BUTTON_2         12    // Пин клавиши 2
#define BUTTON_3         15    // Пин клавиши 3
#define BUZZER           16    // Пин базера
TM1637 tm1637(TM1637_CLK,TM1637_DIO);
const char* ssid = "IvanUA";
const char* password = "";
byte lastSec;
int secFr=0;
bool flash=0;
byte timeZone = 2;
bool sommerZeit = 1;
byte key=0;
int keySpeed;
int timmer=0;
int lastSecTim=0;
byte h=0, m=0, s=0;
bool pause=1;
//______________________________________
void setup() {
  Wire.begin();
  Serial.begin(115200);
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  pinMode(BUZZER, OUTPUT);
  tm1637.init();
  tm1637.set(7);
  getRTCDateTime();
  displayTime();
  wifiConnect();
  updateTime();
}//______________________________________
//=======================================
void loop() {
  if(second!=lastSec) {
    lastSec=second;
    secFr=0;
  } else secFr++;
  getRTCDateTime();
  if(millis()%1000<500 && flash || key!=0) {
    flash=0;
    if(timmer==0) displayTime();
    else displayTimer();
  }
  else if(millis()%1000>500 && !flash || key!=0) {
    flash=1;
    if(timmer==0) displayTime();
    else tm1637.clearDisplay();
  }
  if(hour==0 && minute==5 && second==0) updateTime(); // Обновляем время один раз в сутки
  if(minute==0 && second==0 && secFr==0) {      // сигнал каждый час
    bip();
    bip();
  }
  menu();
}//======================================
void displayTime(){
  tm1637.point(flash);
  tm1637.display(0,hour/10);
  tm1637.display(1,hour%10);
  tm1637.display(2,minute/10);
  tm1637.display(3,minute%10);
}//-------------------------------------
void displayTimer(){
  h=(int)timmer/3600;
  m=(int)timmer/60-h*60;
  s=timmer-m*60;
  tm1637.point(1);
  if(timmer>3600) {
    if(h>9) tm1637.display(0, h/10);
    tm1637.display(1, h%10);
    tm1637.display(2, m/10);
    tm1637.display(3, m%10);
  } else {
    if(m>9) tm1637.display(0, m/10);
    if(m>0) tm1637.display(1, m%10);
    if(s>9 || m>0) tm1637.display(2, s/10);
    tm1637.display(3, s%10);
  }
}//-------------------------------------
void updateTime() {
  WiFiClient client;
  if (!client.connect("google.com", 80)) {
    Serial.println("Connection google.com - failed!!!");
    return;
  }
  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while(!client.available() && repeatCounter < 10) {
    delay(1000);
    Serial.println(".");
    repeatCounter++;
  }
  String line;
  int size = 0;
  client.setNoDelay(false);
  while(client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    // example:
    // date: Thu, 19 Nov 2015 20:25:40 GMT
    if (line.startsWith("DATE: ")) {
      hour = line.substring(23, 25).toInt();
      hour = hour + (timeZone + (1 * sommerZeit));
      if(hour>23) hour = hour-24;
      minute = line.substring(26, 28).toInt();
      second = line.substring(29, 31).toInt();
      year = line.substring(18, 22).toInt();
      day = line.substring(11, 13).toInt();
      String dw = line.substring(6, 9);
      String mont = line.substring(14, 17);
      if(mont=="JAN") month = 1;
      if(mont=="FEB") month = 2;
      if(mont=="MAR") month = 3;
      if(mont=="APR") month = 4;
      if(mont=="MAY") month = 5;
      if(mont=="JUN") month = 6;
      if(mont=="JUL") month = 7;
      if(mont=="AUG") month = 8;
      if(mont=="SEP") month = 9;
      if(mont=="OCT") month = 10;
      if(mont=="NOV") month = 11;
      if(mont=="DEC") month = 12;
      if(dw=="SUN") dayOfWeek = 1;
      if(dw=="MON") dayOfWeek = 2;
      if(dw=="TUE") dayOfWeek = 3;
      if(dw=="WED") dayOfWeek = 4;
      if(dw=="THU") dayOfWeek = 5;
      if(dw=="FRI") dayOfWeek = 6;
      if(dw=="SAT") dayOfWeek = 7;
      Serial.println(String(day)+"."+String(month)+"."+String(year)+" "+String(dayOfWeek)+" "+String(hour)+":"+String(minute)+":"+String(second));
      setRTCDateTime();
    }
  }
}//---------------------------------------
void wifiConnect() {
  Serial.println();
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}//---------------------------------------
void klav() {
  key=0;
  if(digitalRead(BUTTON_1)==HIGH) key=1;
  if(digitalRead(BUTTON_2)==HIGH) key=2;
  //if(digitalRead(BUTTON_3)==HIGH) key=3;
  if(key!=0 && (keySpeed==0 || (h==1 && m==0) || (h==0 && m==59) || (m==9 && s==00))) tm1637.clearDisplay();
  if(key!=0) {
    keySpeed++;
  } else keySpeed=0;
  if(key==1) {
    timmer=(int(timmer/60)+1)*60;
    if(timmer>10800) timmer=10800;
    displayTimer();
  }
  if(key==2) {
    timmer=(int(timmer/60)-(timmer%60==0?1:0))*60;
    if(timmer<0) timmer=0;
    if(timmer!=0) displayTimer();
  }
  if(key!=0 && timmer!=0 && timmer!=10800) bip();
}//---------------------------------------
void bip(){
  digitalWrite(BUZZER, HIGH);
  delay(keySpeed<8?120:keySpeed<40?60:20);
  digitalWrite(BUZZER, LOW);
  delay(keySpeed<8?120:keySpeed<40?60:20);
}//---------------------------------------
void menu(){
  klav();
  if(timmer!=0 && key==0 && pause==0) {
    if(lastSecTim!=second) {
      lastSecTim=second;
      timmer--;
      if(!timmer) alarm();
    }
  }
  if(timmer!=0 && pause && second%10==0 && key==0) bip();
  if(digitalRead(BUTTON_3)==HIGH) {
    pause=!pause;
    bip();
    delay(500);
  }
}//---------------------------------------
void alarm() {
  for(int i=0; i<120; i++){
    tm1637.clearDisplay();
    tm1637.point(1);
    tm1637.display(0, 0);
    tm1637.display(1, 0);
    tm1637.display(2, 0);
    tm1637.display(3, 0);
    bip();
    bip();
    bip();
    tm1637.clearDisplay();
    delay(280);
    if(digitalRead(BUTTON_3)==HIGH) {
      pause=1;
      bip();
      delay(500);
      return;
    }
  }
  pause=1;
}

