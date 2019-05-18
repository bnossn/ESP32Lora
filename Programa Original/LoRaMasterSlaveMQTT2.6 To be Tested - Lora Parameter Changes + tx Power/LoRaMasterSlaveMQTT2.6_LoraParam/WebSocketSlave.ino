#ifndef MASTER

const char* APssid     = "PIA-Slave";
const char* APpassword = "PIA-admin";

//Global
WebSocketsServer webSocket = WebSocketsServer(80);

//Json Space
const size_t JsnBuffer = 2*JSON_ARRAY_SIZE(10) + 2*JSON_ARRAY_SIZE(16) + 8*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(9) + 80;
//const size_t JsnBuffer = 32*JSON_ARRAY_SIZE(10) + 33*JSON_ARRAY_SIZE(16) + JSON_OBJECT_SIZE(1) + 16*JSON_OBJECT_SIZE(5) + 510;


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
        strcpy(strReply, uint64ToHexStr(slave.id).c_str());
        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "GetGatewayID") == 0) {
        DynamicJsonDocument docPar(JsnBuffer);
        const char *jsonStr = readFile(SPIFFS, "/GatewayID.config");

        deserializeJson(docPar, jsonStr);
        const char* cGatewayID = docPar["GatewayID"];
        
        strcpy(strReply, cGatewayID);
        webSocket.sendTXT(num, strReply);
        break;
        
      }

      if (strcasecmp(appCommand, "ListAIMin") == 0) {
        const char *jsonStr = readFile(SPIFFS, "/AIMinVar.config");

        strcpy(strReply, jsonStr);
        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "ListAIMax") == 0) {
        const char *jsonStr = readFile(SPIFFS, "/AIMaxVar.config");

        strcpy(strReply, jsonStr);
        webSocket.sendTXT(num, strReply);
        break;
      }

      if (strcasecmp(appCommand, "ResetConfig") == 0) {

        rstSlaveConfigFiles();
        strcpy(strReply, "Configuration Files have been Reseted");
        webSocket.sendTXT(num, strReply);        
        break;
      }

      if (strcasecmp(appCommand, "ListFiles") == 0) {

        listDir(SPIFFS, "/", 0);
        readFile(SPIFFS, "/GatewayID.config"); 
        readFile(SPIFFS, "/AIMinVar.config"); 
        
        strcpy(strReply, "Configuration Files have been Listed");
        webSocket.sendTXT(num, strReply);        
        break;
      }

      //FIX ME: IF SOMETHING AFTER ":" IS NULL, CODE CRASHES
      char *token;
      token = strtok(appCommand, ":");

      if (strcasecmp(token, "SetGatewayID") == 0) {
        char *newGtwID;
        newGtwID = strtok(NULL, ":");
        strcpy(strReply, "");

        if (strlen(newGtwID) != 12 ){
          Serial_Println(bDebug, "Error: Gateway ID doesn`t have the expected length");
          strcpy(strReply, "Error: Gateway ID doesn`t have the expected length");
          webSocket.sendTXT(num, strReply);
          break;
        }

        DynamicJsonDocument docSer(JsnBuffer);
        docSer["GatewayID"] = newGtwID;

        serializeJson(docSer, strReply);
        writeFile(SPIFFS, "/GatewayID.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
        
      }

      //FIX ME: THE MIN VALUE HAS TO BE A NUMBER
      if (strcasecmp(token, "SetAIMin") == 0) { //SetAIMin:Index:Min
        char *cAIindex;
        cAIindex = strtok(NULL, ":");
        int iAIindex;
        char *cnewMin;
        cnewMin = strtok(NULL, ":");
        int inewMin;
        strcpy(strReply, "");

        if (cAIindex == NULL){
          Serial_Println(bDebug, "Error: Analog Input index cannot be NULL");
          strcpy(strReply, "Error: Analog Input index cannot be NULL");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if (strlen(cAIindex) != 1){
          Serial_Println(bDebug, "Error: Analog Input index is an one digit number");
          strcpy(strReply, "Error: Analog Input index is an one digit number");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if ( (*cAIindex < '0') or (*cAIindex > '7') ){
          Serial_Println(bDebug, "Error: Analog Input index has to be between 0 and 7");
          strcpy(strReply, "Error: Analog Input index has to be between 0 and 7");
          webSocket.sendTXT(num, strReply);
          break;
        }
       
        if (cnewMin == NULL){
          Serial_Println(bDebug, "Error: Min variable cannot be NULL");
          strcpy(strReply, "Error: Min variable cannot be NULL");
          webSocket.sendTXT(num, strReply);
          break;
        }

        const char *jsonStr = readFile(SPIFFS, "/AIMinVar.config");
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);

        DynamicJsonDocument docSer(JsnBuffer);
        JsonObject jsAI[8];

        inewMin = atoi(cnewMin);
        iAIindex = atoi(cAIindex);

        //Set the correct AI to the new value
        for (int i = 0; i < (sizeof(AIminvar) / sizeof(AIminvar[0])); i++) {
          
          sprintf (reg1buffer, "AI%d", i);
          jsAI[i] = docSer.createNestedObject(reg1buffer);
          
          if (iAIindex == i){
            jsAI[i]["Min"] = inewMin;
            AIminvar[i] = inewMin;
            continue;
          }
          
          jsAI[i]["Min"] = AIminvar[i];
       
        }

        serializeJson(docSer, strReply);
        writeFile(SPIFFS, "/AIMinVar.config", strReply);

        webSocket.sendTXT(num, strReply);
        break;
        
      }

      //FIX ME: THE MAX VALUE HAS TO BE A NUMBER
      if (strcasecmp(token, "SetAIMax") == 0) { //SetAIMax:Index:Max
        char *cAIindex;
        cAIindex = strtok(NULL, ":");
        int iAIindex;
        char *cnewMax;
        cnewMax = strtok(NULL, ":");
        int inewMax;
        strcpy(strReply, "");

        if (cAIindex == NULL){
          Serial_Println(bDebug, "Error: Analog Input index cannot be NULL");
          strcpy(strReply, "Error: Analog Input index cannot be NULL");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if (strlen(cAIindex) != 1){
          Serial_Println(bDebug, "Error: Analog Input index is an one digit number");
          strcpy(strReply, "Error: Analog Input index is an one digit number");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if ( (*cAIindex < '0') or (*cAIindex > '7') ){
          Serial_Println(bDebug, "Error: Analog Input index has to be between 0 and 7");
          strcpy(strReply, "Error: Analog Input index has to be between 0 and 7");
          webSocket.sendTXT(num, strReply);
          break;
        }
       
        if (cnewMax == NULL){
          Serial_Println(bDebug, "Error: Max variable cannot be NULL");
          strcpy(strReply, "Error: Max variable cannot be NULL");
          webSocket.sendTXT(num, strReply);
          break;
        }

        if (*cnewMax == '0'){
          Serial_Println(bDebug, "Error: Max variable cannot be Zero");
          strcpy(strReply, "Error: Max variable cannot be Zero");
          webSocket.sendTXT(num, strReply);
          break;
        }

        const char *jsonStr = readFile(SPIFFS, "/AIMaxVar.config");
        Serial_Print(bDebug, "Data Before Writing Operation: ");
        Serial_Println(bDebug, jsonStr);

        DynamicJsonDocument docSer(JsnBuffer);
        JsonObject jsAI[8];

        inewMax = atoi(cnewMax);
        iAIindex = atoi(cAIindex);

        //Set the correct AI to the new value
        for (int i = 0; i < (sizeof(AImaxvar) / sizeof(AImaxvar[0])); i++) {
          
          sprintf (reg1buffer, "AI%d", i);
          jsAI[i] = docSer.createNestedObject(reg1buffer);
          
          if (iAIindex == i){
            jsAI[i]["Max"] = inewMax;
            AImaxvar[i] = inewMax;
            continue;
          }
          
          jsAI[i]["Max"] = AImaxvar[i];
       
        }

        serializeJson(docSer, strReply);
        writeFile(SPIFFS, "/AIMaxVar.config", strReply);

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
  
  displayMessage(110);

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
  display.println("Slave");
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
    displayMessage(120);
     
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

void loadSlaveConfig(){
  const char *jsonStr;
  Serial_Println(bDebug, "\n" );
  
  DynamicJsonDocument docPar(JsnBuffer);

// !!! Load Gateway ID  
  jsonStr = readFile(SPIFFS, "/GatewayID.config");
  Serial_Print(bDebug, "Data Before Writing Operation: ");
  Serial_Println(bDebug, jsonStr);

  deserializeJson(docPar, jsonStr);
  const char* cGatewayID = docPar["GatewayID"];
  //sscanf (cGatewayID,"%X",&gatewayID);
  gatewayID = strtoll(cGatewayID, NULL, 16);

  Serial_Println(bDebug, "Gateway Loaded ");
// !!! \Load Gateway ID  


// !!! Load AI Min
  jsonStr = readFile(SPIFFS, "/AIMinVar.config");
  Serial_Print(bDebug, "Data Before Writing Operation: ");
  Serial_Println(bDebug, jsonStr);

  deserializeJson(docPar, jsonStr);

  for (int i = 0; i < (sizeof(AIminvar) / sizeof(AIminvar[0])); i++) {
    sprintf (reg1buffer, "AI%d", i);    
    AIminvar[i] = docPar[reg1buffer]["Min"];;
  }
  Serial_Println(bDebug, "AI Min Variables Loaded");
// !!! \Load AI Min

// !!! Load AI Max
  jsonStr = readFile(SPIFFS, "/AIMaxVar.config");
  Serial_Print(bDebug, "Data Before Writing Operation: ");
  Serial_Println(bDebug, jsonStr);

  deserializeJson(docPar, jsonStr);

  for (int i = 0; i < (sizeof(AImaxvar) / sizeof(AImaxvar[0])); i++) {
    sprintf (reg1buffer, "AI%d", i);    
    AImaxvar[i] = docPar[reg1buffer]["Max"];;
  }
  Serial_Println(bDebug, "AI Max Variables Loaded");
// !!! \Load AI Min

  Serial_Println(bDebug, "\n" );
}

#endif
