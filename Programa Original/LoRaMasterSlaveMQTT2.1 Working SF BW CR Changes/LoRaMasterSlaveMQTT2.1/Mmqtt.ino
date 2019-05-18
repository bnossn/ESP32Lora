//Compila apenas se MASTER estiver definido no arquivo principal
#ifdef MASTER


//Função que será chamada ao receber mensagem do servidor MQTT
void MQTTCallback(char* topico, byte* message, unsigned int msgSize) {
  timerWrite(timer, 0); //Alimenta WatchDog

  //Convertendo a mensagem recebida para string
  message[msgSize] = '\0';
  String strMessage = String((char*)message);
  strMessage.toLowerCase();  
  String strTopico = String(topico);
  strTopico.toLowerCase();
  //float f = s.toFloat();

  Serial.print("Mensagem recebida! Topico: ");
  Serial.print(strTopico);
  Serial.print(". Tamanho: ");
  Serial.print(String(msgSize).c_str());
  Serial.print(". Mensagem: ");
  Serial.println(strMessage);

  String topic_ClientID;
  String topic_SlaveID;
  String topic_OutputType; //(DO or AO)
  String topic_OutputIndex; //(1, 2, 3, ...)
  String topic_OutputValue;

  String strCLIENT_ID = String(clientID);
  strCLIENT_ID.toLowerCase();
  
  Serial.print("MQTTCallback: strCLIENT_ID: ");
  Serial.println(strCLIENT_ID);
  Serial.print("MQTTCallback: strTopico.indexOf(strCLIENT_ID): ");
  Serial.println(strTopico.indexOf(strCLIENT_ID));
  
  if (!(strTopico.indexOf(strCLIENT_ID) >= 0)){ //Verificar se é para este cliente.
    Serial.print("Mensagem nao é para este cliente: ");
    return;
  }

  topic_OutputValue = strMessage; //strMessage holds the OutputValue
  //Garante que pelo menos o primeiro digito é numerico \/
  if (topic_OutputValue.charAt(0) < '0' || topic_OutputValue.charAt(0) > '9'){
    Serial.println("MQTTCallback: Output value nao numerico!");
    return;
  }

// !!!! <Recupera os dados do topico>
  int index = strTopico.indexOf("/");
  if(index >= 0){//Primeira parte recupera o "appPub"
    if (!(strTopico.substring(0, index).equalsIgnoreCase("appPub"))){
      Serial.println("MQTTCallback: Topico nao contem \"appPub\"");
      return;
    }
    strTopico = strTopico.substring(index + 1); //keep the rest;

    Serial.print("MQTTCallback: Primeira parte strTopico: ");
    Serial.println(strTopico);
  }

  index = strTopico.indexOf("/");
  if(index >= 0){//Segunda parte recupera o client ID
    topic_ClientID = strTopico.substring(0, index);
    
    if (topic_ClientID.length() == 0){
      Serial.println("MQTTCallback: Tamanho Client ID nao pode ser zero.");
      return;
    }
    strTopico = strTopico.substring(index + 1); //keep the rest;

    Serial.print("MQTTCallback: Segunda parte strTopico: ");
    Serial.println(strTopico);    
  }

  index = strTopico.indexOf("/");
  if(index >= 0){//Terceira parte recupera o SlaveID
    topic_SlaveID = strTopico.substring(0, index);
    
    if (topic_SlaveID.length() == 0){
      Serial.println("MQTTCallback: Tamanho Slave ID nao pode ser zero.");
      return;
    }
    strTopico = strTopico.substring(index + 1); //keep the rest;

    Serial.print("MQTTCallback: Terceira parte strTopico: ");
    Serial.println(strTopico);
  }

  //Quarta parte recupera o OutputType e OutputIndex
  //strTopico = OutputType + OutputIndex (ex: AO2)
  
  Serial.print("MQTTCallback: strTopico: ");
  Serial.println(strTopico);
  
  Serial.print("MQTTCallback: (strTopico.indexOf(\"DO\") >= 0) || (strTopico.indexOf(\"do\") >= 0): ");
  Serial.println((strTopico.indexOf("DO") >= 0) || (strTopico.indexOf("do") >= 0));
  
  if ((strTopico.indexOf("DO") >= 0) || (strTopico.indexOf("do") >= 0)){ //Is it a DO?
    topic_OutputType = "DO";
  }else if((strTopico.indexOf("AO") >= 0) || (strTopico.indexOf("ao") >= 0)){//Is it an AO?
    topic_OutputType = "AO";
  }else{ //If it`s neither of the two, return;
    Serial.println("MQTTCallback: Output Type Invalido!");
    return;
  }
  
  topic_OutputIndex = strTopico.substring(topic_OutputType.length());
  
  Serial.print("MQTTCallback: topic_OutputIndex: ");
  Serial.println(topic_OutputIndex);
  
  Serial.print("MQTTCallback: (topic_OutputIndex.charAt(0) < '0' || topic_OutputIndex.charAt(0) > '9'): ");
  Serial.println((topic_OutputIndex.charAt(0) < '0' || topic_OutputIndex.charAt(0) > '9'));
  
  //Garante que pelo menos o primeiro digito é numerico \/
  if (topic_OutputIndex.charAt(0) < '0' || topic_OutputIndex.charAt(0) > '9'){
    Serial.println("MQTTCallback: Output Index nao numerico!");
    return;
  }
// !!!! <\Recupera os dados do topico>

Serial.print("topic_ClientID: ");
Serial.println(topic_ClientID);
Serial.print("topic_SlaveID: ");
Serial.println(topic_SlaveID);
Serial.print("topic_OutputType: ");
Serial.println(topic_OutputType);
Serial.print("topic_OutputIndex: ");
Serial.println(topic_OutputIndex);
Serial.print("topic_OutputValue: ");
Serial.println(topic_OutputValue);


// !!!! <Atualiza Slave`s Variables>
  if (topic_ClientID.equalsIgnoreCase(String(clientID))){//Double Check ClientID
    
    for (int i = 0; i < (NMAX_SLAVES); i++){ //Iterate through each Slave
      
      if (topic_SlaveID.equalsIgnoreCase(uint64ToHexStr(slaves[i].id))){ //Se mensagem for para este Slave
        
        enqueue(i); //Adiciona este slave na fila de prioridade.
        
        if (topic_OutputType.equalsIgnoreCase("DO")){ //Atualizando DO
          if(topic_OutputValue.toInt() == 1){
            
            Serial.print("anterior slaves[i].DigOut: ");
            Serial.println(slaves[i].DigOut);
            slaves[i].setDO(topic_OutputIndex.toInt());
            Serial.print("novo slaves[i].DigOut: ");
            Serial.println(slaves[i].DigOut);
            
          }else if(topic_OutputValue.toInt() == 0){
            
            Serial.print("anterior slaves[i].DigOut: ");
            Serial.println(slaves[i].DigOut);
            slaves[i].resetDO(topic_OutputIndex.toInt());
            Serial.print("novo slaves[i].DigOut: ");
            Serial.println(slaves[i].DigOut);
            
          }
          slaves[i].DigOutmodified = true;
          slaves[i].bmodSent = false;
        }

        if(topic_OutputType.equalsIgnoreCase("AO")){ //Atualizando AO
          slaves[i].AO[topic_OutputIndex.toInt()] = (uint16_t) topic_OutputValue.toInt();
          slaves[i].AOmodified[topic_OutputIndex.toInt()] = true;
          slaves[i].bmodSent = false;
        }
        
      }
      
    }
    
  }
// !!!! <\Atualiza Slave`s Variables>

  Serial.println();
  
}

//Função responsável por conectar ao server MQTT e subescrever aos topicos.
void connectMQTTServer() {
  if (WiFi.status() == WL_CONNECTED){
    
    Serial.println();
      long mqttConnectTimer = millis();
    //Espera até que a conexão com o MQTT seja estabelecida por 15s se nao continua progama.
    while ((!client.connected()) && ((millis()-mqttConnectTimer) < 15000 )) {
      //reseta o temporizador (alimenta o watchdog evitando reset do sistema)
      timerWrite(timer, 0);
      Serial.println("Connecting to MQTT Server...");
  
      displayMessage(30);
  
      String tempDevID =  uint64ToHexStr(thisdev_ID);
  
      //Se conecta ao id que definimos
      if (client.connect(tempDevID.c_str())) {
        //Se a conexão foi bem sucedida
        Serial.println("connected");
        
      } else {
        //Se ocorreu algum erro
        Serial.print("error = ");
        Serial.println(client.state());
        Serial.println(" try again in 1 second");
        //reseta o temporizador (alimenta o watchdog evitando reset do sistema)
        timerWrite(timer, 0);
        // Wait 1 second before retrying
        delay(1000);
      }
    }
    Serial.println();
    
  }
}

//Topico = ID do clinte / ID do slave / Tipo Sinal (DO, DI, AO ou AI) /
void publishDataOnMQTT(int slaveIndex){ //MQTT Dash APP Format.
    String tempTopic;

    client.loop();
      
    for (int i=0; i<nMaxIO; i++ ){ //Publica DO`s
      tempTopic = String(clientID) + "/" + uint64ToHexStr(slaves[slaveIndex].id) + "/" + "DO" + String(i);
      client.publish(tempTopic.c_str(), String(slaves[slaveIndex].getDO(i)).c_str(), MQTTRETAINED);
    }

    for (int i=0; i<nMaxIO; i++ ){ //Publica DI`s
      tempTopic = String(clientID) + "/" + uint64ToHexStr(slaves[slaveIndex].id) + "/" + "DI" + String(i);
      client.publish(tempTopic.c_str(), String(slaves[slaveIndex].getDI(i)).c_str(), MQTTRETAINED);
    }

    for (int i=0; i < nMaxAO; i++ ){ //Publica AO`s
      tempTopic = String(clientID) + "/" + uint64ToHexStr(slaves[slaveIndex].id) + "/" + "AO" + String(i);
      client.publish(tempTopic.c_str(), String(slaves[slaveIndex].AO[i],DEC).c_str(), MQTTRETAINED);
    }

    for (int i=0; i < nMaxAI; i++ ){ //Publica AI`s
      tempTopic = String(clientID) + "/" + uint64ToHexStr(slaves[slaveIndex].id) + "/" + "AI" + String(i);
      client.publish(tempTopic.c_str(), String(slaves[slaveIndex].AI[i],DEC).c_str(), MQTTRETAINED);
    }
}

//Topico = "Telemetry" -> Publish = GatewayMac/SlaveMac/Pino/Valor
void publishDataOnPubSub(int slaveIndex){ //google PubSub Format
    char PubTopic[12];
    char PubMsg[64];
    char buffer[12];

    client.loop();

    strcpy(PubTopic, "Telemetry");

    //Publish = GatewayMac/SlaveMac/Pino/Valor

    for (int i=0; i<nMaxIO; i++ ){ //Publica DO`s
      strcpy(PubMsg, "");
      strcat(PubMsg, uint64ToHexStr(thisdev_ID).c_str());
      strcat(PubMsg, "/");
      strcat(PubMsg, uint64ToHexStr(slaves[slaveIndex].id).c_str());
      strcat(PubMsg, "/");
      strcat(PubMsg, "DO");
      sprintf (buffer, "%d", i); //int to string
      strcat(PubMsg, buffer);
      strcat(PubMsg, "/");
      sprintf (buffer, "%d", slaves[slaveIndex].getDO(i)); ////int to string
      strcat(PubMsg, buffer);
      client.publish(PubTopic, PubMsg, MQTTRETAINED);
    }

    for (int i=0; i<nMaxIO; i++ ){ //Publica DI`s
      strcpy(PubMsg, "");
      strcat(PubMsg, uint64ToHexStr(thisdev_ID).c_str());
      strcat(PubMsg, "/");
      strcat(PubMsg, uint64ToHexStr(slaves[slaveIndex].id).c_str());
      strcat(PubMsg, "/");
      strcat(PubMsg, "DI");
      sprintf (buffer, "%d", i); //int to string
      strcat(PubMsg, buffer);
      strcat(PubMsg, "/");
      sprintf (buffer, "%d", slaves[slaveIndex].getDI(i)); ////int to string
      strcat(PubMsg, buffer);
      client.publish(PubTopic, PubMsg, MQTTRETAINED);
    }

    for (int i=0; i < nMaxAO; i++ ){ //Publica AO`s
      strcpy(PubMsg, "");
      strcat(PubMsg, uint64ToHexStr(thisdev_ID).c_str());
      strcat(PubMsg, "/");
      strcat(PubMsg, uint64ToHexStr(slaves[slaveIndex].id).c_str());
      strcat(PubMsg, "/");
      strcat(PubMsg, "AO");
      sprintf (buffer, "%d", i); //int to string
      strcat(PubMsg, buffer);
      strcat(PubMsg, "/");
      sprintf (buffer, "%d", slaves[slaveIndex].AO[i]); ////int to string
      strcat(PubMsg, buffer);
      client.publish(PubTopic, PubMsg, MQTTRETAINED);
    }

    for (int i=0; i < nMaxAI; i++ ){ //Publica AI`s
      strcpy(PubMsg, "");
      strcat(PubMsg, uint64ToHexStr(thisdev_ID).c_str());
      strcat(PubMsg, "/");
      strcat(PubMsg, uint64ToHexStr(slaves[slaveIndex].id).c_str());
      strcat(PubMsg, "/");
      strcat(PubMsg, "AI");
      sprintf (buffer, "%d", i); //int to string
      strcat(PubMsg, buffer);
      strcat(PubMsg, "/");
      sprintf (buffer, "%d", slaves[slaveIndex].AI[i]); ////int to string
      strcat(PubMsg, buffer);
      client.publish(PubTopic, PubMsg, MQTTRETAINED);
    }
}

//Inscreve nos topicos que recebe Status desejado dos outputs
//Topico = "appPub" / ID do clinte / ID do slave / Tipo Sinal (DO ou AO) /
void subsToTopics(){
  Serial.println();

  String tempTopic;

  for (int iSlave = 0; iSlave < (NMAX_SLAVES); iSlave++) {
    if (!(slaves[iSlave].id == 0)){ //Slave ID inicializa com zero
      
      for (int i=0; i<nMaxIO; i++ ){ //Inscreve DO`s
        tempTopic = String("appPub") + "/" + String(clientID) + "/" + uint64ToHexStr(slaves[iSlave].id) + "/" + "DO" + String(i);
        Serial.print("Subscribing to: " + tempTopic);
        Serial.print(" - ");
        Serial.println(client.subscribe(tempTopic.c_str()));
        //client.subscribe(tempTopic.c_str());
      }

      for (int i=0; i < nMaxAO; i++ ){ //Inscreve AO`s
        tempTopic = String("appPub") + "/" + String(clientID) + "/" + uint64ToHexStr(slaves[iSlave].id) + "/" + "AO" + String(i);
        Serial.print("Subscribing to: " + tempTopic);
        Serial.print(" - ");
        Serial.println(client.subscribe(tempTopic.c_str()));
        //client.subscribe(tempTopic.c_str());
      }
      
    }
  }
  Serial.println();
}



#endif
