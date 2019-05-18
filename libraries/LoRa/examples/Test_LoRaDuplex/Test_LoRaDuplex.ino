/*
  LoRa Duplex communication

  Sends a message every half second, and polls continually
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.

  Uses readString() from Stream class to read payload. The Stream class'
  timeout may affect other functuons, like the radio's callback. For an

  created 28 April 2017
  by Tom Igoe
*/
#include <SPI.h>              // include libraries
#include "LoRa.h"
#include <SD.h>
#include <LiquidCrystal_I2C.h>  
LiquidCrystal_I2C lcd(0x3f,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
File dataFile;

boolean SDcard = false;

const int ledSt = A3;          // SDCard
const int relay = 4;          // SDCard
const int chipSelect = 8;          // SDCard
const int csPin = 10;          // LoRa radio chip select
const int resetPin = 9;       // LoRa radio reset
const int irqPin = 2;         // change for your board; must be a hardware interrupt pin
int pin_SDA = A4;
int pin_SCL = A5;

String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

void setup() {
  
  Serial.begin(9600);                   // initialize serial
  while (!Serial);
  
  pinMode(chipSelect, OUTPUT); //ChipSelect
  pinMode(relay, OUTPUT); //Relay
  pinMode(ledSt, OUTPUT); //Relay
  digitalWrite(relay,LOW);
  digitalWrite(ledSt,HIGH);

   lcd.init();   
   delay(100);                 
   lcd.init();

  if(SDcard)
  {
      char filename[] = "datalog.csv";  // cria o Arquivo chamado datalog.csv poderá ser dado o nome que quiser, obs pode ser criado na extencao .txt sem problemas basta mudar .csv para .txt
      delay(100);
      Serial.println(filename); // imprimi o nome do Arquivo via serial 
                     
      if (!SD.begin(chipSelect)) {
        Serial.println("Cartao Falhou, ou nao esta presente");
        return;
      }
          
        
      Serial.println("Cartao Inicializado");
      dataFile = SD.open("datalog.csv", FILE_WRITE);
      dataFile.close(); 
  }
  
  lcd.backlight();
  //lcd.clear();
  delay(100);
  lcd.setCursor(1,0); // COLUNA, LINHA
  lcd.print("AFSmart Lora");
  delay(100);
  lcd.clear();
  
  Serial.println("LoRa Duplex");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
}

void loop() {
    
//    File dataFile = SD.open("datalog.csv", FILE_WRITE);  // abre o Arquivo do SD card para escrita
//    if(dataFile) {
//      
//          dataFile.println("Karlito"); // escreve a temperatura no SDCard
//          dataFile.close();
//          delay(500);         
//          Serial.println("Gravou no Arquivo datalog.csv");
//      }  
//     
//      else {
//        Serial.println("erro ao abrir datalog.csv");
//      }

  
  if (millis() - lastSendTime > interval) {
    String message = "AFEletronica estou aqui ouvindo 100%";   // send a message
    sendMessage(message);
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;    // 2-3 seconds
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  digitalWrite(relay,HIGH);
  digitalWrite(ledSt,LOW);
  delay(200);
  digitalWrite(relay,LOW);
  digitalWrite(ledSt,HIGH);
  
  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message received: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  if(SdCard)
  {
     File dataFile = SD.open("datalog.csv", FILE_WRITE);  // abre o Arquivo do SD card para escrita
     dataFile.println("RSSI: " + String(LoRa.packetRssi())); // escreve a temperatura no SDCard
     dataFile.println("Snr: " + String(LoRa.packetSnr()));; // escreve a temperatura no SDCard
     dataFile.close();
  }

  
   lcd.setCursor(1,0); // COLUNA, LINHA
   //lcd.print("RSSI  ==>");
   lcd.print("RSSI: " + String(LoRa.packetRssi()));
   lcd.setCursor(1,1); // COLUNA, LINHA
   lcd.print("Snr: " + String(LoRa.packetSnr()));
   
  
}

