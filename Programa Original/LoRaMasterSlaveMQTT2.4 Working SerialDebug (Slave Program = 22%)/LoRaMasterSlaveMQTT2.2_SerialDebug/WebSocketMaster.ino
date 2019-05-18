#ifdef MASTER

const char* APssid     = "PIA-Gateway";
const char* APpassword = "PIA-admin";

//Global
WebSocketsServer webSocket = WebSocketsServer(80);

//Json Space
const size_t SlavesIDCap = 2*JSON_ARRAY_SIZE(10) + 2*JSON_ARRAY_SIZE(16) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 50;
//const size_t SlavesIDCap = 32*JSON_ARRAY_SIZE(10) + 33*JSON_ARRAY_SIZE(16) + JSON_OBJECT_SIZE(1) + 16*JSON_OBJECT_SIZE(5) + 510;


//Called when receiving any WebSocket Message
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  char strReply[1000];
  char appCommand[255];

  //Figure out the type of WebSocket event
  switch (type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      sprintf(Sbuffer, "[%u] Disconnected!\n", num);
      Serial_Println(bDebug, Sbuffer);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        sprintf(Sbuffer, "[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        Serial_Print(bDebug, Sbuffer);
        /*sprintf(Sbuffer, "[%u] Connection from ", num);
         *Serial_Print(bDebug, Sbuffer);
          Serial_Println(bDebug, ip.toString());*/

        // send message to client
        //webSocket.sendTXT(num, "Connected");
      }
      break;

    case WStype_TEXT:
      sprintf(Sbuffer, "[%u] get Text: %s\n", num, payload);
      Serial_Print(bDebug, Sbuffer);

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

        resetConfigFiles();
        
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
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);

        DynamicJsonDocument docPar(SlavesIDCap);
        deserializeJson(docPar, jsonStr);
        
        const char* SlavesID = docPar["SlavesID"]; 
        int nSlaves = docPar["nSlaves"];

        if (nSlaves >= NMAX_SLAVES){
          Serial_Println(bDebug, "You`ve reached the maximum number of slaves");
          strcpy(strReply, "Reached Maximum number of slaves");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(SlavesID) + strlen(slaveID) + 35) > sizeof(strReply)){
          Serial_Println(bDebug, "Overflow in the Slaves number");
          strcpy(strReply, "Overflow in the Slaves number");
          webSocket.sendTXT(num, strReply);
          break;
        }

        strcpy(strReply, SlavesID);
        strcat(strReply, slaveID);
        strcat(strReply, "-");
        nSlaves++;

        DynamicJsonDocument docSer(SlavesIDCap);
        docSer["SlavesID"] = strReply;
        docSer["nSlaves"] = nSlaves;

        strcpy(strReply, "");
        serializeJson(docSer, strReply);
        writeFile(SPIFFS, "/SlavesID.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(token, "DeleteSlave") == 0) {
        char *slaveID;
        slaveID = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/SlavesID.config");
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);
        
        DynamicJsonDocument docPar(SlavesIDCap);
        deserializeJson(docPar, jsonStr);
        const char* SlavesID = docPar["SlavesID"]; 
        int nSlaves = docPar["nSlaves"];

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

        DynamicJsonDocument docSer(SlavesIDCap);
        docSer["SlavesID"] = strReply;
        docSer["nSlaves"] = nSlaves;

        strcpy(strReply, "");
        serializeJson(docSer, strReply);
        writeFile(SPIFFS, "/SlavesID.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(token, "SetWifiSSID") == 0) {
        char *newSSID;
        newSSID = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/Wifi.config");
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);

        DynamicJsonDocument docPar(SlavesIDCap);
        deserializeJson(docPar, jsonStr);
        const char* wifiSSID = docPar["SSID"]; 
        const char* Password = docPar["Password"];

        if (strlen(newSSID) >= sizeof(ssid)){
          Serial_Println(bDebug, "Error: Wifi SSID longer than supported by the program");
          strcpy(strReply, "Error: Wifi SSID longer than supported by the program");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(newSSID) + strlen(Password) + 30) > sizeof(strReply)){
          Serial_Println(bDebug, "Error: Overflow in the SSID + Password structure");
          strcpy(strReply, "Error: Overflow in the SSID + Password structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        DynamicJsonDocument docSer(SlavesIDCap);
        docSer["SSID"] = newSSID;
        docSer["Password"] = Password;

        serializeJson(docSer, strReply);
        writeFile(SPIFFS, "/Wifi.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
        
      }

      if (strcasecmp(token, "SetWifiPassword") == 0) {
        char *newPassword;
        newPassword = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/Wifi.config");
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);

        DynamicJsonDocument docPar(SlavesIDCap);
        deserializeJson(docPar, jsonStr);
        const char* wifiSSID = docPar["SSID"]; 
        const char* Password = docPar["Password"];

        if (strlen(newPassword) >= sizeof(password)){
          Serial_Println(bDebug, "Error: Wifi password longer than supported by the program");
          strcpy(strReply, "Error: Wifi password longer than supported by the program");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(wifiSSID) + strlen(newPassword) + 30) > sizeof(strReply)){
          Serial_Println(bDebug, "Overflow in the SSID + Password structure");
          strcpy(strReply, "Overflow in the SSID + Password structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        DynamicJsonDocument docSer(SlavesIDCap);
        docSer["SSID"] = wifiSSID;
        docSer["Password"] = newPassword;

        serializeJson(docSer, strReply);
        writeFile(SPIFFS, "/Wifi.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
        
      }

      if (strcasecmp(token, "SetClientID") == 0) {
        char *newClientID;
        newClientID = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/ClientID.config");
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);

        DynamicJsonDocument docPar(SlavesIDCap);
        deserializeJson(docPar, jsonStr);
        const char* ClientID = docPar["ClientID"]; 

        if (strlen(newClientID) >= sizeof(clientID)){
          Serial_Println(bDebug, "Error: Client ID longer than supported by the program");
          strcpy(strReply, "Error: Client ID longer than supported by the program");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(ClientID) + 20) > sizeof(strReply)){
          Serial_Println(bDebug, "Overflow in the CientID structure");
          strcpy(strReply, "Overflow in the CientID structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        DynamicJsonDocument docSer(SlavesIDCap);
        docSer["ClientID"] = newClientID;

        serializeJson(docSer, strReply);
        writeFile(SPIFFS, "/ClientID.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
        
      }

      if (strcasecmp(token, "SetMQTTServer") == 0) {
        char *newMQTTServer;
        newMQTTServer = strtok(NULL, ":");
        strcpy(strReply, "");

        const char *jsonStr = readFile(SPIFFS, "/MQTT.config");
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);

        DynamicJsonDocument docPar(SlavesIDCap);
        deserializeJson(docPar, jsonStr);
        const char* MQTT_Server = docPar["MQTT_Server"]; 
        const char* MQTT_Port = docPar["MQTT_Port"];

        if (strlen(newMQTTServer) >= sizeof(mqttServer)){
          Serial_Println(bDebug, "Error: MQTT Server longer than supported by the program");
          strcpy(strReply, "Error: MQTT Server longer than supported by the program");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if((strlen(newMQTTServer) + strlen(MQTT_Port) + 35) > sizeof(strReply)){
          Serial_Println(bDebug, "Overflow in the MQTT.Config structure");
          strcpy(strReply, "Overflow in the MQTT.Config structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        DynamicJsonDocument docSer(SlavesIDCap);
        docSer["MQTT_Server"] = newMQTTServer;
        docSer["MQTT_Port"] = MQTT_Port;

        serializeJson(docSer, strReply);
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
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);

        DynamicJsonDocument docPar(SlavesIDCap);
        deserializeJson(docPar, jsonStr);
        const char* MQTT_Server = docPar["MQTT_Server"]; 
        const char* MQTT_Port = docPar["MQTT_Port"];

        if((strlen(MQTT_Server) + strlen(newMQTTPort) + 35) > sizeof(strReply)){
          Serial_Println(bDebug, "Overflow in the MQTT.Config structure");
          strcpy(strReply, "Overflow in the MQTT.Config structure");
          webSocket.sendTXT(num, strReply);
          break;
        }

        DynamicJsonDocument docSer(SlavesIDCap);
        docSer["MQTT_Server"] = MQTT_Server;
        docSer["MQTT_Port"] = newMQTTPort;

        serializeJson(docSer, strReply);
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

  Serial_Print(bDebug, "Setting AP (Access Point)…");
  
  yield();
  display.fillScreen(ILI9341_BLACK);
  yield();
  display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Master");
  display.println("Setting Access Point...");

  IPAddress _ip = IPAddress(10, 0, 1, 100);
  IPAddress _gw = IPAddress(10, 0, 1, 1);
  IPAddress _sn = IPAddress(255, 255, 255, 0);

  //.softAP(const char* ssid, const char* password, int channel, int ssid_hidden, int max_connection)
  WiFi.softAP(APssid, APpassword, 1, 0, 1);

  WiFi.softAPConfig(_ip, _gw, _sn);

  IPAddress IP = WiFi.softAPIP();


  Serial_Print(bDebug, "AP IP address: ");
  Serial_Println(bDebug, IP.toString().c_str());
  yield();
  display.fillScreen(ILI9341_BLACK);
  yield();
  display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Master");
  display.println("AP IP address: ");
  display.println(IP);
  

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

  if (!PEin.digitalRead(APPin)){
    Serial_Println(bDebug, "Adhoc button pressed...");
    yield();
    display.fillScreen(ILI9341_BLACK);
    yield();
    display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("Adhoc button pressed...");
     
  }

  // !!!! Activate Access Point Configuration !!!!
  AP_Timer = millis();
  while (!PEin.digitalRead(APPin)){
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
  Serial_Println(bDebug, "\n" );
  
  DynamicJsonDocument docPar(SlavesIDCap);
  
// !!!! Load Slaves
  jsonStr = readFile(SPIFFS, "/SlavesID.config");
  
  deserializeJson(docPar, jsonStr);
  const char* SlavesID = docPar["SlavesID"]; 
  int nSlaves = docPar["nSlaves"];
  
  int i=0;
  char *idToken;
  idToken = strtok((char*) SlavesID, "-");
  while (idToken != NULL){
    slaves[i].id = strtoull(idToken, NULL, 16);
    idToken = strtok (NULL, "-");
    i++;
    //Serial_Print(bDebug, slaves[i].id, HEX);
    //Serial_Print(bDebug, "-");
  }

  Serial_Println(bDebug, "Slaves Loaded ");

// !!!! Load Wifi Configuration
  jsonStr = readFile(SPIFFS, "/Wifi.config");
  
  deserializeJson(docPar, jsonStr);
  const char* wifiSSID = docPar["SSID"]; 
  const char* filePassword = docPar["Password"];
  
  strcpy(ssid, wifiSSID);
  strcpy(password, filePassword);

  Serial_Print(bDebug, "Wifi Config Loaded: ");
  Serial_Print(bDebug, "SSID: ");
  Serial_Print(bDebug, ssid);
  Serial_Print(bDebug, "Password: ");
  Serial_Println(bDebug, password);

// !!!! Load MQTT Server Configuration
  jsonStr = readFile(SPIFFS, "/MQTT.config");
  
  deserializeJson(docPar, jsonStr);
  const char* MQTT_Server = docPar["MQTT_Server"]; 
  const char* MQTT_Port = docPar["MQTT_Port"];

  strcpy(mqttServer, MQTT_Server);
  mqttPort = (uint16_t) strtoul(MQTT_Port, NULL, 0);

  Serial_Println(bDebug, "MQTT Server Config Loaded");
  
// !!!! Load Client ID
  jsonStr = readFile(SPIFFS, "/ClientID.config");
  
  deserializeJson(docPar, jsonStr);
  const char* fileClientID = docPar["ClientID"];
  strcpy(clientID, fileClientID);
  Serial_Println(bDebug, "Client ID Loaded");

  Serial_Println(bDebug, "\n" );
}

#endif
