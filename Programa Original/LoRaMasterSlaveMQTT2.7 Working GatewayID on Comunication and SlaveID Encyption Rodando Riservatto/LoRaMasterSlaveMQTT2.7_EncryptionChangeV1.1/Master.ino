//Compila apenas se MASTER estiver definido no arquivo principal
#ifdef MASTER

//Lib de MQTT
#include <PubSubClient.h>

//Reter Mensagens MQTT
#define MQTTRETAINED false

/*Callback function header
 * The callback function header needs to be declared
 * before the PubSubClient constructor and the
 * actual callback defined afterwards.
 */
void MQTTCallback(char* topico, byte* message, unsigned int msgSize);

//Cliente WiFi que o MQTT irá utilizar para se conectar
WiFiClient wifiClient;

//Cliente MQTT
PubSubClient client(wifiClient);

//Matriz de Digital Output Pins (Ate 16);
int pinDigOut[] = {6, 5, 4, 3, 2, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

//Matriz de Digital Input Pins (Ate 16);
int pinDigIn[] = {3, 2, 1, 0, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1};

//Define Numero maximo de DO pins
int nMaxDO;
//Define Numero maximo de DI pins
int nMaxDI;

//Define o numero maximo de slaves
#define NMAX_SLAVES 8
Slave slaves[NMAX_SLAVES]; // Utilizando 8 slaves

//Variável utilizada para Armazenar o ID deste Gateway.
uint64_t thisdev_ID;

//Define Matrix para registar RSSI e SNR da comunicação.
int SlavesRSSI[NMAX_SLAVES]; //Registra força do sinal.
float SlavesSNR[NMAX_SLAVES]; //Registra Signal to Noise Ratio.

char ssid[255];
char password[255];
char mqttServer[255];
uint16_t mqttPort;
char clientID[128];
//{0xac9e3ba4ae30};

//Intervalo entre os envios
#define INTERVAL 1000

//Tempo de espera de resposta após envio de solicitação
#define RECEIVE_WINDOW 3000

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

  //Verifica se os structs tem os requisitos minimos para a crypto. Trava controlador caso negativo.
  checkStructPadding();
  //Inicializa arquivos de configuracao.
  startGatewayFileManager();
  //Carrega as configurações persistentes.
  loadGatewayConfig();
  //Define Numero maximo de DO / DI
  nMaxIO = sizeof(slaves[0].DigOut)*8;
  //Define Numero maximo de AO
  nMaxAO = sizeof(slaves[0].AO)/sizeof(slaves[0].AO[0]);
  //Define Numero maximo de AI
  nMaxAI = sizeof(slaves[0].AI)/sizeof(slaves[0].AI[0]);
  
  //Inicia variavel que registra ID do Master
  thisdev_ID = getChipID();
  Serial_Print(bDebug, "Device ID: ");
  Serial_Println(bDebug, uint64ToHexStr(thisdev_ID).c_str());

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
  //Inscreve nos topicos que recebe Status desejado dos outputs
  subsToTopics();
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

        Serial_Print(bDebug, "Next Slave Index: ");
        sprintf (Sbuffer, "%u", nextSlaveIndex);
        Serial_Println(bDebug, Sbuffer);
        
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
        Serial_Print(bDebug, "Janela recebimento aberta por: ");
        sprintf (Sbuffer, "%lu", (millis() - tempTimer));
        Serial_Println(bDebug, Sbuffer);
        Serial_Println(bDebug, "\n" );     

  
        //Se passou o tempo definido em COMMAXTIME desde a ultima comunicação
        if (millis() - slaves[nextSlaveIndex].lastRecTime > COMMAXTIME) {
          slaves[nextSlaveIndex].isOnline=false;
        }
  
        displayMessage(60); //Mostra status dos Slaves (Offline/Online)

        //Publica se o Slave ainda estiver online
        if (slaves[nextSlaveIndex].isOnline){
          //publishDataOnMQTT(nextSlaveIndex);
          publishDataOnPubSub(nextSlaveIndex);
          // \/ publica via serial se o debug estiver desligado
          if (!bDebug){publishDataOnSerial(nextSlaveIndex);}
        }

        messageSent = true;
        
      }

      yield();
      client.loop();
      //Se AP button estiver pressionado, subir AP de configuracao.
      CheckAPInterrupt();

    }
    
  }

  //Se AP button estiver pressionado, subir AP de configuracao.
  CheckAPInterrupt();
    
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

  Serial_Println(bDebug, "Mensagem Enviada:");
  Serial_Print(bDebug, "Data: ");
  Serial_Println(bDebug, data.c_str());

 // !!!! Create Header
  Header header;
  header.idOrigem = thisdev_ID;
  gen_iv(aes_iv); //Gerar um vetor aleatório.
  memcpy(header.AES_iv, aes_iv, sizeof(aes_iv));
 // !!!! \Create Header
  
 // !!!! Create gtwData
  struct gtwData *ClearGTWDataPtr = (struct gtwData*) malloc(sizeof(struct gtwData));
  struct gtwData *cypherGTWDataPtr;

  strcpy(ClearGTWDataPtr->gtwMsg, data.c_str());
  ClearGTWDataPtr->idDestino = slaves[slaveIndex].id;
  
  //FIX ME - DATA BELOW HAS TO BE SET SOMEWHERE

  if (slaves[slaveIndex].imsgCounter >= 65000){ //Evita overflow no msgCounter
    slaves[slaveIndex].imsgCounter = 0;
    slaves[slaveIndex].bRstCounter = true; //Receive reseta para false;
  }

  slaves[slaveIndex].imsgCounter = slaves[slaveIndex].imsgCounter+1;
  ClearGTWDataPtr->imsgCounter = slaves[slaveIndex].imsgCounter;  
  ClearGTWDataPtr->bRstCounter = slaves[slaveIndex].bRstCounter;
 // !!!! \Create gtwData

  //Encrypt Gateway Data
  cypherGTWDataPtr = (struct gtwData*) encryptData(ClearGTWDataPtr, sizeof(struct gtwData), aes_key, header.AES_iv);

  //Inicializa o pacote
  LoRa.beginPacket();
  //envia o Header
  LoRa.write((uint8_t *)&header, sizeof(struct Header));
  //Envia o GTW data criptografado
  LoRa.write((uint8_t *) cypherGTWDataPtr, sizeof(struct gtwData));
  //Finaliza e envia o pacote
  LoRa.endPacket();

  free(ClearGTWDataPtr);
  ClearGTWDataPtr = NULL;
  free(cypherGTWDataPtr);
  cypherGTWDataPtr = NULL;

}

//Retornar verdadeiro se recebemos o pacote do slave esperado.
bool receive(int slaveIndex) {
  //Tentamos ler o pacote
  int packetSize = LoRa.parsePacket();

  //Verificamos se o pacote tem o tamanho mínimo de bytes que esperamos
  if (packetSize >= (sizeof(struct Header) + sizeof(struct slaveData))) { //recebeu algo
    sprintf (Sbuffer, "onReceive Size: %i bytes", packetSize);
    Serial_Println(bDebug, Sbuffer);

    struct Header *headerPtr = (struct Header*) malloc(sizeof(struct Header));
    LoRa.readBytes((uint8_t *)headerPtr, sizeof(struct Header)); //Le o Header

    Serial_Print(bDebug, "idOrigem: ");
    Serial_Println(bDebug, uint64ToHexStr(headerPtr->idOrigem).c_str());
  
    if (headerPtr->idOrigem == slaves[slaveIndex].id) {//Check se mensagem veio do device esperado
      Serial_Println(bDebug, "Passed the idOrigem Check: ");

      struct slaveData *cypherSlvDataPtr = (struct slaveData*) malloc(sizeof(struct slaveData));
      struct slaveData *clearSlvDataPtr;

      LoRa.readBytes((uint8_t *)cypherSlvDataPtr, sizeof(struct slaveData)); //Le o Cypher

      sprintf (Sbuffer, "cypherSlvDataPtr->imsgCounter: %i", cypherSlvDataPtr->imsgCounter);
      Serial_Println(bDebug, Sbuffer);
      sprintf (Sbuffer, "cypherSlvDataPtr->DigIn: %X", cypherSlvDataPtr->DigIn);
      Serial_Println(bDebug, Sbuffer);
      sprintf (Sbuffer, "cypherSlvDataPtr->DigOut: %X", cypherSlvDataPtr->DigOut);
      Serial_Println(bDebug, Sbuffer);

      clearSlvDataPtr = (struct slaveData*) decryptData(cypherSlvDataPtr, sizeof(struct slaveData), aes_key, headerPtr->AES_iv);

      // \/Check para confirmar que mensagem e para este Gateway.
      if (clearSlvDataPtr->idDestino == thisdev_ID){
        Serial_Println(bDebug, "Passed the idDestino Check: ");

        //Ultimo check para garantir que msg nao e repetida (Evita ataque de repeater)
        if (clearSlvDataPtr->imsgCounter == slaves[slaveIndex].imsgCounter){//Confere se msg recebida tem msm valor da msg enviada
          slaves[slaveIndex].isOnline=true; //Slave esta respondendo
          
          SlavesRSSI[slaveIndex] = LoRa.packetRssi(); //Registra força do sinal.
          SlavesSNR[slaveIndex] = LoRa.packetSnr(); //Registra Signal to Noise ratio

          //Reseta temporizador - tempo desde ultima troca de dados
          slaves[slaveIndex].lastRecTime = millis();

          decodeSlaveData(clearSlvDataPtr, slaveIndex);

          if (slaves[slaveIndex].bmodSent){ //So reseta booleanas se a modificacao foi enviada ao Slave.
            //Uma vez que recebeu, os Outputs estão atualizados - Resetar modified bool.
            slaves[slaveIndex].DigOutmodified = false;
            
            for (int i=0; i < nMaxAO; i++ ){ //Resetar modified bool.
              slaves[slaveIndex].AOmodified[i]= false;
            }
          }

          if (slaves[slaveIndex].bRstCounter == true){
            slaves[slaveIndex].bRstCounter = false;
          }

          sprintf (Sbuffer, "clearSlvDataPtr->imsgCounter: %i", clearSlvDataPtr->imsgCounter);
          Serial_Println(bDebug, Sbuffer);
          sprintf (Sbuffer, "clearSlvDataPtr->DigIn: %X", clearSlvDataPtr->DigIn);
          Serial_Println(bDebug, Sbuffer);
          sprintf (Sbuffer, "clearSlvDataPtr->DigOut: %X", clearSlvDataPtr->DigOut);
          Serial_Println(bDebug, Sbuffer);
          sprintf (Sbuffer, "clearSlvDataPtr->AI[0]: %i", clearSlvDataPtr->AI[0]);
          Serial_Println(bDebug, Sbuffer);
          sprintf (Sbuffer, "clearSlvDataPtr->AI[1]: %i", clearSlvDataPtr->AI[1]);
          Serial_Println(bDebug, Sbuffer);
          sprintf (Sbuffer, "clearSlvDataPtr->AI[2]: %i \n\n", clearSlvDataPtr->AI[2]);
          Serial_Println(bDebug, Sbuffer);

          //Tempo que demorou para o Master criar o pacote, enviar o pacote,
          //o Slave receber, fazer a leitura, criar um novo pacote, enviá-lo
          //e o Master receber e ler e tambem publicar no mqtt
          //String waiting = String(millis() - slaves[slaveIndex].lastSentTime);
          return true;
          
        } // /\Mensagem recebida com sucesso

      }

      free(cypherSlvDataPtr);
      cypherSlvDataPtr = NULL;
      free(clearSlvDataPtr);
      clearSlvDataPtr = NULL;
    }
    
    free(headerPtr);
    headerPtr = NULL;
  }
  
  return false;
}

/*Função responsável por conectar à rede WiFi
 */
void setupWiFi() {
  Serial_Println(bDebug, "\n" );
  Serial_Print(bDebug, "Connecting to ");
  Serial_Print(bDebug, ssid);

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
    Serial_Print(bDebug, ".");
  }

  Serial_Println(bDebug, "\n" );
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
              slaves[slaveIndex].AOmodified[0] || slaves[slaveIndex].AOmodified[1])){
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
void decodeSlaveData(struct slaveData* SlvDataPtr, int slaveIndex){

  slaves[slaveIndex].DigOut = SlvDataPtr->DigOut;

  slaves[slaveIndex].DigIn = SlvDataPtr->DigIn;

  //Atualiza AO. (AO0, AO1, AO2 e etc.)
  for (int j = 0; j < nMaxAO; j++){
    slaves[slaveIndex].AO[j] = SlvDataPtr->AO[j];
  }

  //Atualiza AI. (AI0, AI1, AI2 e etc.)
  for (int j = 0; j < nMaxAI; j++){
    slaves[slaveIndex].AI[j] = SlvDataPtr->AI[j];
  }

}

void printSlavesID(){
  
  Serial_Print(bDebug, "Slaves ID:");
  for (int i = 0; i < (NMAX_SLAVES); i++) {
    if (!(slaves[i].id == 0)){ //Slave ID inicializa com zero
      Serial_Print(bDebug, " - ");
      Serial_Print(bDebug, uint64ToHexStr(slaves[i].id).c_str());
    }
  }
  Serial_Println(bDebug, "\n" );
  
}


#endif
