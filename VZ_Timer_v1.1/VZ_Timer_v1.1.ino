#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#include <ESP8266HTTPUpdateServer.h>
ESP8266HTTPUpdateServer httpUpdater;
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "Adafruit_Si7021.h"
#include "rtc.h"
#include <Ticker.h>
#include "TM1637.h"
#define TM1637_CLK       13 
#define TM1637_DIO       2
#define BUTTON_1         14    // Пин клавиши 1
#define BUTTON_2         12    // Пин клавиши 2
#define BUTTON_3         15    // Пин клавиши 3
#define BUZZER           16    // Пин базера
#include <ESP8266HTTPClient.h>
//#include "page_wifi.h"
TM1637 tm1637(TM1637_CLK,TM1637_DIO);
Ticker blinker;
WiFiClient ESPclient;
PubSubClient MQTTclient(ESPclient);
String versionTimer = "1.1";
String ssid       = "IvanUA";
String password   = "1234567890";
String ssidAP     = "Timer";
String passwordAP = "11223344";
IPAddress apIp(192, 168, 4, 1);
String userApp = "admin";
String passApp = "admin";
// ----------змінні для роботи з mqtt сервером
bool useMqtt = false;
char mqtt_server[21] = "m13.cloudmqtt.com";
int  mqtt_port = 13011;
char mqtt_user[25] = "aaaaaaaa";
char mqtt_pass[25] = "bbbbbbbbbbbb";
char mqtt_name[25] = "Timer";
char mqtt_pub_temp[25] = "Timer/temp";
char mqtt_pub_humi[25] = "Timer/humi";
char mqtt_pub_info[25] = "Inform/mess";

byte timeZone = 2;
bool sommerZeit = 1;
byte kuSet = 2; //0 - отключена, 1 - всегда, 2 - по времени
byte kuStart = 7;
byte kuStop = 23;
float corrTemp = 0.0;
float corrHumi = 0.0;

int timmer = 0;
bool pause = 1;

byte lastSecond;
int secFr = 0;
bool flash = 0;
byte key = 0;
int keySpeed;
int lastSecTim = 0;
byte h = 0, m = 0, s = 0;
bool statusWifi = 0;
Adafruit_Si7021 sensorSi = Adafruit_Si7021();
float tempSensors = 85;
float humiSensors = 0;
bool sensors = false;
String jsonConfig = "{}";
bool stopAlarm = false;

//______________________________________
void setup() {
  Wire.begin();
  Serial.begin(115200);
  SPIFFS.begin();
  loadConfig();
//saveConfig();
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  pinMode(BUZZER, OUTPUT);
  tm1637.init();
  tm1637.set(7);
  updateTime();
  blinker.attach(1, updateLocalTime);
  displayTime();
  controlInitialization();
  wifiConnect();
  updateTime();
  ArduinoOTA.setHostname("Timer_Kuche");
  ArduinoOTA.begin();
  // ------------------
  if (sensorSi.begin()) {
    Serial.println("YES!!! find Si7021 sensor!");
    sensors = true;
    sensorTemHumi();;
  } else {
    Serial.println("Did not find Si7021 sensor!");
  }
  // ---------- MQTT client
  MQTTclient.setServer(mqtt_server, mqtt_port);
  MQTTclient.connect(mqtt_name, mqtt_user, mqtt_pass);
}
//______________________________________
void reconnect() {
  if(!ESPclient.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("MQTT reconnection...");
    if(MQTTclient.connect(mqtt_name, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc = ");
      Serial.println(MQTTclient.state());
    }
  }
}
//=======================================
void loop() {
  ArduinoOTA.handle();
  server.handleClient();  
  if(second != lastSecond) {
    lastSecond = second;
    secFr = 0;
  } else secFr++;
  if(secFr==0 && second==0 && minute==0) updateTime(); // обновляем время в начале каждого часа
  if(millis() % (statusWifi ? 1000 : 500) < (statusWifi ? 500 : 400) && flash || key != 0) {
    flash = 0;
    if(timmer == 0) displayTime();
    else displayTimer();
  }
  else if(millis() % (statusWifi ? 1000 : 500) > (statusWifi ? 500 : 400) && !flash || key != 0) {
    flash = 1;
    if(timmer == 0) displayTime();
    else tm1637.clearDisplay();
  }
  if(hour == 0 && minute== 5 && second == 0) updateTime(); // Обновляем время один раз в сутки в 00:05:00

  if((kuStart<=kuStop?(kuStart<=hour&&kuStop>hour):(kuStart<=hour||kuStop>hour))&&kuSet==2 || kuSet==1){ // сигнал каждый час
    if(!secFr && !minute && !second) {
      bip();
      bip();
    }
  }
  
  menu();
  if(!secFr && second == 5 && minute % 10 == 1 && WiFi.status() != WL_CONNECTED) wifiConnect();
  if(!secFr && second == 30) sensorTemHumi(); // каждую 30-ю секунду делаем опрос сенсора

  //---------------------------Подключение к МQTT------------------------------------------------------
  if(useMqtt && !secFr && second == 40 && (minute%2) == 0) {
    if(!MQTTclient.connected()) {
      reconnect();
    }
    if(MQTTclient.connected()) {
      MQTTclient.publish(mqtt_pub_temp, (String(tempSensors)).c_str());
      MQTTclient.publish(mqtt_pub_humi, (String(humiSensors)).c_str());
      Serial.print("Publish in topic ");
      Serial.print("Temperature: " + String(tempSensors) + "*C,   ");
      Serial.println("Humidity: " + String(humiSensors) + " %,  ");
    }
  }
  if(secFr==0 && second == 0) Serial.println(String(hour) + ":" + String(minute) + ":" + String(second));
}//======================================
void displayTime(){
  tm1637.point(flash);
  tm1637.display(0, hour / 10);
  tm1637.display(1, hour % 10);
  tm1637.display(2, minute / 10);
  tm1637.display(3, minute % 10);
}
//-------------------------------------
void displayTimer(){
  h = (int)timmer / 3600;
  m = (int)timmer / 60 - h * 60;
  s = timmer - m * 60;
  tm1637.point(1);
  if(timmer > 3600) {
    if(h > 9) tm1637.display(0, h / 10);
    tm1637.display(1, h % 10);
    tm1637.display(2, m / 10);
    tm1637.display(3, m % 10);
  } else {
    if(m > 9) tm1637.display(0, m / 10);
    if(m > 0) tm1637.display(1, m % 10);
    if(s > 9 || m > 0) tm1637.display(2, s / 10);
    tm1637.display(3, s % 10);
  }
}//-------------------------------------
void updateTime() {
  printTime();
  Serial.println("Start Update TIME!");
  if(WiFi.status() != WL_CONNECTED) {
    getRTCDateTime();
    printTime();
    Serial.println("Not internet connected. Time update - RTC");
    return;
  }
  WiFiClient google_client;
  if (!google_client.connect("google.com", 80)) {
    Serial.println("Connection google.com - failed!!!");
    return;
  }
  
  google_client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while(!google_client.available() && repeatCounter < 10) {
    delay(1000);
    Serial.print("proba : ");
    Serial.println(repeatCounter);
    repeatCounter++;
  }
  //Serial.print ("google_client.available() - ");
  //Serial.println (google_client.available());
  String line;
  int size = 0;
  google_client.setNoDelay(false);
  while(google_client.available()) {
    line = google_client.readStringUntil('\n');
    line.toUpperCase();
    //Serial.print("line = ");
    //Serial.println(line);
    // example:
    // date: Thu, 19 Nov 2015 20:25:40 GMT
    if (line.startsWith("DATE: ")) {
      hour = line.substring(23, 25).toInt();
      hour = hour + (timeZone + (1 * sommerZeit));
      if(hour>23) hour = hour - 24;
      minute = line.substring(26, 28).toInt();
      second = line.substring(29, 31).toInt();
      year = line.substring(18, 22).toInt();
      day = line.substring(11, 13).toInt();
      String dw = line.substring(6, 9);
      String mont = line.substring(14, 17);
      if(mont == "JAN") month = 1;
      if(mont == "FEB") month = 2;
      if(mont == "MAR") month = 3;
      if(mont == "APR") month = 4;
      if(mont == "MAY") month = 5;
      if(mont == "JUN") month = 6;
      if(mont == "JUL") month = 7;
      if(mont == "AUG") month = 8;
      if(mont == "SEP") month = 9;
      if(mont == "OCT") month = 10;
      if(mont == "NOV") month = 11;
      if(mont == "DEC") month = 12;
      if(dw == "SUN") dayOfWeek = 1;
      if(dw == "MON") dayOfWeek = 2;
      if(dw == "TUE") dayOfWeek = 3;
      if(dw == "WED") dayOfWeek = 4;
      if(dw == "THU") dayOfWeek = 5;
      if(dw == "FRI") dayOfWeek = 6;
      if(dw == "SAT") dayOfWeek = 7;
      Serial.print("Load google time: ");
      Serial.println(String(hour) + ":" + String(minute) + ":" + String(second) + "     " + String(day) + "-" + String(month) + "-" + String(year) + " (" + String(dayOfWeek) + ")");
      setRTCDateTime();
    }
  }
}

//=========================================================================================================
void wifiConnect(){
  statusWifi = 0;
  printTime();
  Serial.print("Connecting WiFi (ssid="+String(ssid.c_str())+"  pass="+String(password.c_str())+") ");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  delay (1000);
  for(int i=0; i<20; i++){
    if(WiFi.status()==WL_CONNECTED){
      statusWifi = 1;
      Serial.print(" IP adress : ");
      Serial.println(WiFi.localIP());
      ledSetIp(WiFi.localIP().toString());
      return;
    }
  Serial.print(".");
    delay (500);
  }
  Serial.print("No connected!!!");

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssidAP.c_str(), passwordAP.c_str());
  printTime();
  Serial.println("Start AP mode!!!");
  Serial.print("          Wifi AP IP : " + WiFi.softAPIP());
  ledSetIp(WiFi.softAPIP().toString());
}

//---------------------------------------
void klav() {
  key = 0;
  if(digitalRead(BUTTON_1) == HIGH) key = 1;
  if(digitalRead(BUTTON_2) == HIGH) key = 2;
  //if(digitalRead(BUTTON_3) == HIGH) key = 3;
  if(key != 0 && (keySpeed == 0 || (h == 1 && m == 0) || (h == 0 && m == 59) || (m == 9 && s == 00))) tm1637.clearDisplay();
  if(key != 0) {
    keySpeed++; // если долго нажата любая кнопка то нарасчиваем флаг скорости сигнала
  } else keySpeed = 0;
  if(key == 1) {
    timmer = (int(timmer / 60) + 1) * 60;
    if(timmer > 10800) timmer = 10800;
    displayTimer();
  }
  if(key == 2) {
    timmer = (int(timmer / 60) - (timmer % 60 == 0 ? 1 : 0)) * 60;
    if(timmer < 0) timmer = 0;
    if(timmer != 0) displayTimer();
  }
  if(key != 0 && timmer != 0 && timmer != 10800) bip();
}//---------------------------------------
void bip(){
  digitalWrite(BUZZER, HIGH);
  delay(keySpeed < 8 ? 120 : keySpeed < 40 ? 60 : 20);
  digitalWrite(BUZZER, LOW);
  delay(keySpeed < 8 ? 120 : keySpeed < 40 ? 60 : 20);
}//---------------------------------------
void menu(){
  klav();
  if(timmer != 0 && key == 0 && pause == 0) {
    if(lastSecTim != second) {
      lastSecTim = second;
      timmer--;
      if(!timmer) alarm();
    }
  }
  if(timmer != 0 && pause && second % 10 == 0 && key == 0) bip();
  if(digitalRead(BUTTON_3) == HIGH) {
    pause = !pause;
    bip();
    delay(500);
  }
}//---------------------------------------
void alarm() {
  for(int i = 0; i < 120; i++){
    server.handleClient();  
    if(digitalRead(BUTTON_3) == HIGH || stopAlarm) {
      stopAlarm = false;
      pause = 1;
      bip();
      delay(500);
      return;
    }
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
    if(i%12==0){
      if(useMqtt){
        if(!MQTTclient.connected()) {
          reconnect();
        }
        if(MQTTclient.connected()) {
          MQTTclient.publish(mqtt_pub_info, "Сработал таймер!!!");
        }
      }
    }
  }
  pause = 1;
}

//--------------------------------------
void sensorTemHumi() {
  if(sensors == false) return;
  tempSensors = sensorSi.readTemperature() + corrTemp;
  humiSensors = sensorSi.readHumidity() + corrHumi;
  Serial.println("Temperature Si7021: " + String(tempSensors) + " *C,  Humidity: " + String(humiSensors));
}
//----------------------------------------
void updateLocalTime() {
  second++;
  if(second>59) {
    second = 0;
    minute++;
    if(minute>59) {
      minute = 0;
      hour++;
      if(hour>23) {
        hour = 0;
        day++;
        if(day==32 || (day==31 && (month==4 || month==6 || month==9 || month==11)) || (month==2 && ((day==29 && year%4!=0) || (day==30 && year%4==0)))) {
          day=1;
          month++;
          if(month>12){
            month=1;
            year++;
          }
        }
      }
    }
  } 
  //Serial.println(String(hour) + ":" + String(minute) + ":" + String(second));
}

// чтение конфигурации из памяти устройства
//========================================================================================
bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if(!configFile) {
    Serial.println("Failed to open config file");
    saveConfig();
    configFile.close();
    return false;
  }
  size_t size = configFile.size();
  if(size > 1024) {
    Serial.println("Config file size is too large");
    configFile.close();
    return false;
  }
  jsonConfig = configFile.readString();
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonConfig);
  configFile.close();
  ssid = doc["ssid"].as<String>();
  password = doc["password"].as<String>();
  ssidAP = doc["ssidAP"].as<String>();
  passwordAP = doc["passwordAP"].as<String>();
  userApp = doc["userApp"].as<String>();
  passApp = doc["passApp"].as<String>();
  useMqtt = doc["useMqtt"];
  snprintf(mqtt_server, 24, "%s", (doc["mqtt_server"].as<String>()).c_str());
  mqtt_port = doc["mqtt_port"];
  snprintf(mqtt_user, 24, "%s", (doc["mqtt_user"].as<String>()).c_str());
  snprintf(mqtt_pass, 24, "%s", (doc["mqtt_pass"].as<String>()).c_str());
  snprintf(mqtt_name, 24, "%s", (doc["mqtt_name"].as<String>()).c_str());
  snprintf(mqtt_pub_temp, 24, "%s", (doc["mqtt_pub_temp"].as<String>()).c_str());
  snprintf(mqtt_pub_humi, 24, "%s", (doc["mqtt_pub_humi"].as<String>()).c_str());
  snprintf(mqtt_pub_info, 24, "%s", (doc["mqtt_pub_info"].as<String>()).c_str());
  timeZone = doc["timeZone"];
  sommerZeit = doc["sommerZeit"];
  kuSet = doc["kuSet"];
  kuStart = doc["kuStart"];
  kuStop = doc["kuStop"];
  corrTemp = doc["corrTemp"];
  corrHumi  = doc["corrHumi"];
  printTime();
  Serial.print("Load Config : ");
  Serial.println(jsonConfig);
  return true;
}

// запись конфигурации в память устройства
//========================================================================================
bool saveConfig() {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonConfig);
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["ssidAP"] = ssidAP;
  doc["passwordAP"] = passwordAP;
  doc["userApp"] = userApp;
  doc["passApp"] = passApp;
  doc["useMqtt"] = useMqtt;
  doc["mqtt_server"] = mqtt_server;
  doc["mqtt_port"] = mqtt_port;
  doc["mqtt_user"] = mqtt_user;
  doc["mqtt_pass"] = mqtt_pass;
  doc["mqtt_name"] = mqtt_name;
  doc["mqtt_pub_temp"] = mqtt_pub_temp;
  doc["mqtt_pub_humi"] = mqtt_pub_humi;
  doc["mqtt_pub_info"] = mqtt_pub_info;
  doc["timeZone"] = timeZone;
  doc["sommerZeit"] = sommerZeit;
  doc["kuSet"] = kuSet;
  doc["kuStart"] = kuStart;
  doc["kuStop"] = kuStop;
  doc["corrTemp"] = corrTemp;
  doc["corrHumi"] = corrHumi;
  jsonConfig = "";
  if(serializeJson(doc, jsonConfig)==0){
    Serial.println(F("Failed to write to jsonConfig"));
  }  
  File configFile = SPIFFS.open("/config.json", "w");
  if(!configFile) {
    configFile.close();
    return false;
  }
  if(serializeJson(doc, configFile)==0){
    Serial.println(F("Failed to write to file"));
  } 
  printTime();
  Serial.print("Save Config : ");
  Serial.println(jsonConfig);
  configFile.close();
  bip();
  return true;
}

void ledSetIp(String res){
  char charArray[res.length()+1];
  strcpy(charArray, res.c_str());
  int startA = 1;
  tm1637.clearDisplay();
  for(int i=0; i<res.length(); i++){
    int a = (int)charArray[i];
    if(a==46 || startA>4 || (a<48 || a>57)){
      startA=0;
      delay(1000);
      tm1637.clearDisplay();
    }else tm1637.display(startA - 1, a-48);
    startA++;
  }
  delay(1000);
}
//--------------------------------------------------------------------------
void printTime() {
  Serial.print((hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute) + ":" + (second < 10 ? "0" : "") + String(second) + "  ");
  Serial.print(String(day) + "-" +  String(month) + "-" +  String(year) + " (" + String(dayOfWeek) + ")   ");
}
