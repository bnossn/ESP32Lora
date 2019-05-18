#include <Wire.h>
#include <LiquidCrystal_I2C.h>
 
LiquidCrystal_I2C lcd(0x3F,16,2); // set the LCD address to 0x3F for a 16 chars and 2 line display

int Button = 0;

void setup()
{
  pinMode(Button, INPUT);
  digitalWrite(Button, HIGH);
lcd.init(); // initialize the lcd
lcd.init();
// Print a message to the LCD.
lcd.backlight();
lcd.setCursor(0,0);
lcd.print("Acccess Control");
lcd.setCursor(0,1);
lcd.print("    INELTEC     ");
delay(1000);


lcd.noBacklight(); //desliga BackLight Display
}
 
void loop()
{
  int Botao = digitalRead(Button);
//  for (int positionCounter = 0; positionCounter < 16; positionCounter++) {
//    lcd.setCursor(0,1);
//    lcd.print("INELTEC");
//    // scroll one position left:
//    lcd.scrollDisplayLeft();
//    // wait a bit:
//    delay(300);
//  }
   // lcd.noBacklight(); //desliga BackLight Display
    if(Botao == 0)
    {
      lcd.backlight(); //liga BackLight Display
      delay(2000);
      lcd.noBacklight(); //desliga BackLight Display
  
      
    }
}
