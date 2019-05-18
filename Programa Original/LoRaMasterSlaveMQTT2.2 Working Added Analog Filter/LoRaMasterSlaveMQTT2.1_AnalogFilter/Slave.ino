//Compila apenas se MASTER não estiver definido no arquivo principal
#ifndef MASTER

//Lib Conversão Jason
//#include <ArduinoJson.h>

//Matriz de Digital Output Pins (Ate 16);
int pinDigOut[] = {6, 5, 4, 3, 2, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

//int pinDigOuttst[]={2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1};

//Matriz de Digital Input Pins (Ate 16);
int pinDigIn[] = {3, 2, 1, 0, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1};

//Matriz de Analog Output Pins (Ate 8);
int pinAnOut[] = {-1, -1, -1, -1, -1, -1, -1, -1};

//Matriz de Analog Input Pins (Ate 8);
int pinAnIn[] = {39, 36, -1, -1, -1, -1, -1, -1};

//Usado para ver se o looping esta rodando.
#define INTERVAL 500

//Intervalo de update do valor analogico (filtrado)
#define ANALOGINTERVAL 500
long lastAnalogUpdt = 0;

//Inicia Slave
Slave slave(getChipID());

//Constante de integração filtro analogico
const float KP = 0.1;

//Tempo do último Check (Debug)
long lastCheckTime = 0;

//Define Numero maximo de DO pins
int nMaxDO;
//Define Numero maximo de DI pins
int nMaxDI;
//Define Numero maximo de AO pins
int nMaxAO;
//Define Numero maximo de AI pins
int nMaxAI;

//variavel botao1
bool bbutton1 = false;

void setup() {

  Serial.begin(115200);
  //Chama a configuração do Watchdog (Tempo de 10s p/ Slave)
  configureWatchdog(10000000);

  //Inicializa input/output
  PEout.init();
  PEin.init();

  timerWrite(timer, 0); //Alimenta WatchDog
    
  //Chama a configuração inicial do display
  setupDisplay();

  timerWrite(timer, 0); //Alimenta WatchDog
    
  displayMessage(10);

  //Define Numero maximo de DO pins.
  nMaxDO = (sizeof(pinDigOut) / sizeof(pinDigOut[0]));
  //Define Numero maximo de DI pins.
  nMaxDI = (sizeof(pinDigIn) / sizeof(pinDigIn[0]));
  //Define Numero maximo de AO pins
  nMaxAO = sizeof(slave.AO) / sizeof(slave.AO[0]);
  //Define Numero maximo de AI pins
  nMaxAI = sizeof(slave.AI) / sizeof(slave.AI[0]);

 /* Com o port expander nao precisa.
  //Inicializa Digital Output Pins
  for (int i = 0; i < nMaxDO; i++) {
    if (pinDigOut[i] >= 0) {
      pinMode(pinDigOut[i], OUTPUT);
    }
  }*/

  //Inicializa Digital Input`s Pins
  for (int i = 0; i < nMaxDI; i++) {
    if (pinDigIn[i] >= 0) {
      PEin.pinMode(pinDigIn[i], INPUT);
    }
  }

  timerWrite(timer, 0); //Alimenta WatchDog
  //Chama a configuração inicial do LoRa
  setupLoRa();
  
  //inicia Rotina de criptografia
  aes_init();

  timerWrite(timer, 0); //Alimenta WatchDog

  //Mostra id do device
  Serial.println("Slave: " + uint64ToHexStr(slave.id));

  displayMessage(20);
  
}

void loop() {

  //reseta o temporizador (alimenta o watchdog)
  timerWrite(timer, 0); //Alimenta WatchDog

  /*
    if (millis() - lastCheckTime > INTERVAL) {
      //Marcamos o tempo para ver se o looping esta rodando (Debug)
      lastCheckTime = millis();
      Serial.println("Looping rodando...");
    }*/

  receive();

  if (millis() - lastAnalogUpdt > ANALOGINTERVAL) {
    //Faz update periodico dos AO`s
    lastAnalogUpdt = millis();
    updateAnalogInput();
  }

  
  /*
    decodeDataMsg("DOFFF0;AO0FFF1;AO1FFF2;");

    Serial.print("DO: ");
    Serial.println(String(slave.DigOut,HEX));
    Serial.print("AO0: ");
    Serial.println(String(slave.AO[0],HEX));
    Serial.print("AO1: ");
    Serial.println(String(slave.AO[1],HEX));*/


  //Se passou o tempo definido em COMMAXTIME desde a ultima comunicação
  if (millis() - slave.lastRecTime > COMMAXTIME) {
    displayMessage(30);
  }

}

void receive() {
  //Tenta ler o pacote
  int packetSize = LoRa.parsePacket();
  //Serial.println("Packet Size: " + String(packetSize));
  yield();

  //Verificamos se o pacote tem o tamanho mínimo de caracteres que esperamos
  if (packetSize > GETDATA.length()) {
    String received = "";
    //Armazena os dados do pacote em uma string
    while (LoRa.available()) {
      received += (char) LoRa.read();
    }

    //Verifica se a string possui o que está contido em "GETDATA"
    int index = received.indexOf(GETDATA);
    if (index >= 0) {
      //Recuperamos a string que está após o "GETDATA",
      //que no caso serão os dados de nosso interesse criptografados
      String data = received.substring(GETDATA.length());

      Serial.println("Recebeu algo:");
      Serial.println(data);
      Serial.println();

      //Decriptografa mensagem e verifica se ela é para este device.
      //Retorna "" se a mensagem nao for para este device.
      if (bEncrypt) data = decryptMessage(data, uint64ToHexStr(slave.id));


      //Verifica se mensagem foi decriptofrada com sucesso para este device
      if (data.length() > 0) {
        slave.lastRecTime = millis(); //Reseta temporizador

        Serial.print("Recebeu: ");
        Serial.println(data);
        Serial.println();

        decodeDataMsg(data); //Atualizar variaveis de Saidas
        updateOutputPins(); //Atualiza pinos de saida;

        updateInputVar();
        
        displayMessage(40);

        send();

      }
    }
  }

}

void send() {
  //Faz a leitura dos dados
  
  Serial.println("Criando pacote para envio");
  Serial.println();

  // !!!! Cria data !!!!
  String data;
  data = encodeSendData();
  // \!!!! Cria data !!!!

  Serial.print("data enviado: ");
  Serial.println(data);
  
  //Criptografa mensagem com endereco deste Device.
  String slaveID = uint64ToHexStr(slave.id);
  //Linha necessaria por conta de bug na biblioteca AES. Garante que a cryptografia vai funcionar
  if (bEncrypt) encryptMessage(data, slaveID);
  if (bEncrypt) data = encryptMessage(data, slaveID);

  //Cria o pacote para envio
  LoRa.beginPacket();
  LoRa.print(SETDATA + data);
  //Finaliza e envia o pacote
  LoRa.endPacket();

  //Mostra no display
  /*  display.fillScreen(ILI9341_BLACK);
    display.setCursor(0, 0);
    display.println("Enviou: " + String(data));
    display.setCursor(0, 50);
    display.println("RSSI: " + String(LoRa.packetRssi()));
    
  */
  Serial.print("Mensagem Enviada: ");
  Serial.println(data);
  Serial.println();
}

//Decode do slave so atualiza Slave Output Variables
void decodeDataMsg(String msgData) {
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
      slave.DigOut = strtoul(tempStr.c_str(), NULL, 16); //Converte de volta para HEX
      continue;
    }

    for (int j = 0; j < nMaxAO; j++) {
      objStr = "AO" + String(j, DEC);
      if (tempStr.indexOf(objStr) >= 0) { //Contains the AOj part
        tempStr = tempStr.substring(objStr.length());
        slave.AO[j] = strtoul(tempStr.c_str(), NULL, 16); //Converte de volta para HEX
        continue;
      }
    }
  }
}

//Atualiza Slave Input Variables (DI's e AI's)
void updateInputVar() {

  //Atualiza slave DI variables
  uint16_t ntemp = 0;
  for (int i = 0; i < nMaxDI; i++) {
    if (pinDigIn[i] >= 0) {
      //ntemp |= ((1UL << i) | PEin.digitalRead(pinDigIn[i])); //Seta, se pino estiver HIGH
      ntemp |= (!PEin.digitalRead(pinDigIn[i]) << i); //Seta, se pino estiver HIGH
    }
  }

  slave.DigIn = ntemp;

  updateAnalogInput();

}

void updateAnalogInput() {
  int vReal = 0;
  int error = 0;
  
  //Atualiza slave AI variables
  for (int i = 0; i < nMaxAI; i++) {
    if (pinAnIn[i] >= 0) {
      vReal = analogRead(pinAnIn[i]);

      if ((vReal-slave.AI[i]) > 0){
        error = ceil((vReal-slave.AI[i]) * KP); //Arredonda para cima
      }else{
        error = floor((vReal-slave.AI[i]) * KP); //Arredonda para baixo
      }
      /*Debugging:*/
      Serial.print("AO");
      Serial.print(String(i));
      Serial.print(": ");
      Serial.println(String(slave.AI[i]));
      Serial.print("Error: ");
      Serial.println(String(error));
      slave.AI[i] = slave.AI[i] + error;
    }
  }
}

/*  data= "DO" + hex(DO) + ";" + "DI" + hex(DI) + ";" + "AO0" + hex(AO0) + ";" + "AO1" + hex(AO1) + ";" +
    + "AI0" + hex(AI0) + ";" + "AI1" + hex(AI1) + ";"
   Exemplo:
   DOFFFF;DIFFFF;AO0FFFF;AO1FFFF;AI0FFFF;AI1FFFF;
   Envia sempre todos os dados ao Master;
*/
String encodeSendData() {
  String msgData;

  msgData = ";" + msgData;

  for (int i = nMaxAI - 1 ; i >= 0 ; i--) {
    msgData = String(slave.AI[i], HEX) + msgData;
    msgData = "AI" + String(i, DEC) + msgData;
    msgData = ";" + msgData;
  }

  for (int i = nMaxAO - 1 ; i >= 0 ; i--) {
    msgData = String(slave.AO[i], HEX) + msgData;
    msgData = "AO" + String(i, DEC) + msgData;
    msgData = ";" + msgData;
  }
  
  msgData = ";" + msgData;
  msgData = String(slave.DigIn, HEX) + msgData;
  msgData = "DI" + msgData;
  msgData = ";" + msgData;
  msgData = String(slave.DigOut, HEX) + msgData;
  msgData = "DO" + msgData;

  return msgData;
}

/*FIX ME - Falta implementar atualizacao do AO.
   Atualmente so tem saidas digitais implementada.
*/
void updateOutputPins() {

  uint32_t dOut = slave.DigOut;

  //Atualiza DO
  for (int i = 0; i < nMaxDO; i++) {
    if (pinDigOut[i] >= 0) {
      PEout.digitalWrite(pinDigOut[i], !slave.getDO(i)); //Esta invertido na placa 0= liga, 1 desliga;
    }
  }

  //FIX ME - Atualiza AO
  for (int i = 0; i < nMaxAO; i++) {
    if (pinAnOut[i] >= 0) {
      //AnalogWrite
    }
  }

}


#endif
