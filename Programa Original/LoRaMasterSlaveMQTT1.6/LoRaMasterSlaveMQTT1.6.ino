#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
//#include <SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <Slave.h>

#include <WiFi.h>
#include <WebSocketsServer.h>

//Lib Conversão Json
#include <ArduinoJson.h>

//Lib manipulacao de Arquivos
#include "FS.h"
#include "SPIFFS.h"

//Deixe esta linha descomentada para compilar o Master
//Comente ou remova para compilar o Slave
#define MASTER

//Define numero max de DO`s e DI`s - Padrao = 16 de cada
//#define NMAX_IO 16

//Define se vai criptografar a mensagem.
bool bEncrypt = true;

// !!!! Configura Display !!!!
//#define SCK 5   // GPIO5  SCK
//#define MISO 19 // GPIO19 MISO
//#define MOSI 27 // GPIO27 MOSI
//#define SS 18   // GPIO18 CS
//#define RST 14  // GPIO14 RESET
//#define DI00 26 // GPIO26 IRQ(Interrupt Request)
#define OLED_SDA 21
#define OLED_SCL 22

//Variável para controlar o display
//SSD1306 display(0x3c, 4, 15);
Adafruit_SH1106 display(21, 22);
// !!!! \Configura Display !!!!

//const int ledSt = 2;   //Pisca quando recebe msg       
const int relay = 33;  
//const int btn = 4; //Botão original da placa

// !!!! Configura Lora !!!!
#define BAND 915E6 //Frequência do radio - exemplo : 433E6, 868E6, 915E6

//Constante para informar ao Slave que queremos os dados
const String GETDATA = "g=";
//Constante que o Slave retorna junto com os dados para o Master
const String SETDATA = "s=";

// Limear de tempo para estabelecer erro de comunicação
#define COMMAXTIME 10000

//SE Módulo com  ESP32
#define chipSelect 27
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
  
  Serial.println("Watchdog Ativo!");
}

void setupDisplay(){
//  //O estado do GPIO16 é utilizado para controlar o display OLED
//  pinMode(16, OUTPUT);
//  //Reseta as configurações do display OLED
//  digitalWrite(16, LOW);
//  //Para o OLED permanecer ligado, o GPIO16 deve permanecer HIGH
//  //Deve estar em HIGH antes de chamar o display.init() e fazer as demais configurações,
//  //não inverta a ordem
//  digitalWrite(16, HIGH);
//
//  //Configurações do display
//  display.init();
//  display.flipScreenVertically();
//  display.setFont(ArialMT_Plain_10);
//  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.begin(SH1106_SWITCHCAPVCC, 0x3C); 
  display.clearDisplay();

//Configurações do display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Inicializando...");
  display.setTextSize(1);
  display.setCursor(0,30);
  display.println("PIA - IoT Solutions");
  display.display();
  delay(2000);
  display.clearDisplay();
}

//Configurações iniciais do LoRa
void setupLoRa(){ 
//  //Inicializa a comunicação
//  SPI.begin(SCK, MISO, MOSI, SS);
//  LoRa.setPins(SS, RST, DI00);
//
//  //Inicializa o LoRa
//  if (!LoRa.begin(BAND, true)){
//    //Se não conseguiu inicializar, mostra uma mensagem no display
//    display.clear();
//    display.drawString(0, 0, "Erro ao inicializar o LoRa!");
//    display.display();
//    while (1);
//  }
//
//  //Ativa o crc
//  LoRa.enableCrc();
//  //Ativa o recebimento de pacotes
//  LoRa.receive();


  //Inicializa a comunicação
  pinMode(chipSelect, OUTPUT); //ChipSelect
  pinMode(relay, OUTPUT); //Relay
  //pinMode(ledSt, OUTPUT); //ledSt blinking LED
  digitalWrite(relay,HIGH);
  //digitalWrite(ledSt,HIGH);
  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin


  //Inicializa o LoRa
  if (!LoRa.begin(BAND)) {             // initialize ratio at 915 MHz
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Erro ao inicializar o LoRa!");
    display.display();
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");

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
