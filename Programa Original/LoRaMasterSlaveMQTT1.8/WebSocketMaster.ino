#ifdef MASTER

const char* APssid     = "PIA-Gateway";
const char* APpassword = "PIA-admin";

//Global
WebSocketsServer webSocket = WebSocketsServer(80);

//Json Space
const size_t SlavesIDCap = JSON_OBJECT_SIZE(2) + 390;
DynamicJsonBuffer GlobalJsonBuffer(SlavesIDCap);
/*const size_t Par_SlavesIDCap = JSON_OBJECT_SIZE(2) + 390;
DynamicJsonBuffer Par_SlavesIDjsonBuffer(Par_SlavesIDCap);
const size_t Ser_SlavesIDCap = JSON_OBJECT_SIZE(2);
DynamicJsonBuffer Set_SlavesIDjsonBuffer(Ser_SlavesIDCap);*/


//Called when receiving any WebSocket Message
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  char strReply[1000];
  char appCommand[255];

  //Figure out the type of WebSocket event
  switch (type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        /*Serial.printf("[%u] Connection from ", num);
          Serial.println(ip.toString());*/

        // send message to client
        //webSocket.sendTXT(num, "Connected");
      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      strcpy(appCommand, "");
      strcpy(appCommand, (const char *)payload);
      strcpy(strReply, "");

      if (strcasecmp(appCommand, "GetDevID") == 0) {
        strcpy(strReply, uint64ToHexStr(getChipID()).c_str());
        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "GetSlavesID") == 0) {
        for (int i = 0; i < (NMAX_SLAVES); i++) {
          if (!(slaves[i].id == 0)) { //Se Slave nao existe, seu ID = 0
            strcat(strReply, uint64ToHexStr(slaves[i].id).c_str());
            strcat(strReply, " - ");
          }
        }
        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "ResetConfig") == 0) {

        SPIFFS.format();

        writeFile(SPIFFS, "/Wifi.config", "{\"SSID\":\"\",\"Password\":\"\"}");
        writeFile(SPIFFS, "/MQTT.config", "{\"MQTT_Server\":\"saorafael.sytes.net\", \"MQTT_Port\":\"1883\"}");
        writeFile(SPIFFS, "/SlavesID.config", "{\"SlavesID\":\"ac9e3ba4ae30-\", \"nSlaves\":1}");
        writeFile(SPIFFS, "/ClientID.config", "{\"ClientID\":\"10\"}");
        
        strcpy(strReply, "Configuration Files have been Reseted");
        webSocket.sendTXT(num, strReply);        
        break;
      }

      if (strcasecmp(appCommand, "ListFiles") == 0) {

        listDir(SPIFFS, "/", 0);

        readFile(SPIFFS, "/Wifi.config");
        readFile(SPIFFS, "/MQTT.config");
        readFile(SPIFFS, "/SlavesID.config");
        readFile(SPIFFS, "/ClientID.config");
        
        strcpy(strReply, "Configuration Files have been Listed");
        webSocket.sendTXT(num, strReply);        
        break;
      }

      if (strcasecmp(appCommand, "ListSlaves") == 0){
        const char *jsonStr = readFile(SPIFFS, "/SlavesID.config");
        strcpy(strReply, jsonStr);
        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "ListWifiConfig") == 0){
        strcpy(strReply, "");
        const char *jsonStr = readFile(SPIFFS, "/Wifi.config");
        strcpy(strReply, jsonStr);
        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "ListMQTTConfig") == 0){
        strcpy(strReply, "");
        const char *jsonStr = readFile(SPIFFS, "/MQTT.config");
        strcpy(strReply, jsonStr);
        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "ListClientID") == 0){
        const char *jsonStr = readFile(SPIFFS, "/ClientID.config");
        strcpy(strReply, jsonStr);
        webSocket.sendTXT(num, strReply);
        break;
      }


      //FIX ME: IF SOMETHING AFTER ":" IS NULL, CODE CRASHES
      char *token;
      token = strtok(appCommand, ":");
      if (strcasecmp(token, "AddSlave") == 0) {
        char *slaveID;
        slaveID = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/SlavesID.config");
        Serial.print("Data Before Writing Operation: ");
        Serial.println(jsonStr);
        
        JsonObject& rootPar = GlobalJsonBuffer.parseObject(jsonStr);
        const char* SlavesID = rootPar["SlavesID"]; 
        int nSlaves = rootPar["nSlaves"];

        if (nSlaves >= NMAX_SLAVES){
          Serial.println("You`ve reached the maximum number of slaves");
          strcpy(strReply, "Reached Maximum number of slaves");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(SlavesID) + strlen(slaveID) + 35) > sizeof(strReply)){
          Serial.println("Overflow in the Slaves number");
          strcpy(strReply, "Overflow in the Slaves number");
          webSocket.sendTXT(num, strReply);
          break;
        }

        strcpy(strReply, SlavesID);
        strcat(strReply, slaveID);
        strcat(strReply, "-");
        nSlaves++;

        JsonObject& rootSer = GlobalJsonBuffer.createObject();
        rootSer["SlavesID"] = strReply;
        rootSer["nSlaves"] = nSlaves;

        strcpy(strReply, "");
        rootSer.printTo(strReply);
        writeFile(SPIFFS, "/SlavesID.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(token, "DeleteSlave") == 0) {
        char *slaveID;
        slaveID = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/SlavesID.config");
        Serial.print("Data Before Writing Operation: ");
        Serial.println(jsonStr);
        
        JsonObject& rootPar = GlobalJsonBuffer.parseObject(jsonStr);
        const char* SlavesID = rootPar["SlavesID"]; 
        int nSlaves = rootPar["nSlaves"];

        char *idToken;
        idToken = strtok((char*) SlavesID, "-");

        while (idToken != NULL){
          if (strcasecmp(idToken, slaveID) == 0){
            idToken = strtok (NULL, "-");
            nSlaves--;
            continue;
          }
          
          strcat(strReply, idToken);
          strcat(strReply, "-");
          idToken = strtok (NULL, "-");
        }

        JsonObject& rootSer = GlobalJsonBuffer.createObject();
        rootSer["SlavesID"] = strReply;
        rootSer["nSlaves"] = nSlaves;

        strcpy(strReply, "");
        rootSer.printTo(strReply);
        writeFile(SPIFFS, "/SlavesID.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(token, "SetWifiSSID") == 0) {
        char *newSSID;
        newSSID = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/Wifi.config");
        Serial.print("Data Before Writing Operation: ");
        Serial.println(jsonStr);

        JsonObject& rootPar = GlobalJsonBuffer.parseObject(jsonStr);
        const char* wifiSSID = rootPar["SSID"]; 
        const char* Password = rootPar["Password"];

        if (strlen(newSSID) >= sizeof(ssid)){
          Serial.println("Error: Wifi SSID longer than supported by the program");
          strcpy(strReply, "Error: Wifi SSID longer than supported by the program");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(newSSID) + strlen(Password) + 30) > sizeof(strReply)){
          Serial.println("Error: Overflow in the SSID + Password structure");
          strcpy(strReply, "Error: Overflow in the SSID + Password structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        JsonObject& rootSer = GlobalJsonBuffer.createObject();
        rootSer["SSID"] = newSSID;
        rootSer["Password"] = Password;

        rootSer.printTo(strReply);
        writeFile(SPIFFS, "/Wifi.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
        
      }

      if (strcasecmp(token, "SetWifiPassword") == 0) {
        char *newPassword;
        newPassword = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/Wifi.config");
        Serial.print("Data Before Writing Operation: ");
        Serial.println(jsonStr);

        JsonObject& rootPar = GlobalJsonBuffer.parseObject(jsonStr);
        const char* wifiSSID = rootPar["SSID"]; 
        const char* Password = rootPar["Password"];

        if (strlen(newPassword) >= sizeof(password)){
          Serial.println("Error: Wifi password longer than supported by the program");
          strcpy(strReply, "Error: Wifi password longer than supported by the program");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(wifiSSID) + strlen(newPassword) + 30) > sizeof(strReply)){
          Serial.println("Overflow in the SSID + Password structure");
          strcpy(strReply, "Overflow in the SSID + Password structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        JsonObject& rootSer = GlobalJsonBuffer.createObject();
        rootSer["SSID"] = wifiSSID;
        rootSer["Password"] = newPassword;

        rootSer.printTo(strReply);
        writeFile(SPIFFS, "/Wifi.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
        
      }

      if (strcasecmp(token, "SetClientID") == 0) {
        char *newClientID;
        newClientID = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/ClientID.config");
        Serial.print("Data Before Writing Operation: ");
        Serial.println(jsonStr);

        JsonObject& rootPar = GlobalJsonBuffer.parseObject(jsonStr);
        const char* ClientID = rootPar["ClientID"]; 

        if (strlen(newClientID) >= sizeof(clientID)){
          Serial.println("Error: Client ID longer than supported by the program");
          strcpy(strReply, "Error: Client ID longer than supported by the program");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(ClientID) + 20) > sizeof(strReply)){
          Serial.println("Overflow in the CientID structure");
          strcpy(strReply, "Overflow in the CientID structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        JsonObject& rootSer = GlobalJsonBuffer.createObject();
        rootSer["ClientID"] = newClientID;

        rootSer.printTo(strReply);
        writeFile(SPIFFS, "/ClientID.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
        
      }

      if (strcasecmp(token, "SetMQTTServer") == 0) {
        char *newMQTTServer;
        newMQTTServer = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/MQTT.config");
        Serial.print("Data Before Writing Operation: ");
        Serial.println(jsonStr);

        JsonObject& rootPar = GlobalJsonBuffer.parseObject(jsonStr);
        const char* MQTT_Server = rootPar["MQTT_Server"]; 
        const char* MQTT_Port = rootPar["MQTT_Port"];

        if (strlen(newMQTTServer) >= sizeof(mqttServer)){
          Serial.println("Error: MQTT Server longer than supported by the program");
          strcpy(strReply, "Error: MQTT Server longer than supported by the program");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(newMQTTServer) + strlen(MQTT_Port) + 35) > sizeof(strReply)){
          Serial.println("Overflow in the MQTT.Config structure");
          strcpy(strReply, "Overflow in the MQTT.Config structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        JsonObject& rootSer = GlobalJsonBuffer.createObject();
        rootSer["MQTT_Server"] = newMQTTServer;
        rootSer["MQTT_Port"] = MQTT_Port;

        rootSer.printTo(strReply);
        writeFile(SPIFFS, "/MQTT.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
      }

      //FIX ME: THE PORT HAS TO BE A NUMBER
      if (strcasecmp(token, "SetMQTTPort") == 0) {
        char *newMQTTPort;
        newMQTTPort = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/MQTT.config");
        Serial.print("Data Before Writing Operation: ");
        Serial.println(jsonStr);

        JsonObject& rootPar = GlobalJsonBuffer.parseObject(jsonStr);
        const char* MQTT_Server = rootPar["MQTT_Server"]; 
        const char* MQTT_Port = rootPar["MQTT_Port"];

        if((strlen(MQTT_Server) + strlen(newMQTTPort) + 35) > sizeof(strReply)){
          Serial.println("Overflow in the MQTT.Config structure");
          strcpy(strReply, "Overflow in the MQTT.Config structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        JsonObject& rootSer = GlobalJsonBuffer.createObject();
        rootSer["MQTT_Server"] = MQTT_Server;
        rootSer["MQTT_Port"] = newMQTTPort;

        rootSer.printTo(strReply);
        writeFile(SPIFFS, "/MQTT.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "Restart") == 0) {
        strcpy(strReply, "Restarting...");
        webSocket.sendTXT(num, strReply);
        while (!WiFi.softAPdisconnect());
        ESP.restart();
        break;
      }


    
      strcpy(strReply, "Command Not Found");
      webSocket.sendTXT(num, strReply);
      break;


    // send data to all connected clients
    // webSocket.broadcastTXT("message here");

    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;

  }

}

void startAPSocketCom() {

  Serial.print("Setting AP (Access Point)…");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Master");
  display.setCursor(0, 10);
  display.println("Setting Access Point...");
  display.display();


  IPAddress _ip = IPAddress(10, 0, 1, 100);
  IPAddress _gw = IPAddress(10, 0, 1, 1);
  IPAddress _sn = IPAddress(255, 255, 255, 0);

  //.softAP(const char* ssid, const char* password, int channel, int ssid_hidden, int max_connection)
  WiFi.softAP(APssid, APpassword, 1, 0, 1);

  WiFi.softAPConfig(_ip, _gw, _sn);

  IPAddress IP = WiFi.softAPIP();


  Serial.print("AP IP address: ");
  Serial.println(IP);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Master");
  display.setCursor(0, 10);
  display.println("AP IP address: ");
  display.setCursor(0, 20);
  display.println(IP);
  display.display();

  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);


  //Only exit when restarted.
  while (true) {
    // Look for and handle WebSocket data
    webSocket.loop();
    timerWrite(timer, 0); //Alimenta WatchDog
  }

}

void CheckAPInterrupt(){
  int AP_Timer;

  if (!digitalRead(APPin)){
    Serial.println("Adhoc button pressed...");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Adhoc button pressed...");
    display.display();  
  }

  // !!!! Activate Access Point Configuration !!!!
  AP_Timer = millis();
  while (!digitalRead(APPin)){
    if (millis() - AP_Timer > ACCESSPOINT_TIME) {
      timerWrite(timer, 0); //Alimenta WatchDog
      startAPSocketCom();
    }
    timerWrite(timer, 0); //Alimenta WatchDog
  }
  // !!!! \Activate Access Point Configuration !!!!
  
}

void loadConfig(){
  const char *jsonStr;
  Serial.println();
  
// !!!! Load Slaves
  jsonStr = readFile(SPIFFS, "/SlavesID.config");
  
  JsonObject& rootParSlaveID = GlobalJsonBuffer.parseObject(jsonStr);
  const char* SlavesID = rootParSlaveID["SlavesID"]; 
  int nSlaves = rootParSlaveID["nSlaves"];
  
  int i=0;
  char *idToken;
  idToken = strtok((char*) SlavesID, "-");
  while (idToken != NULL){
    slaves[i].id = strtoull(idToken, NULL, 16);
    idToken = strtok (NULL, "-");
    i++;
    //Serial.print(slaves[i].id, HEX);
    //Serial.print("-");
  }

  Serial.println("Slaves Loaded ");

// !!!! Load Wifi Configuration
  jsonStr = readFile(SPIFFS, "/Wifi.config");
  
  JsonObject& rootParWifi = GlobalJsonBuffer.parseObject(jsonStr);
  const char* wifiSSID = rootParWifi["SSID"]; 
  const char* filePassword = rootParWifi["Password"];
  
  strcpy(ssid, wifiSSID);
  strcpy(password, filePassword);

  Serial.print("Wifi Config Loaded: ");
  Serial.print("SSID: ");
  Serial.print(ssid);
  Serial.print("Password: ");
  Serial.println(password);

// !!!! Load MQTT Server Configuration
  jsonStr = readFile(SPIFFS, "/MQTT.config");
  
  JsonObject& rootParMQTT = GlobalJsonBuffer.parseObject(jsonStr);
  const char* MQTT_Server = rootParMQTT["MQTT_Server"]; 
  const char* MQTT_Port = rootParMQTT["MQTT_Port"];

  strcpy(mqttServer, MQTT_Server);
  mqttPort = (uint16_t) strtoul(MQTT_Port, NULL, 0);

  Serial.println("MQTT Server Config Loaded");
  
// !!!! Load Client ID
  jsonStr = readFile(SPIFFS, "/ClientID.config");
  
  JsonObject& rootParClientID = GlobalJsonBuffer.parseObject(jsonStr);
  const char* fileClientID = rootParClientID["ClientID"];
  strcpy(clientID, fileClientID);
  Serial.println("Client ID Loaded");

  Serial.println();
}

#endif
