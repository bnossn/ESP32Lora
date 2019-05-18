#include <PortExpander_I2C.h>
#include <Wire.h>  
PortExpander_I2C ler(0x26); // LEITURA ENTRADAS
PortExpander_I2C pe(0x27); // ESCRITA RELES
int ledPin = 6; // This is the eighth expansion port
int i;


void setup() { 
  ler.init();
  pe.init();
  Serial.begin(115200);
  pinMode(12,INPUT);
  pinMode(2,OUTPUT);
  for( int i = 0; i < 8; i++ ){
    ler.pinMode(i,INPUT);
  }
}

void loop() {
 int io12= digitalRead(12);
  for( int i = 0; i < 8; i++ ){
    if( ler.digitalRead(i) == 0 ){
      Serial.print("Pin " );
      Serial.print(i);
      Serial.println(" is low.");
    }
  }
  if (io12 == LOW) {
    digitalWrite(2, HIGH);
  } else {
    digitalWrite(2, LOW);
  }


  
  
}
