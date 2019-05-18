/*
 * There are three serial ports on the ESP known as U0UXD, U1UXD and U2UXD.
 * 
 * U0UXD is used to communicate with the ESP32 for programming and during reset/boot.
 * U1UXD is unused and can be used for your projects. Some boards use this port for SPI Flash access though
 * U2UXD is unused and can be used for your projects.
 * http://www.imei.info
 * 
*/
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
//HardwareSerial Serial1(1);
HardwareSerial Serial2(2);

//#define RXD2 16
//#define TXD2 17

#define RXD2 13
#define TXD2 14

// For the Adafruit shield, these are the default.
#define TFT_DC 2
#define TFT_CS 4

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {

  tft.begin();
  tft.setRotation(3); // OPTION 0/1/2/3/4
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_BLUE);  
  tft.setTextSize(3);
  tft.println("PIA-IOT SOLUTIONS");
  tft.setTextColor(ILI9341_YELLOW);
  tft.println("   AFELETRONICA  ");

  
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial.begin(115200);
  
  //Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial Txd is on pin: "+String(TX));
  Serial.println("Serial Rxd is on pin: "+String(RX));
  Serial2.println("AT\n");
  delay(150);
  Serial2.println("AT+GMI"); //Request manufacturer identification 
  delay(150);
  Serial2.println("AT+GMR"); //Request TA revision identification of software release 
  delay(150);
  Serial2.println("AT+GSN "); // Request TA serial number identification (IMEI) 
  delay(150); 

}

void GetSignalQuality(){
     String response = ""; 
     long int time = millis();   
     Serial.println("Obtendo a qualidade do sinal ...");
     Serial.println("Tips:+CSQ: XX,QQ : Significa a Qualidade do Sinal baixo quando o XX é '99'!");
     Serial2.println("AT+CSQ");    
     while( (time+2000) > millis()){
        while(Serial2.available()){       
          response += char(Serial2.read());
        }  
      }    
      Serial.print(response);   

}
void SendTextMessage()
{
Serial2.print("\r");
delay(1000);                    //Wait for a second while the modem sends an "OK"
Serial2.print("AT+CMGF=1\r");    //Because we want to send the SMS in text mode
delay(1000);
//to be sent to the number specified.
//Replace this number with the target mobile number.
Serial2.println("AT+CMGS=\"982329344\"");
delay(1000);
Serial2.print("PIA - IOT SOLUTINONS\r");   //The text for the message
delay(1000);
Serial2.write(0x1A);  //Equivalent to sending Ctrl+Z 

}

void DialVoiceCall()
{
    /*Nos testes realizados não foi necessário colocar o código do país bem como código da operadora, 
    quando colocado não realizou a ligação, mas atente que é necessário informar o DDD do estado do celular*/
    //Serial2.println("ATD019991834606;\r\n");
    Serial2.println("ATD019982329344;\r\n");
                                                        
    delay(10000);
    Serial2.println();
}

void SubmitHttpRequest()
{
Serial2.println("AT+CSQ");
delay(100);

//ShowSerialData();// this code is to show the data from gprs shield, in order to easily see the process of how the gprs shield submit a http request, and the following is for this purpose too.

Serial2.println("AT+CGATT?");
delay(100);

//ShowSerialData();

Serial2.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
delay(1000);

//ShowSerialData();

Serial2.println("AT+SAPBR=3,1,\"APN\",\"CLARO\"");//setting the APN, the second need you fill in your local apn server
delay(4000);

//ShowSerialData();

Serial2.println("AT+SAPBR=1,1");//setting the SAPBR, for detail you can refer to the AT command mamual
delay(5000);

//ShowSerialData();

Serial2.println("AT+HTTPINIT"); //init the HTTP request

delay(3000); 
//ShowSerialData();

Serial2.println("AT+HTTPPARA=\"URL\",\"www.uol.com.br\"");// setting the http para, the second parameter is the website you want to access
delay(3000);

//ShowSerialData();

Serial2.println("AT+HTTPACTION=0");//submit the request 
delay(10000);//o atraso é muito importante, o tempo de atraso é baseado no retorno do site, se os dados de retorno são muito grandes, o tempo necessário sera longo.
//while(!mySerial.available());

//ShowSerialData();

Serial2.println("AT+HTTPREAD");// le os dados do site que você acessado
delay(300);

//ShowSerialData();

Serial2.println("");
delay(100);
}

void loop() 
{ 
  
      if (Serial.available())
        switch(Serial.read())
          {
          
              case 'q':
              GetSignalQuality();
              break;
              case 't':
              SendTextMessage();
              break;
              case 'd':
              DialVoiceCall();
              break;
              case 'h':
              SubmitHttpRequest();
              break;
         }
    if (Serial2.available())
          Serial.write(Serial2.read());
}

/* Baud-rates available: 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, or 115200, 256000, 512000, 962100
 *  
 *  Protocols available:
 * SERIAL_5N1   5-bit No parity 1 stop bit
 * SERIAL_6N1   6-bit No parity 1 stop bit
 * SERIAL_7N1   7-bit No parity 1 stop bit
 * SERIAL_8N1 (the default) 8-bit No parity 1 stop bit
 * SERIAL_5N2   5-bit No parity 2 stop bits 
 * SERIAL_6N2   6-bit No parity 2 stop bits
 * SERIAL_7N2   7-bit No parity 2 stop bits
 * SERIAL_8N2   8-bit No parity 2 stop bits 
 * SERIAL_5E1   5-bit Even parity 1 stop bit
 * SERIAL_6E1   6-bit Even parity 1 stop bit
 * SERIAL_7E1   7-bit Even parity 1 stop bit 
 * SERIAL_8E1   8-bit Even parity 1 stop bit 
 * SERIAL_5E2   5-bit Even parity 2 stop bit 
 * SERIAL_6E2   6-bit Even parity 2 stop bit 
 * SERIAL_7E2   7-bit Even parity 2 stop bit  
 * SERIAL_8E2   8-bit Even parity 2 stop bit  
 * SERIAL_5O1   5-bit Odd  parity 1 stop bit  
 * SERIAL_6O1   6-bit Odd  parity 1 stop bit   
 * SERIAL_7O1   7-bit Odd  parity 1 stop bit  
 * SERIAL_8O1   8-bit Odd  parity 1 stop bit   
 * SERIAL_5O2   5-bit Odd  parity 2 stop bit   
 * SERIAL_6O2   6-bit Odd  parity 2 stop bit    
 * SERIAL_7O2   7-bit Odd  parity 2 stop bit    
 * SERIAL_8O2   8-bit Odd  parity 2 stop bit    
*/
