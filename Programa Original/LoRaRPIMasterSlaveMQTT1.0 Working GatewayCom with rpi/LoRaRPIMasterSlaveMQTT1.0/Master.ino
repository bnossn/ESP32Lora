//Compila apenas se MASTER estiver definido no arquivo principal
#ifdef MASTER

//Lib de MQTT
#include <PubSubClient.h>

//Reter Mensagens MQTT
#define MQTTRETAINED false

/*Callback function header
   The callback function header needs to be declared
   before the PubSubClient constructor and the
   actual callback defined afterwards.
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
long lastSentTime = 0;

bool firstMsg;

//usado para identificar quando rpi terminou de enviar um comando.
boolean bnewCommand = false;

//buffer para o comando via serial.
const byte numChars = 255;
char recvCommand[numChars];

int loopIndex = 0;

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
  //startGatewayFileManager();
  //Carrega as configurações persistentes.
  //loadGatewayConfig();
  //inicia Rotina de criptografia
  aes_init();
  //Define Numero maximo de DO / DI
  nMaxIO = sizeof(slaves[0].DigOut) * 8;
  //Define Numero maximo de AO
  nMaxAO = sizeof(slaves[0].AO) / sizeof(slaves[0].AO[0]);
  //Define Numero maximo de AI
  nMaxAI = sizeof(slaves[0].AI) / sizeof(slaves[0].AI[0]);

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

  displayMessage(40);

  displayMessage(50);

}

void loop() {

  //reseta o temporizador (alimenta o watchdog)
  timerWrite(timer, 0);

  //Read Slave ID from serial command
  recvfromSerial();


  if (bnewCommand == true) {
    bnewCommand = false;

    printSlavesID();

    char *token;
    token = strtok(recvCommand, ":");

    if (strcasecmp(token, "GetDataFrom") == 0) {
      uint8_t slaveIndex;
      char *strSlaveID;
      strSlaveID = strtok(NULL, ":");
      uint64_t iSlaveID;
      iSlaveID = strtoull(strSlaveID, NULL, 16);

      //If not, register and send to the new index registered.
      slaveIndex = 0; //Vai guardar o index do slave de interesse.
      bool bSlaveFound = false;
      for (int i = 0; i < NMAX_SLAVES; i++) { //Checa se slave ja esta registrado.
        if (slaves[i].id == iSlaveID) {
          bSlaveFound = true;
          break; //Encontrou o Slave de interesse
        }
        slaveIndex++;
      }


      if (!bSlaveFound) { //Se slave nao esta registrado, registra-lo
      slaveIndex = 0;
        for (int i = 0; i < NMAX_SLAVES; i++) {
          if (slaves[i].id == 0) {
            slaves[i].id = iSlaveID;
            break;
          }
          slaveIndex++;
        }
      }
     
     if (slaveIndex >= NMAX_SLAVES) slaveIndex = 0; //Garante que o rpi nao vai registrar mais que NMAX_SLAVES slaves.
      
      //Envia o pacote para informar ao Slave que queremos receber os dados
      send(slaveIndex);

      //abre janela de recebimento por 3 segundos ou ate receber.
      bool bSlaveReplied = false; //Usado para identificar se slave respondeu.
      long tempTimer = millis();
      while ((millis() - tempTimer) < RECEIVE_WINDOW) {
        //Verificamos se há pacotes para recebermos
        if (receive(slaveIndex)) {
          timerWrite(timer, 0); //alimenta o watchdog
          if (!bDebug) publishDataOnSerial(slaveIndex);
          bSlaveReplied = true;
          break;
        }
      }

      Serial_Print(bDebug, "Janela recebimento aberta por: ");
      sprintf (Sbuffer, "%lu", (millis() - tempTimer));
      Serial_Println(bDebug, Sbuffer);
      Serial_Println(bDebug, "\n" );

      if (!bSlaveReplied) Serial.println("No Answer From Slave: " + uint64ToHexStr(slaves[slaveIndex].id));

      printSlavesID();
    }

  }




  //Se AP button estiver pressionado, subir AP de configuracao.
  //CheckAPInterrupt();

}

/* Mensagem enviada tem o formato:
  "GETDATA + (TamanhoMsg & slaveID & initialization vector & data criptografado)"
  Parte em parenteses é montada dentro de encryptMessage()
  data=
  "DO" + hex(DO) + ";" + "AO0" + hex(AO0) + ";" + "AO1" + hex(AO1) + ";"
  Exemplo:
  DOFFFF;AO0FFFF;AO1FFFF;
  Só envia o output que foi alterado pelo usuário
  Se nenhum output foi alterado, envia "empty"
*/
void send(int slaveIndex) {

  // !!!! Cria data !!!!
  String data;
  data = encodeSendData(slaveIndex);
  // \!!!! Cria data !!!!
  if (!slaves[slaveIndex].bmodSent && !data.equalsIgnoreCase("empty")) {
    slaves[slaveIndex].bmodSent = true; //Se modificacao nao foi enviada, transforma-la em enviada.
  }

  Serial_Println(bDebug, "Mensagem Enviada:");
  Serial_Print(bDebug, "Data: ");
  Serial_Println(bDebug, data.c_str());

  //Criptografa mensagem com endereco do Slave.
  String slaveID = uint64ToHexStr(slaves[slaveIndex].id);
  //Linha necessaria por conta de bug na biblioteca AES. Garante que a cryptografia vai funcionar
  if (bEncrypt) encryptMessage(data, slaveID);
  if (bEncrypt) data = encryptMessage(data, slaveID);

  //Inicializa o pacote
  LoRa.beginPacket();
  //Envia o que está contido em "GETDATA" + payload
  LoRa.print(GETDATA + data);
  //Finaliza e envia o pacote
  LoRa.endPacket();


  Serial_Print(bDebug, "Encrypted: ");
  Serial_Print(bDebug, GETDATA.c_str());
  Serial_Println(bDebug, data.c_str());
}

//Retornar verdadeiro se recebemos o pacote do slave esperado.
bool receive(int slaveIndex) {
  //Tentamos ler o pacote
  int packetSize = LoRa.parsePacket();

  //Verificamos se o pacote tem o tamanho mínimo de caracteres que esperamos
  if (packetSize > SETDATA.length()) {
    Serial_Println(bDebug, "Mensagem Recebida");
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
        slaves[slaveIndex].isOnline = true; //Slave esta respondendo

        SlavesRSSI[slaveIndex] = LoRa.packetRssi(); //Registra força do sinal.
        SlavesSNR[slaveIndex] = LoRa.packetSnr(); //Registra Signal to Noise ratio

        //Reseta temporizador - tempo desde ultima troca de dados
        slaves[slaveIndex].lastRecTime = millis();

        Serial_Print(bDebug, "Data: ");
        Serial_Println(bDebug, data.c_str());

        decodeDataMsg(data, slaveIndex);


        if (slaves[slaveIndex].bmodSent) { //So reseta booleanas se a modificacao foi enviada ao Slave.
          //Uma vez que recebeu, os Outputs estão atualizados - Resetar modified bool.
          slaves[slaveIndex].DigOutmodified = false;

          for (int i = 0; i < nMaxAO; i++ ) { //Resetar modified bool.
            slaves[slaveIndex].AOmodified[i] = false;
          }
        }

        //Tempo que demorou para o Master criar o pacote, enviar o pacote,
        //o Slave receber, fazer a leitura, criar um novo pacote, enviá-lo
        //e o Master receber e ler e tambem publicar no mqtt
        //String waiting = String(millis() - slaves[slaveIndex].lastSentTime);

        return true;
      }
    }
  }
  return false;
}

void recvfromSerial() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  //char startMarker = '';
  char endMarker = '\n';
  char rc;

  while (Serial.available() > 0 && bnewCommand == false) { // <<== NEW - get all bytes from buffer
    rc = Serial.read();

    if (rc != endMarker) {
      recvCommand[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      recvCommand[ndx] = '\0'; // terminate the string
      ndx = 0;
      bnewCommand = true;
    }
  }
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
  while ((WiFi.status() != WL_CONNECTED) && ((millis() - wifiTimer) < 15000)) {
    timerWrite(timer, 0); //Alimenta WatchDog
    delay(500);
    Serial_Print(bDebug, ".");
  }

  Serial_Println(bDebug, "\n" );
}

/* data= "DO" + hex(DO) + ";" + "AO0" + hex(AO0) + ";" + "AO1" + hex(AO1) + ";"
   Exemplo:
   DOFFFF;AO0FFFF;AO1FFFF;
   Só envia o output que foi alterado pelo usuário
   Se nenhum output foi alterado, envia "empty"
*/
String encodeSendData(int slaveIndex) {
  String msgData;

  // Se nao houve alteracao no output, enviar "empty"
  if (!(slaves[slaveIndex].DigOutmodified ||
        slaves[slaveIndex].AOmodified[0] || slaves[slaveIndex].AOmodified[1])) {
    msgData = "empty";
  } else {

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
void decodeDataMsg(String msgData, int slaveIndex) {
  int index;
  String tempStr;
  String objStr;

  while (msgData.indexOf(";") >= 0) {
    index = msgData.indexOf(";");
    tempStr = msgData.substring(0, index); //Take a part of the data
    msgData = msgData.substring(index + 1); //Keep the rest

    if (tempStr.indexOf("DO") >= 0) { //Contains the DO part
      objStr = "DO";
      tempStr = tempStr.substring(objStr.length());
      slaves[slaveIndex].DigOut = strtoul(tempStr.c_str(), NULL, 16); //Converte de volta para HEX
      continue;
    }

    if (tempStr.indexOf("DI") >= 0) { //Contains the DI part
      objStr = "DI";
      tempStr = tempStr.substring(objStr.length());
      slaves[slaveIndex].DigIn = strtoul(tempStr.c_str(), NULL, 16); //Converte de volta para HEX
      continue;
    }


    //Atualiza AO. (AO0, AO1, AO2 e etc.)
    for (int j = 0; j < nMaxAO; j++) {
      objStr = "AO" + String(j, DEC);
      if (tempStr.indexOf(objStr) >= 0) { //Contains the AOj part
        tempStr = tempStr.substring(objStr.length());
        slaves[slaveIndex].AO[j] = strtoul(tempStr.c_str(), NULL, 16); //Converte de volta para HEX
        continue;
      }
    }

    //Atualiza AI. (AI0, AI1, AI2 e etc.)
    for (int j = 0; j < nMaxAI; j++) {
      objStr = "AI" + String(j, DEC);
      if (tempStr.indexOf(objStr) >= 0) { //Contains the AIj part
        tempStr = tempStr.substring(objStr.length());
        slaves[slaveIndex].AI[j] = strtoul(tempStr.c_str(), NULL, 16); //Converte de volta para HEX
        continue;
      }
    }
  }
}

void printSlavesID() {

  Serial_Print(bDebug, "Slaves ID:");
  for (int i = 0; i < (NMAX_SLAVES); i++) {
    if (!(slaves[i].id == 0)) { //Slave ID inicializa com zero
      Serial_Print(bDebug, " - ");
      Serial_Print(bDebug, uint64ToHexStr(slaves[i].id).c_str());
    }
  }
  Serial_Println(bDebug, "\n" );

}


#endif
