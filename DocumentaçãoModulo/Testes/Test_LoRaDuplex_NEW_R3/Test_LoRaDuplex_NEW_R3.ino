/*
 * Teste AFSmart ESP32_EthLora V1.0
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
//#include <SD.h>
//#include <mySD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
//#include <LiquidCrystal_I2C.h>  
//LiquidCrystal_I2C lcd(0x3f,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
//File dataFile;
#define OLED_SDA 21
#define OLED_SCL 22

boolean SDcard = false;       //para usar o cartão mude para true, o cartõa precisa estar inserido ao módudo
 
const int ledSt = 2;          
const int relay = 33;   

// Initialize the OLED display using Wire library
//SSD1306  display(0x3C, 21, 22);
Adafruit_SH1106 display(21, 22);

//SE Módulo com ARDUINO PRO-MINI
//const int csPin = 10;          // LoRa radio chip select
//const int resetPin = 9;       // LoRa radio reset
//const int irqPin = 2;         // change for your board; must be a hardware interrupt pin

//SE Módulo com  ESP32
#define chipSelect 25
const int csPin = 5;          // LoRa radio chip select
const int resetPin = 16;       // LoRa radio reset
const int irqPin = 17;         // change for your board; must be a hardware interrupt pin


String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xFF;     // address of this device
byte destination = 0xBB;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

void setup() {
  
  Serial.begin(115200);                   // initialize serial
  while (!Serial);

  display.begin(SH1106_SWITCHCAPVCC, 0x3C); 
  display.clearDisplay();

 
  pinMode(chipSelect, OUTPUT); //ChipSelect
  pinMode(relay, OUTPUT); //Relay
  pinMode(ledSt, OUTPUT); //ledSt 
  digitalWrite(relay,HIGH);
  digitalWrite(ledSt,LOW);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("AFSMART-RADIO-ESP32");
  display.setTextSize(1);
   display.setCursor(0,30);
  display.println("AFELETRONICA");
  display.display();
  delay(2000);
  display.clearDisplay();

  // lcd.init();   
   //delay(100);                 
   //lcd.init();

 /*if(SDcard)
  {
          Serial.print("Initializing SD card...");
             initialize SD library with SPI pins 
            if (!SD.begin(27, 23, 19, 18)) {
              Serial.println("initialization failed!");
              return;
            }
            Serial.println("initialization done.");
             open "test.txt" for writing 
              dataFile = SD.open("FileAFE.txt", FILE_WRITE);
               if open succesfully -> root != NULL 
                then write string "Hello world!" to it
              
              if (dataFile) {
                dataFile.println("PIA - IOT SOLUTION R0.0!");
                dataFile.flush();
                close the file 
                dataFile.close();
              } else {
                 if the file open error, print an error 
                Serial.println("error opening FileAFE.txt");
              }
              delay(1000);
      }*/
            
        
     // Serial.println("Cartao Inicializado");
      //dataFile = SD.open("datalog.csv", FILE_WRITE);
      //dataFile.close(); 
 // }
  
  //lcd.backlight();
  //lcd.clear();
  //delay(1);
  //lcd.setCursor(0,0); // COLUNA, LINHA
 // lcd.print("    AFSmart     ");
  //lcd.setCursor(0,1); // COLUNA, LINHA
  //lcd.print("   Lora ESP32   ");
  //delay(1000);
 // lcd.clear();
  Serial.println("LoRa Duplex");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    display.setTextSize(1);
    display.setCursor(0,35);
    display.println("Radio init failed");
    display.display();
    
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
    display.setTextSize(1);
    display.setCursor(0,35);
    display.println("Radio init succeeded");
    display.display();

}

void loop() {
     
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

  digitalWrite(relay,LOW);
  digitalWrite(ledSt,LOW);
  delay(200);
 digitalWrite(relay,HIGH);
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

    
   //lcd.setCursor(1,0); // COLUNA, LINHA
   //lcd.print("RSSI  ==>");
   //lcd.print("RSSI: " + String(LoRa.packetRssi()));
   //lcd.setCursor(1,1); // COLUNA, LINHA
   //lcd.print("Snr: " + String(LoRa.packetSnr()));

//  display.setColor(WHITE);
//  display.setTextAlignment(TEXT_ALIGN_CENTER);
//  //display.drawString(64, 15, String(LoRa.packetSnr()));
//  display.drawString(64, 15, String("AFELETRONICA"));
//  display.setFont(ArialMT_Plain_10);
//
//  display.display();
 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("AFSMART-RADIO-ESP32");
  display.setCursor(0,10);
  display.println("RSSI: " + String(LoRa.packetRssi()));
  display.setCursor(0,20);
  display.println("Snr: " + String(LoRa.packetSnr()));
  display.setCursor(0,30);
  display.println("Message ID: " + String(incomingMsgId));
  display.setCursor(0,40);
  display.println("Message length: " + String(incomingLength));
  display.setCursor(0,55);
  display.println("AFELETRONICA.COM.BR");
  display.display();
  //delay(2000);
  display.clearDisplay();
   
  
}
