#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
//#include <SSD1306.h>

//Deixe esta linha descomentada para compilar o Master
//Comente ou remova para compilar o Slave
//#define MASTER

//Config Input/Output
#include <PortExpander_I2C.h>
PortExpander_I2C PEin(0x26);
PortExpander_I2C PEout(0x27);

//#include <Adafruit_SH1106.h>
#include <Slave.h>

#include <WiFi.h>
#include <WebSocketsServer.h>

//Lib Conversão Json
#include <ArduinoJson.h>

//Lib manipulacao de Arquivos
#include "FS.h"
#include "SPIFFS.h"

//Ligar debug via RX/TX
const bool bDebug = true;
char Sbuffer[256];

//Define se vai criptografar a mensagem.
bool bEncrypt = true;

// !!!! Access Point Configuration
//Define o Botao que sobe o Access Point
const int APPin = 7;
//Intervalo que o Botao deve ser pressionado para ativar Access Point
#define ACCESSPOINT_TIME 3000
// !!!! \Access Point Configuration

//Buffers de Strings.
char reg1buffer[256];

// !!!! Configura Display !!!!
#include <Adafruit_GFX.h>
#include "Adafruit_ILI9341.h"
#define display_DC 2
#define display_CS 4
Adafruit_ILI9341 display = Adafruit_ILI9341(display_CS, display_DC);
// !!!! \Configura Display !!!!

//const int ledSt = 2;   //Pisca quando recebe msg       
//const int relay = 33;  
//const int btn = 4; //Botão original da placa

// !!!! Configura Lora !!!!
#define BAND 915E6 //Frequência do radio - exemplo : 433E6, 868E6, 915E6
#define SPREADINGFACTOR 9 //Supported values are between 6 and 12. defaults to 7
//#define SPREADINGFACTOR 7 //Supported values are between 6 and 12. defaults to 7
#define SIGNALBW 250E3 //Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, and 250E3.  defaults to 125E3.
#define PREAMBLE_LENGTH 8 //Supported values are between 6 and 65535. defaults to 8
#define TXPOWER 20 //Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN. TX power in dB, defaults to 17.
// !!!! \Configura Lora !!!!

//Constante para informar ao Slave que queremos os dados
const String GETDATA = "g=";
//Constante que o Slave retorna junto com os dados para o Master
const String SETDATA = "s=";

// Limear de tempo para estabelecer erro de comunicação
#define COMMAXTIME (3*60) * 1000

//SE Módulo com  ESP32
//#define chipSelect 27
#define chipSelect 25
const int csPin = 5;          // LoRa radio chip select
const int resetPin = 16;       // LoRa radio reset
const int irqPin = 17;         // change for your board; must be a hardware interrupt pin
// !!!! \Configura Lora !!!!


hw_timer_t *timer = NULL; // (WatchDog) faz o controle do temporizador (interrupção por tempo)

//(WatchDog)função que o temporizador irá chamar, para reiniciar o ESP32
void IRAM_ATTR resetModule(){
    ets_printf("(watchdog) reiniciar\n"); //imprime no log
    esp_restart_noos(); //reinicia o chip
}

//(WatchDog)função que configura o temporizador (usTime = Tempo em microsegundos)
void configureWatchdog(int usTime){
    //hw_timer_t * timerBegin(uint8_t num, uint16_t divider, bool countUp)
    /*
       num: é a ordem do temporizador. Podemos ter quatro temporizadores, então a ordem pode ser [0,1,2,3].
      divider: É um prescaler (reduz a frequencia por fator). Para fazer um agendador de um segundo, 
      usaremos o divider como 80 (clock principal do ESP32 é 80MHz). Cada instante será T = 1/(80) = 1us
      countUp: True o contador será progressivo
    */
  timer = timerBegin(0, 80, true); //timerID 0, div 80
  //timer, callback, interrupção de borda
  timerAttachInterrupt(timer, &resetModule, true);
  //timer, tempo (us), repetição (10s)
  timerAlarmWrite(timer, usTime, true);
  timerAlarmEnable(timer); //habilita a interrupção //enable interrupt
  
  Serial_Println(bDebug, "Watchdog Ativo!");
}

//Configurações iniciais do LoRa
void setupLoRa(){ 

  //Inicializa a comunicação
  pinMode(chipSelect, OUTPUT); //ChipSelect
  //pinMode(relay, OUTPUT); //Relay
  //pinMode(ledSt, OUTPUT); //ledSt blinking LED
  //digitalWrite(relay,HIGH);
  //digitalWrite(ledSt,HIGH);
  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  //Inicializa o LoRa
  if (!LoRa.begin(BAND)) {             // initialize ratio at 915 MHz
    Serial_Println(bDebug, "Erro ao inicializar o LoRa!");
    displayMessage(100);
    while (true);                       // if failed, do nothing
  }

  LoRa.setTxPower(TXPOWER);
  LoRa.setSpreadingFactor(SPREADINGFACTOR);
  LoRa.setSignalBandwidth(SIGNALBW);
  LoRa.setPreambleLength(PREAMBLE_LENGTH);
  
  Serial_Println(bDebug, "LoRa init succeeded.");

  //Ativa o crc
  LoRa.enableCrc();
  //Ativa o recebimento de pacotes
  LoRa.receive();  
}

uint64_t getChipID(){ //return current chip`s ID.
  return ESP.getEfuseMac();
}

String uint64ToHexStr(uint64_t value){
  uint16_t temp1 = (uint16_t)(value>>32); //High 2 bytes
  uint32_t temp2 = (uint32_t) value; //print Low 4bytes.

  return String(temp1, HEX) + String(temp2, HEX);
}


void Serial_Println(bool bshow, const void *message){
  if (bshow){
    Serial.println((const char*) message);
  }
}

void Serial_Print(bool bshow, const void *message){
  if (bshow){
    Serial.print((const char*) message);
  }
}
