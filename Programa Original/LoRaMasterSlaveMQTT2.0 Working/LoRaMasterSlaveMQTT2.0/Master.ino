//Compila apenas se MASTER estiver definido no arquivo principal
#ifdef MASTER

#include "FS.h"
#include "SPIFFS.h"

//Lib de MQTT
#include <PubSubClient.h>

//Matriz de Digital Output Pins (Ate 16);
int pinDigOut[] = {6, 5, 4, 3, 2, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

//Matriz de Digital Input Pins (Ate 16);
int pinDigIn[] = {3, 2, 1, 0, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1};

//Define Numero maximo de DO pins
int nMaxDO;
//Define Numero maximo de DI pins
int nMaxDI;

//Server MQTT que iremos utlizar
//#define MQTT_SERVER "saorafael.sytes.net"
#define MQTTRETAINED false

//Define o numero maximo de slaves
#define NMAX_SLAVES 8
Slave slaves[NMAX_SLAVES]; // Utilizando 8 slaves
uint64_t thisdev_ID;

char ssid[255];
char password[255];
char mqttServer[255];
uint16_t mqttPort;
char clientID[128];
//{0xac9e3ba4ae30};

/*Callback function header
 * The callback function header needs to be declared
 * before the PubSubClient constructor and the
 * actual callback defined afterwards.
 */
void MQTTCallback(char* topico, byte* message, unsigned int msgSize);

//Cliente WiFi que o MQTT irá utilizar para se conectar
WiFiClient wifiClient;

//Cliente MQTT, passamos a url do server, a porta
//e o cliente WiFi
PubSubClient client(wifiClient);

//Intervalo entre os envios
#define INTERVAL 10000

//Tempo de espera de resposta após envio de solicitação
#define RECEIVE_WINDOW 3000

//Intervalo que o Botao deve ser pressionado para ativar Access Point
#define ACCESSPOINT_TIME 3000

//Define o Botao que sobe o Access Point
const int APPin = 7;

//Define Numero maximo de DO / DI
int nMaxIO;

//Define Numero maximo de AO
int nMaxAO;

//Define Numero maximo de AI
int nMaxAI;

//Usado para controlar intervalo entre envios do master.
long lastSentTime=0;

bool firstMsg;

int loopIndex=0;

void setup() {
  Serial.begin(115200);
  //Chama a configuração do Watchdog (Tempo de 30s p/ Master)
  configureWatchdog(30000000);

  //Chama a configuração inicial do display
  setupDisplay();

  displayMessage(10);

  //Inicializa input/output
  PEout.init();
  PEin.init();

  //Define Numero maximo de DO pins.
  nMaxDO = (sizeof(pinDigOut) / sizeof(pinDigOut[0]));
  //Define Numero maximo de DI pins.
  nMaxDI = (sizeof(pinDigIn) / sizeof(pinDigIn[0]));

  //Inicializa Digital Input`s Pins
  for (int i = 0; i < nMaxDI; i++) {
    if (pinDigIn[i] >= 0) {
      PEin.pinMode(pinDigIn[i], INPUT);
    }
  }
  
  //Inicializa arquivos de configuracao.
  startFileManager();
  //Carrega as configurações persistentes.
  loadConfig();
  //inicia Rotina de criptografia
  aes_init();
  //Define Numero maximo de DO / DI
  nMaxIO = sizeof(slaves[0].DigOut)*8;
  //Define Numero maximo de AO
  nMaxAO = sizeof(slaves[0].AO)/sizeof(slaves[0].AO[0]);
  //Define Numero maximo de AI
  nMaxAI = sizeof(slaves[0].AI)/sizeof(slaves[0].AI[0]);
  
  //Inicia variavel que registra ID do Master
  thisdev_ID = getChipID();
  Serial.println("Device ID: " + uint64ToHexStr(thisdev_ID));

  //Se AP button estiver pressionado, subir AP de configuracao.
  CheckAPInterrupt();

  //Mostra Slaves registrados ate o momento
  printSlavesID();

  //Chama a configuração inicial do LoRa
  setupLoRa();
  //Set WiFi Events
  WiFi.onEvent(WiFiEvent);
  //Conectamos à rede WiFi
  setupWiFi();
  //Configura server MQTT
  client.setServer((const char*) mqttServer, mqttPort);
  //Conectamos ao server MQTT
  connectMQTTServer();
  //Configura CallBack MQTT
  client.setCallback(MQTTCallback);
  //Used to not wait the INTERVAL before sending the first message;
  firstMsg = true;
  
  displayMessage(40);

/*
  //For debugging purposes \/
  slaves[0].DigOut = 0xFFF0;
  slaves[0].DigIn = 0xFFF1;
  slaves[0].AO[0] = 0xFFF2;
  slaves[0].AO[1] = 0xFFF3;
  slaves[0].AI[0] = 0xFFF4;
  slaves[0].AI[1] = 0xFFF5;
  slaves[0].DigOutmodified = false;
  slaves[0].AOmodified[0] = false;
  slaves[0].AOmodified[1] = true;*/
  
  loopIndex = 0;

  displayMessage(50);
  
}

void loop() {

  //reseta o temporizador (alimenta o watchdog)
  timerWrite(timer, 0);

  // Reconecta MQTT server, se cair.
  if (!client.connected() && (WiFi.status() == WL_CONNECTED)) {
    connectMQTTServer();
  }else if(WiFi.status() != WL_CONNECTED){
    WiFi.disconnect(true);
    delay(1000);
    setupWiFi();
  }
  client.loop();

  if (slaves[0].id != 0){ //Só comunica se houver algum Slave registrado
    
    //Reseta o looping entre os slaves
    if (slaves[loopIndex].id == 0) loopIndex=0;

    //Garante a espera do tempo INTERVAL antes de enviar mensagem ao proximo Slave.
    bool messageSent = false;

    while (!messageSent){
    
      //Se passou o tempo definido em INTERVAL desde o último envio ou se for a primeira mensagem
      if ((millis() - lastSentTime > INTERVAL) || (firstMsg == true)) {

        if (firstMsg == true) firstMsg = false;//Reset first Msg
        
        //Marcamos o tempo que ocorreu o último envio
        lastSentTime = millis();

        //Variable that holds which one to communicate next.
        int nextSlaveIndex;
        
        if (!queue_isEmpty()){ //Prioritize the ones on the queue
          nextSlaveIndex = dequeue();
        }else{
          nextSlaveIndex = loopIndex;
          loopIndex += 1;
        }

        Serial.print("Next Slave Index: ");
        Serial.println(nextSlaveIndex);
        
        //Envia o pacote para informar ao Slave que queremos receber os dados
        send(nextSlaveIndex);
  
        //abre janela de recebimento por 3 segundos ou ate receber.
        long tempTimer = millis();
        while ((millis() - tempTimer) < RECEIVE_WINDOW){
          //Verificamos se há pacotes para recebermos
          if (receive(nextSlaveIndex)){
            timerWrite(timer, 0); //alimenta o watchdog
            break;
          }
          client.loop();

        }
        Serial.print("Janela recebimento aberta por: ");
        Serial.println((millis() - tempTimer));
        Serial.println();     
        
        publishDataOnMQTT(nextSlaveIndex);
  
        //Se passou o tempo definido em COMMAXTIME desde a ultima comunicação
        if (millis() - slaves[nextSlaveIndex].lastRecTime > COMMAXTIME) {
          slaves[nextSlaveIndex].isOnline=false;
        }
  
        displayMessage(60); //Mostra status dos Slaves (Offline/Online)

        messageSent = true;
        
      }

      yield();
      client.loop();
      //Se AP button estiver pressionado, subir AP de configuracao.
      CheckAPInterrupt();

    }
    
  }
    
}

 /* Mensagem enviada tem o formato:
 * "GETDATA + (TamanhoMsg & slaveID & initialization vector & data criptografado)"
 * Parte em parenteses é montada dentro de encryptMessage()
 * data= 
 * "DO" + hex(DO) + ";" + "AO0" + hex(AO0) + ";" + "AO1" + hex(AO1) + ";"
 * Exemplo:
 * DOFFFF;AO0FFFF;AO1FFFF;
 * Só envia o output que foi alterado pelo usuário
 * Se nenhum output foi alterado, envia "empty"
 */
void send(int slaveIndex) {
  // !!!! Cria data !!!!
  String data;
  data = encodeSendData(slaveIndex);
  // \!!!! Cria data !!!!
  if (!slaves[slaveIndex].bmodSent && !data.equalsIgnoreCase("empty")){
    slaves[slaveIndex].bmodSent=true; //Se modificacao nao foi enviada, transforma-la em enviada.
  }

  Serial.println("Mensagem Enviada:");
  Serial.println("Data: " + data);

  //Criptografa mensagem com endereco do Slave.
  String slaveID = uint64ToHexStr(slaves[slaveIndex].id);
  //Serial.println("Antes Cryptografia");
  //Linha necessaria por conta de bug na biblioteca AES. Garante que a cryptografia vai funcionar
  if (bEncrypt) encryptMessage(data, slaveID);
  //Serial.println("Pos primeira Cryptografia");
  if (bEncrypt) data = encryptMessage(data, slaveID);
  //Serial.println("Pos segunda Cryptografia");

  //Inicializa o pacote
  LoRa.beginPacket();
  //Envia o que está contido em "GETDATA" + payload
  LoRa.print(GETDATA + data);
  //Finaliza e envia o pacote
  LoRa.endPacket();

  Serial.println("Encrypted: " + GETDATA + data);
}

//Retornar verdadeiro se recebemos o pacote do slave esperado.
bool receive(int slaveIndex) {
  //Tentamos ler o pacote
  int packetSize = LoRa.parsePacket();
  //Serial.println(packetSize);

  //Verificamos se o pacote tem o tamanho mínimo de caracteres que esperamos
  if (packetSize > SETDATA.length()) {
    Serial.println("Mensagem Recebida");
    String received = "";
    //Armazena os dados do pacote em uma string
    while (LoRa.available()) {
      received += (char) LoRa.read();
    }
    //Verifica se a string possui o que está contido em "SETDATA"
    int index = received.indexOf(SETDATA);
    if (index >= 0) {
      //Recuperamos a string que está após o "SETDATA",
      //que no caso serão os dados de nosso interesse criptografados
      String data = received.substring(SETDATA.length());

      //Decriptografa mensagem do slave de interesse.
      if (bEncrypt) data = decryptMessage(data, uint64ToHexStr(slaves[slaveIndex].id));

      //Verifica se mensagem foi decriptofrada com sucesso
      if (data.length() > 0) {
        slaves[slaveIndex].isOnline=true; //Slave esta respondendo
        //Reseta temporizador - tempo desde ultima troca de dados
        slaves[slaveIndex].lastRecTime = millis();

        Serial.println("Data: " + data);
        //Serial.println();

        decodeDataMsg(data, slaveIndex);
        
        //publishDataOnMQTT(slaveIndex);

        if (slaves[slaveIndex].bmodSent){ //So reseta booleanas se a modificacao foi enviada ao Slave.
          //Uma vez que recebeu, os Outputs estão atualizados - Resetar modified bool.
          slaves[slaveIndex].DigOutmodified = false;
          
          for (int i=0; i < nMaxAO; i++ ){ //Resetar modified bool.
            slaves[slaveIndex].AOmodified[i]= false;
          }
        }

        //Tempo que demorou para o Master criar o pacote, enviar o pacote,
        //o Slave receber, fazer a leitura, criar um novo pacote, enviá-lo
        //e o Master receber e ler e tambem publicar no mqtt
        //String waiting = String(millis() - slaves[slaveIndex].lastSentTime);

        //Mostra no display os dados e o tempo que a operação demorou
        /*display.fillScreen(ILI9341_BLACK);
        display.setCursor(0, 0);
        display.println("Origem: ");
        display.setCursor(0, 10);
        display.println(uint64ToHexStr(slaves[loopIndex].id));
        display.setCursor(0, 20);
        //display.println("Temperatura: " + String(Values_iTemperature) + " C");
        display.setCursor(0, 30);
        //display.println("Umidade: " + String(Values_iHumidity) + " %");
        display.setCursor(0, 40);
        //display.println("Status Out1: " + String(bOutputStatus1));
        display.setCursor(0, 50);
        display.println("Tempo Op: " + waiting + "ms");
        display.setCursor(0, 50);
        display.println("RSSI: " + String(LoRa.packetRssi()));
        */

        return true;
      }
    }
  }
  return false;
}

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

/*Função responsável por conectar à rede WiFi
 */
void setupWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  displayMessage(20);
  
  //Manda o esp se conectar à rede através
  //do ssid e senha
  WiFi.begin(ssid, password);
  
  timerWrite(timer, 0); //Alimenta WatchDog
  long wifiTimer = millis();
  //Espera até que a conexão com a rede seja estabelecida por 30s se nao continua progama.
  while ((WiFi.status() != WL_CONNECTED) && ((millis()-wifiTimer) < 15000)) {
    timerWrite(timer, 0); //Alimenta WatchDog
    delay(500);
    Serial.print(".");
  }

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
        //Inscreve nos topicos que recebe Status desejado dos outputs
        //Topico = "appPub" / ID do clinte / ID do slave / Tipo Sinal (DO ou AO) /
        subsToTopics();
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
void publishDataOnMQTT(int slaveIndex){
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

/* data= "DO" + hex(DO) + ";" + "AO0" + hex(AO0) + ";" + "AO1" + hex(AO1) + ";"
 * Exemplo:
 * DOFFFF;AO0FFFF;AO1FFFF;
 * Só envia o output que foi alterado pelo usuário
 * Se nenhum output foi alterado, envia "empty"
 */
String encodeSendData(int slaveIndex){
  String msgData;
  
  // Se nao houve alteracao no output, enviar "empty"
  if (!(slaves[slaveIndex].DigOutmodified || 
              slaves[slaveIndex].AOmodified[0] || slaves[slaveIndex].AOmodified[0])){
    msgData = "empty";
  }else{

    msgData = ";" + msgData;
    
    for (int i = nMaxAO - 1 ; i >= 0 ; i--) {
      msgData = String(slaves[slaveIndex].AO[i], HEX) + msgData;
      msgData = "AO" + String(i, DEC) + msgData;
      msgData = ";" + msgData;
    }
    
    msgData = String(slaves[slaveIndex].DigOut, HEX) + msgData;
    msgData = "DO" + msgData;

  }

  return msgData;
}

//Decode do master atualiza os Input`s e Output`s (para manter coerencia entre master/slave)
void decodeDataMsg(String msgData, int slaveIndex){
  int index;
  String tempStr;
  String objStr;

  while(msgData.indexOf(";") >= 0){
    index = msgData.indexOf(";");
    tempStr = msgData.substring(0, index); //Take a part of the data
    msgData = msgData.substring(index + 1); //Keep the rest

    if (tempStr.indexOf("DO") >= 0){ //Contains the DO part
      objStr = "DO";
      tempStr = tempStr.substring(objStr.length());
      slaves[slaveIndex].DigOut = strtoul(tempStr.c_str(),NULL,16); //Converte de volta para HEX
      continue;
    }

    if (tempStr.indexOf("DI") >= 0){ //Contains the DI part
      objStr = "DI";
      tempStr = tempStr.substring(objStr.length());
      slaves[slaveIndex].DigIn = strtoul(tempStr.c_str(),NULL,16); //Converte de volta para HEX
      continue;
    }


    //Atualiza AO. (AO0, AO1, AO2 e etc.)
    for (int j = 0; j < nMaxAO; j++){
      objStr = "AO" + String(j,DEC);
      if (tempStr.indexOf(objStr) >= 0){ //Contains the AOj part
        tempStr = tempStr.substring(objStr.length());
        slaves[slaveIndex].AO[j] = strtoul(tempStr.c_str(),NULL,16); //Converte de volta para HEX
        continue;
      }
    }

    //Atualiza AI. (AI0, AI1, AI2 e etc.)
    for (int j = 0; j < nMaxAI; j++){
      objStr = "AI" + String(j,DEC);
      if (tempStr.indexOf(objStr) >= 0){ //Contains the AIj part
        tempStr = tempStr.substring(objStr.length());
        slaves[slaveIndex].AI[j] = strtoul(tempStr.c_str(),NULL,16); //Converte de volta para HEX
        continue;
      }
    }
  }
}

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

void printSlavesID(){
  
  Serial.print("Slaves ID:");
  for (int i = 0; i < (NMAX_SLAVES); i++) {
    if (!(slaves[i].id == 0)){ //Slave ID inicializa com zero
      Serial.print(" - ");
      Serial.print(uint64ToHexStr(slaves[i].id));
    }
  }
  Serial.println();
  
}


#endif
