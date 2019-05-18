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

//Usado para definir o numero de casas decimais para o valor convertido das AI (Zero ou Uma)
#define AIDECIMALS 1

//Intervalo de update do valor analogico (filtrado)
#define ANALOGINTERVAL 500
long lastAnalogUpdt = 0;

//Constante de integração filtro analogico
const float KP = 0.1;

//Valores de Min para conversao das Entradas Analogicas (Ate 8);
uint16_t AIminvar[] = {0, 0, 0, 0, 0, 0, 0, 0};

//Valores de Max para conversao das Entradas Analogicas (Ate 8);
uint16_t AImaxvar[] = {100, 100, 100, 100, 100, 100, 100, 100};

//Inicia Slave
Slave slave(getChipID());

//Variável utilizada para Armazenar o ID do Gateway de interesse.
uint64_t gatewayID;

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
  //Chama a configuração do Watchdog (Tempo de 30s p/ Slave)
  configureWatchdog(30000000);

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

  //Inicializa Digital Input`s Pins
  for (int i = 0; i < nMaxDI; i++) {
    if (pinDigIn[i] >= 0) {
      PEin.pinMode(pinDigIn[i], INPUT);
    }
  }

  timerWrite(timer, 0); //Alimenta WatchDog

  //Inicializa arquivos de configuracao.
  startSlaveFileManager();
  //Carrega as configurações persistentes.
  loadSlaveConfig();
  //Se AP button estiver pressionado, subir AP de configuracao.
  CheckAPInterrupt();
  //Chama a configuração inicial do LoRa
  setupLoRa();

  timerWrite(timer, 0); //Alimenta WatchDog

  //Mostra id do device
  Serial_Println(bDebug, "Slave: ");
  Serial_Println(bDebug, uint64ToHexStr(slave.id).c_str());

  displayMessage(20);
  
}

void loop() {

  //reseta o temporizador (alimenta o watchdog)
  timerWrite(timer, 0); //Alimenta WatchDog

  receive();

  if (millis() - lastAnalogUpdt > ANALOGINTERVAL) {
    //Faz update periodico dos AO`s
    lastAnalogUpdt = millis();
    updateAnalogInput();
  }

  
  /*
    decodeDataMsg("DOFFF0;AO0FFF1;AO1FFF2;");

    Serial_Print(bDebug, "DO: ");
    Serial_Println(bDebug, String(slave.DigOut,HEX));
    Serial_Print(bDebug, "AO0: ");
    Serial_Println(bDebug, String(slave.AO[0],HEX));
    Serial_Print(bDebug, "AO1: ");
    Serial_Println(bDebug, String(slave.AO[1],HEX));*/


  //Se passou o tempo definido em COMMAXTIME desde a ultima comunicação
  if (millis() - slave.lastRecTime > COMMAXTIME) {
    displayMessage(20);
  }

  //Se AP button estiver pressionado, subir AP de configuracao.
  CheckAPInterrupt();

}

void receive(uint64_t idOrigem) {
  int packetSize = LoRa.parsePacket();

  //Verificamos se o pacote tem o tamanho mínimo de bytes que esperamos
  if (packetSize >= (sizeof(struct Header) + sizeof(struct gtwData))) { //recebeu algo
    sprintf (Sbuffer, "onReceive Size: %i bytes", packetSize);
    Serial_Println(bDebug, Sbuffer);

    struct Header *headerPtr = (struct Header*) malloc(sizeof(struct Header));
    LoRa.readBytes((uint8_t *)headerPtr, sizeof(struct Header)); //Le o Header

    Serial_Print(bDebug, "idOrigem: ");
    Serial_Println(bDebug, uint64ToHexStr(headerPtr->idOrigem).c_str());
    

    if (headerPtr->idOrigem == idOrigem) { //Check se mensagem veio do device esperado
      Serial_Println(bDebug, "Passed the idOrigem Check: ");

      struct gtwData *cypherGTWDataPtr = (struct gtwData*) malloc(sizeof(struct gtwData));
      struct gtwData *clearGTWDataPtr;

      LoRa.readBytes((uint8_t *)cypherGTWDataPtr, sizeof(struct gtwData)); //Le o Cypher

      sprintf (Sbuffer, "cypherGTWDataPtr->imsgCounter: %i", cypherGTWDataPtr->imsgCounter);
      Serial_Println(bDebug, Sbuffer);
      sprintf (Sbuffer, "cypherGTWDataPtr->bRstCounter: %i", cypherGTWDataPtr->bRstCounter);
      Serial_Println(bDebug, Sbuffer);
      sprintf (Sbuffer, "cypherGTWDataPtr->iSetDO: %i", cypherGTWDataPtr->iSetDO);
      Serial_Println(bDebug, Sbuffer);

      clearGTWDataPtr = (struct gtwData*) decryptData(cypherGTWDataPtr, sizeof(struct gtwData), aes_key, headerPtr->AES_iv);

      // \/Check para confirmar que mensagem e para este slave.
      if (clearGTWDataPtr->idDestino == slave.id){
        Serial_Println(bDebug, "Passed the idDestino Check: ");

        //Ultimo check para garantir que msg nao e repetida (Evita ataque de repeater)
        if ((clearGTWDataPtr->imsgCounter > slave.imsgCounter) || (clearGTWDataPtr->bRstCounter == true)){ //Confere se mensagem nao e repetida ou se deve zerar contador
          slave.imsgCounter = clearGTWDataPtr->imsgCounter; //Atualiza o counter local. Ele sera utilizado na resposta do Slave.
          
          slave.lastRecTime = millis(); //Reseta temporizador
          
          sprintf (Sbuffer, "clearGTWDataPtr->imsgCounter: %i", clearGTWDataPtr->imsgCounter);
          Serial_Println(bDebug, Sbuffer);
          sprintf (Sbuffer, "clearGTWDataPtr->bRstCounter: %i", clearGTWDataPtr->bRstCounter);
          Serial_Println(bDebug, Sbuffer);
          sprintf (Sbuffer, "clearGTWDataPtr->iSetDO: %s", clearGTWDataPtr->iSetDO);
          Serial_Println(bDebug, Sbuffer);

          if (!clearGTWDataPtr->bEmptyMsg){ //Atualiza output se mensagem recebida pedir.
            decodeDataMsg(clearGTWDataPtr); //Atualizar variaveis de Saidas
            updateOutputPins(); //Atualiza pinos de saida;
          }
  
          updateInputVar();

          slave.packetRssi = LoRa.packetRssi();
          slave.packetSnr = LoRa.packetSnr();
          
          displayMessage(40);
  
          send();
        
        }
        
      }
    
      free(cypherGTWDataPtr);
      cypherGTWDataPtr = NULL;
      free(clearGTWDataPtr);
      clearGTWDataPtr = NULL;
    }
  
    free(headerPtr);
    headerPtr = NULL;
  }

}

void send() {
  //Faz a leitura dos dados
  
  Serial_Println(bDebug, "Criando pacote para envio");
  Serial_Println(bDebug, "\n" );
  
 // !!!! Create Header
  Header header;
  header.idOrigem = slave.id;
  gen_iv(aes_iv); //Gerar um vetor aleatório.
  memcpy(header.AES_iv, aes_iv, sizeof(aes_iv));
 // !!!! \Create Header

 // !!!! Create slaveData
  struct slaveData *clearSlvDataPtr;
  struct slaveData *cypherSlvDataPtr;
  
  clearSlvDataPtr = encodeSendData();
  clearSlvDataPtr->idDestino = gatewayID;
  clearSlvDataPtr->imsgCounter = slave.imsgCounter; //Enviar de volta o número recebido da pelo Gateway

  cypherSlvDataPtr  = (struct slaveData*) encryptData(clearSlvDataPtr, sizeof(struct slaveData), aes_key, header.AES_iv);
 // !!!! \Create slaveData
 
  //Inicializa o pacote
  LoRa.beginPacket();
  //envia o Header
  LoRa.write((uint8_t *)&header, sizeof(struct Header));
  //Envia o GTW data criptografado
  LoRa.write((uint8_t *) cypherSlvDataPtr, sizeof(struct slaveData));
  //Finaliza e envia o pacote
  LoRa.endPacket();

  free(clearSlvDataPtr);
  clearSlvDataPtr = NULL;
  free(cypherSlvDataPtr);
  cypherSlvDataPtr = NULL;
}


//Decode do slave so atualiza Slave Output Variables
void decodeDataMsg(struct gtwData* GTWDecodePtr) {

  slave.DigOut = GTWDecodePtr->iSetDO;
  
  for (int j = 0; j < nMaxAO; j++) {
    slave.AO[j] = GTWDecodePtr->iSetAO[j];
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

      /*if (AIDECIMALS == 0){vReal = ((analogRead(pinAnIn[i]) - AIminvar[i])/((float) AImaxvar[i]))*100;}
      else{vReal = ((analogRead(pinAnIn[i]) - AIminvar[i])/((float) AImaxvar[i]))*100*(pow(10,AIDECIMALS));}*/

      vReal = round(((analogRead(pinAnIn[i]) - AIminvar[i])/((float) AImaxvar[i]))*100*(pow(10,AIDECIMALS)));

      Serial_Print(bDebug, "Analog Pin Value: ");
      sprintf (Sbuffer, "%d", analogRead(pinAnIn[i]));
      Serial_Print(bDebug, Sbuffer);
      Serial_Print(bDebug, " - ");

      Serial_Print(bDebug, "AIminvar[i]: ");
      sprintf (Sbuffer, "%d", AIminvar[i]);
      Serial_Print(bDebug, Sbuffer);
      Serial_Print(bDebug, " - ");

      Serial_Print(bDebug, "AImaxvar[i]: ");
      sprintf (Sbuffer, "%d", AImaxvar[i]);
      Serial_Print(bDebug, Sbuffer);
      Serial_Print(bDebug, " - ");

      Serial_Print(bDebug, "vReal: ");
      sprintf (Sbuffer, "%d", vReal);
      Serial_Print(bDebug, Sbuffer);
      Serial_Print(bDebug, " - ");
      
      if ((vReal-slave.AI[i]) > 0){
        error = ceil((vReal-slave.AI[i]) * KP); //Arredonda para cima
      }else{
        error = floor((vReal-slave.AI[i]) * KP); //Arredonda para baixo
      }
      /*Debugging:*/
      Serial_Print(bDebug, "AO");
      sprintf (Sbuffer, "%d", i);
      Serial_Print(bDebug, Sbuffer);
      Serial_Print(bDebug, ": ");
      sprintf (Sbuffer, "%d", slave.AI[i]);
      Serial_Print(bDebug, Sbuffer);
      Serial_Print(bDebug, " - Error: ");
      sprintf (Sbuffer, "%d", error);
      Serial_Println(bDebug, Sbuffer);
      /*\Debugging:*/
      
      slave.AI[i] = slave.AI[i] + error;
    }
  }
}

/*  data= Struct slaveData
*/
struct slaveData* encodeSendData() {
  struct slaveData *slaveEncodePtr = (struct slaveData*) malloc(sizeof(struct slaveData));


  for (int i = nMaxAI - 1 ; i >= 0 ; i--) {
    slaveEncodePtr->AI[i] = slave.AI[i];
  }

  for (int i = nMaxAO - 1 ; i >= 0 ; i--) {
    slaveEncodePtr->AO[i] = slave.AO[i];
  }

  slaveEncodePtr->DigIn = slave.DigIn;

  slaveEncodePtr->DigOut = slave.DigOut;

  return slaveEncodePtr;
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
