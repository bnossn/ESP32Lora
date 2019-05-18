#include <PortExpander_I2C.h>

PortExpander_I2C pe(0x27); // ESCRITA RELES
PortExpander_I2C ler(0x26); // LEITURA ENTRADAS

int ledPin = 6; // This is the eighth expansion port
int i;
void setup() {  
  pe.init();
}

void loop() {
  for(i=0; i<=6;i++)
  {
    pe.digitalWrite(i, LOW);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                     // wait for a second
    pe.digitalWrite(i, HIGH);    // turn the LED off by making the voltage LOW
    delay(1000);                     // wait for a second
  }
}
