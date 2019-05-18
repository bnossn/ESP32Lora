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
/*#include <SPI.h>              // include libraries
#include "LoRa.h"
//#include <SD.h>
//#include <mySD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
//#define OLED_SDA 21
//#define OLED_SCL 22


Adafruit_SH1106 display(21, 22);
//Adafruit_SH1106 display(22, 21);*/

#include <driver/adc.h>

//SE Módulo com ARDUINO PRO-MINI
//const int csPin = 10;          // LoRa radio chip select
//const int resetPin = 9;       // LoRa radio reset
//const int irqPin = 2;         // change for your board; must be a hardware interrupt pin
/*
//SE Módulo com  ESP32
#define chipSelect 25
const int csPin = 5;          // LoRa radio chip select
const int resetPin = 16;       // LoRa radio reset
const int irqPin = 17;         // change for your board; must be a hardware interrupt pin*/


String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends
//Analog Input
//#define ANALOG_PIN_14 14

#define    ADC_CHANNEL_0    36     /*!< ADC channel */ 
#define    ADC_CHANNEL_3    39     /*!< ADC channel */ 
#define    ADC_CHANNEL_6    34     /*!< ADC channel */ 
#define    ADC_CHANNEL_7    35     /*!< ADC channel */ 
#define    ADC2_CHANNEL_6   14     /*!< ADC2 channel 6 is GPIO14 */ 
#define    ADC2_CHANNEL_7   27     /*!< ADC2 channel 7 is GPIO27 */ // liguei errado na pcb sera ajustado para futuras versões


int analog_ADC_CHANNEL_0 = 0;
int analog_ADC_CHANNEL_3 = 0;
int analog_ADC_CHANNEL_6 = 0;
int analog_ADC_CHANNEL_7 = 0;
int analog_ADC2_CHANNEL_6 = 0;
int analog_ADC2_CHANNEL_7 = 0;

void setup() {
  
  Serial.begin(115200);                   // initialize serial
  while (!Serial);

 /* display.begin(SH1106_SWITCHCAPVCC, 0x3C); 
  display.clearDisplay();*/

/*
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("PIA - IOT SOLUTION R0.0");
  display.setTextSize(1);
  display.setCursor(0,30);
  display.println("AFELETRONICA");
  display.display();
  delay(2000);
  display.clearDisplay();*/
  
  Serial.println("PIA - IOT SOLUTION R0.0");

 
}

void loop() {

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);
  analog_ADC_CHANNEL_0 = adc1_get_raw(ADC1_CHANNEL_0);

  adc1_config_channel_atten(ADC1_CHANNEL_3,ADC_ATTEN_DB_11);
  analog_ADC_CHANNEL_3 = adc1_get_raw(ADC1_CHANNEL_3);

  //analog_ADC_CHANNEL_0  =  analogRead(ADC_CHANNEL_0);
  //analog_ADC_CHANNEL_3  =  analogRead(ADC_CHANNEL_3);
  analog_ADC_CHANNEL_6  =  analogRead(ADC_CHANNEL_6);
  analog_ADC_CHANNEL_7  =  analogRead(ADC_CHANNEL_7);
  analog_ADC2_CHANNEL_6 =  analogRead(ADC2_CHANNEL_6); // ok funcionando
  analog_ADC2_CHANNEL_7 =  analogRead(ADC2_CHANNEL_7);
  
  Serial.println("C36: " + String(analog_ADC_CHANNEL_0));
  delay(100);
  Serial.println("C39: " + String(analog_ADC_CHANNEL_3));
  Serial.println("C34: " + String(analog_ADC_CHANNEL_6));
  Serial.println("C35: " + String(analog_ADC_CHANNEL_7));
  Serial.println("C14: " + String(analog_ADC2_CHANNEL_6)); // ok funcionando
  Serial.println("C27: " + String(analog_ADC2_CHANNEL_7));
       
  delay(500);
  //display.setTextSize(1);
 /* display.setCursor(0,35);
  display.setTextSize(4);
  display.clearDisplay();
  display.println(analog_ADC_CHANNEL_0);
  display.display();*/
   
}
