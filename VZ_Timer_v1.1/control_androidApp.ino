//--------------------------------------------------------------------------
void controlInitialization(void) {
  server.onNotFound([]() {(404, "text/plain", "Access is possible only from the application on Android");});
  server.on("/stat", response_stat);
  server.on("/config", response_config);
  server.on("/set", set_timer);
  server.on("/save", save_config_timer);
  httpUpdater.setup(&server);
  server.begin();
}
//======================================================================================================
void response_stat() {
  String userN = server.arg("user").c_str();
  String passN = server.arg("pass").c_str();
  if(userN == userApp && passN == passApp){
    String json = "{";
    json += "\"ver\":\"";
    json += versionTimer;
  // настройки сети Wifi
    json += "\",\"time\":\"";
    json += (String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute) + ":" + (second < 10 ? "0" : "") + String(second));
    json += "\",\"timmer\":\"";
    json += timmer;
    json += "\",\"pause\":\"";
    json += pause;
    json += "\"}";
    server.send(200, "text/json", json);
  } else server.send(401, "text/json", "Error user login or password...");
}
  
//======================================================================================================
void response_config() {
  String userN = server.arg("user").c_str();
  String passN = server.arg("pass").c_str();
  if(userN == userApp && passN == passApp){
    String json = "{";
    json += "\"ver\":\"";
    json += versionTimer;
    json += "\",\"time\":\"";
    json += (String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute) + ":" + (second < 10 ? "0" : "") + String(second));
    json += "\",\"ssid\":\"";
    json += ssid;
    json += "\",\"password\":\"";
    json += password;
    json += "\",\"ssidAP\":\"";
    json += ssidAP;
    json += "\",\"passwordAP\":\"";
    json += passwordAP;
    json += "\",\"useMqtt\":\"";
    json += useMqtt;
    json += "\",\"mqtt_server\":\"";
    json += mqtt_server;
    json += "\",\"mqtt_port\":\"";
    json += mqtt_port;
    json += "\",\"mqtt_user\":\"";
    json += mqtt_user;
    json += "\",\"mqtt_pass\":\"";
    json += mqtt_pass;
    json += "\",\"mqtt_name\":\"";
    json += mqtt_name;
    json += "\",\"mqtt_pub_temp\":\"";
    json += mqtt_pub_temp;
    json += "\",\"mqtt_pub_humi\":\"";
    json += mqtt_pub_humi;
    json += "\",\"mqtt_pub_info\":\"";
    json += mqtt_pub_info;
    json += "\",\"timeZone\":\"";
    json += timeZone;
    json += "\",\"sommerZeit\":\"";
    json += sommerZeit;
    json += "\",\"kuSet\":\"";
    json += kuSet;
    json += "\",\"kuStart\":\"";
    json += kuStart;
    json += "\",\"kuStop\":\"";
    json += kuStop;
    json += "\",\"corrTemp\":\"";
    json += corrTemp;
    json += "\",\"corrHumi\":\"";
    json += corrHumi;
    json += "\",\"temp\":\"";
    json += tempSensors;
    json += "\",\"humi\":\"";
    json += humiSensors;
    json += "\"}";
    server.send(200, "text/json", json);
   } else server.send(401, "text/json", "Error user login or password..."); 
}
//======================================================================================================
void set_timer() {
  String userN = server.arg("user").c_str();
  String passN = server.arg("pass").c_str();
  if(userN == userApp && passN == passApp){
    stopAlarm = false;
    if(server.arg("timmer")!="") timmer = server.arg("timmer").toInt();
    if(timmer) timmer++;
    if(server.arg("pause")!="") pause = server.arg("pause").toInt();
    if(server.arg("stop")!="") stopAlarm = server.arg("stop").toInt();
    server.send(200, "text/plain", "OK");
  } else server.send(401, "text/json", "Error user login or password...");
}
//======================================================================================================
void save_config_timer() {
  String userN = server.arg("user").c_str();
  String passN = server.arg("pass").c_str();
  if(userN == userApp && passN == passApp){
    stopAlarm = false;
    if(server.arg("ssid")!="") ssid = server.arg("ssid").c_str();
    if(server.arg("password")!="") password = server.arg("password").c_str();
    if(server.arg("ssidAP")!="") ssidAP = server.arg("ssidAP").c_str();
    if(server.arg("passwordAP")!="") passwordAP = server.arg("passwordAP").c_str();
    if(server.arg("newUserApp")!="") userApp = server.arg("newUserApp").c_str();
    if(server.arg("newPassApp")!="") passApp = server.arg("newPassApp").c_str();
    if(server.arg("useMqtt")!="") useMqtt = server.arg("useMqtt").toInt();
    if(server.arg("mqtt_server")!="") snprintf(mqtt_server, 24, "%s", server.arg("mqtt_server").c_str());
    if(server.arg("mqtt_port")!="") mqtt_port = server.arg("mqtt_port").toInt();
    if(server.arg("mqtt_user")!="") snprintf(mqtt_user, 24, "%s", server.arg("mqtt_user").c_str());
    if(server.arg("mqtt_pass")!="") snprintf(mqtt_pass, 24, "%s", server.arg("mqtt_pass").c_str());
    if(server.arg("mqtt_name")!="") snprintf(mqtt_name, 24, "%s", server.arg("mqtt_name").c_str());
    if(server.arg("mqtt_pub_temp")!="") snprintf(mqtt_pub_temp, 24, "%s", server.arg("mqtt_pub_temp").c_str());
    if(server.arg("mqtt_pub_humi")!="") snprintf(mqtt_pub_humi, 24, "%s", server.arg("mqtt_pub_humi").c_str());
    if(server.arg("mqtt_pub_info")!="") snprintf(mqtt_pub_info, 24, "%s", server.arg("mqtt_pub_info").c_str());
    if(server.arg("timeZone")!="") timeZone = server.arg("timeZone").toFloat();
    if(server.arg("sommerZeit")!="") sommerZeit = server.arg("sommerZeit").toInt();
    if(server.arg("kuSet")!="") kuSet = server.arg("kuSet").toInt();
    if(server.arg("kuStart")!="") kuStart = server.arg("kuStart").toInt();
    if(server.arg("kuStop")!="") kuStop = server.arg("kuStop").toInt();
    if(server.arg("corrTemp")!="") corrTemp = server.arg("corrTemp").toFloat();
    if(server.arg("corrHumi")!="") corrHumi = server.arg("corrHumi").toFloat();
    saveConfig();
    updateTime();
    server.send(200, "text/plain", "OK");
  } else server.send(401, "text/json", "Error user login or password...");
}
