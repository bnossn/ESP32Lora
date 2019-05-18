
uint8_t last_msgID;

inline int max(int a,int b) {return ((a)>(b)?(a):(b)); }

inline int min(int a,int b) {return ((a)<(b)?(a):(b)); }


void setupDisplay() {
//  display.begin();
//
//  // read diagnostics (optional but can help debug problems)
//  uint8_t x = display.readcommand8(ILI9341_RDMODE);
//  Serial_Print(bDebug, "Display Power Mode: 0x"); 
//  sprintf (Sbuffer, "%X", x);
//  Serial_Println(bDebug, Sbuffer);
//  
//  x = display.readcommand8(ILI9341_RDMADCTL);
//  Serial_Print(bDebug, "MADCTL Mode: 0x"); 
//  sprintf (Sbuffer, "%X", x);
//  Serial_Println(bDebug, Sbuffer);
//
//  x = display.readcommand8(ILI9341_RDPIXFMT);
//  Serial_Print(bDebug, "Pixel Format: 0x"); 
//  sprintf (Sbuffer, "%X", x);
//  Serial_Println(bDebug, Sbuffer);
//  
//  x = display.readcommand8(ILI9341_RDIMGFMT);
//  Serial_Print(bDebug, "Image Format: 0x"); 
//  sprintf (Sbuffer, "%X", x);
//  Serial_Println(bDebug, Sbuffer);
//  
//  x = display.readcommand8(ILI9341_RDSELFDIAG);
//  Serial_Print(bDebug, "Self Diagnostic: 0x"); 
//  sprintf (Sbuffer, "%X", x);
//  Serial_Println(bDebug, Sbuffer);
//
//  yield();
//  display.fillScreen(ILI9341_BLACK);
//  yield();
//  display.setTextColor(ILI9341_WHITE);
//  display.setTextSize(1);
//  display.setRotation(2);
//
//  Serial_Println(bDebug, F("Display Setup Done!"));

}

void action_relay()
  {
//    PEout.digitalWrite(4, LOW);   // turn the LED on (HIGH is the voltage level)
//    delay(1000);                     // wait for a second
//    PEout.digitalWrite(4, HIGH);    // turn the LED off by making the voltage LOW
 
  }

#ifndef MASTER
void displayMessage(uint8_t msgID){ //Slaves Message


    switch (msgID){
      case 10:
        if (last_msgID != msgID){
          yield();
          display.fillScreen(ILI9341_BLACK);
          yield();
          display.setTextColor(ILI9341_WHITE);  display.setTextSize(2);
          display.setCursor(0, 0);
          display.println("Slave");
        }
        break;
      case 20:
        if (last_msgID != msgID){
          yield();
          display.fillScreen(ILI9341_BLACK);
          yield();
          display.setTextColor(ILI9341_WHITE);    display.setTextSize(2);
          display.setCursor(0, 0);
          display.println("Slave: " + uint64ToHexStr(slave.id));
          display.println();
          display.setTextSize(1);
          display.println("Aguardando Gateway...");
          display.print("Gateway no: " + uint64ToHexStr(gatewayID));
        }
        break;
      case 30:
        if (last_msgID != msgID){
          yield();
          display.fillScreen(ILI9341_BLACK);
          yield();
          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
          display.setCursor(0, 0);
          display.println("Slave: " + uint64ToHexStr(slave.id));
          display.println("Master nao comunicando");
        }
        break;
      case 40:
        yield();
        display.fillScreen(ILI9341_BLACK);
        yield();
        display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
        display.setCursor(0, 0);
        display.println("Slave: " + uint64ToHexStr(slave.id));
        display.println("Valores Atuais: ");
        display.println("Outputs:");
        display.print("DO: ");
        display.println(String(slave.DigOut, HEX));
        display.print("AO0: ");
        display.println(String(slave.AO[0], DEC));
        display.print("AO1: ");
        display.println(String(slave.AO[1], DEC));
        display.println("Inputs:");
        display.print("DI: ");
        display.println(String(slave.DigIn, HEX));
        display.print("AI0: ");
        display.println(String(slave.AI[0], DEC));
        display.print("AI1: ");
        display.println(String(slave.AI[1], DEC));
        display.println("RSSI: " + String(LoRa.packetRssi()));
        display.println("SNR: " + String(LoRa.packetSnr()));
        break;
      case 255:
        break;
      default:
        Serial_Println(bDebug, "Message ID not Valid");
        break;
    }
    
  last_msgID = msgID;
}
#endif

#ifdef MASTER
void displayMessage(uint8_t msgID){ //Master Messages

    switch (msgID){
      case 10:
        if (last_msgID != msgID){
//          yield();
//          display.fillScreen(ILI9341_BLACK);
//          yield();
//          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
//          display.setCursor(0, 0);
//          display.println("Master");
//          display.println("Inicializando...");
        }
        break;
      case 20:
        if (last_msgID != msgID){
//          yield();
//          display.fillScreen(ILI9341_BLACK);
//          yield();
//          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
//          display.setCursor(0, 0);
//          display.print("Connecting to ");
//          display.print(ssid);
        }
        break;
      case 30:
        if (last_msgID != msgID){
//          yield();
//          display.fillScreen(ILI9341_BLACK);
//          yield();
//          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
//          display.setCursor(0, 0);
//          display.print("Connecting to MQTT Server...");
        }
        break;
      case 40:
        if (last_msgID != msgID){
//          yield();
//          display.fillScreen(ILI9341_BLACK);
//          yield();
//          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
//          display.setCursor(0, 0);
//          display.println("Subscribing to topics");
        }
        break;
      case 50:
        if (last_msgID != msgID){
//          yield();
//          display.fillScreen(ILI9341_BLACK);
//          yield();
//          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
//          display.setCursor(0, 0);
//          display.println("Master");
//          display.println("Aguardando Primeira Mensagem");
        }
        break;
      case 60:
//          yield();
//          display.fillScreen(ILI9341_BLACK);
//          yield();
//          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
//          display.setCursor(0, 0);
          for (int i = 0; i < NMAX_SLAVES; i++){
            if (slaves[i].id != 0){
//              //display.setCursor(0, (i*10));
//              display.print("Slave ");
//              display.print(i);
//              display.print(": ");
//              if(slaves[i].isOnline){
//                display.println("online");
//              }else{
//                display.println("offline");
//              }
//              display.print("-ID: ");
//              display.println(uint64ToHexStr(slaves[i].id));
//              display.print("-RSSI: ");
//              display.println(SlavesRSSI[i]);
//              display.print("-SNR: ");
//              display.println(SlavesSNR[i]);
            }
          }
        break;
      case 70:
        if (last_msgID != msgID){
//          yield();
//          display.fillScreen(ILI9341_BLACK);
//          yield();
//          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
//          display.setCursor(0, 0);
        }
        break;
      case 80:
        if (last_msgID != msgID){
//          yield();
//          display.fillScreen(ILI9341_BLACK);
//          yield();
//          display.setTextColor(ILI9341_WHITE); display.setTextSize(2);
//          display.setCursor(0, 0);
        }
        break;
      default:
        Serial_Println(bDebug, "Message ID not Valid");
        break;
    }
    
  last_msgID = msgID;
}
#endif

  
unsigned long testFillScreen() {
  unsigned long start = micros();
  display.fillScreen(ILI9341_BLACK);
  yield();
  display.fillScreen(ILI9341_RED);
  yield();
  display.fillScreen(ILI9341_GREEN);
  yield();
  display.fillScreen(ILI9341_BLUE);
  yield();
  display.fillScreen(ILI9341_BLACK);
  yield();
  return micros() - start;
}

unsigned long testText() {
  display.fillScreen(ILI9341_BLACK);
  unsigned long start = micros();
  display.setCursor(0, 0);
  display.setTextColor(ILI9341_WHITE);  display.setTextSize(1);
  display.println("Hello World!");
  display.setTextColor(ILI9341_YELLOW); display.setTextSize(2);
  display.println(1234.56);
  display.setTextColor(ILI9341_RED);    display.setTextSize(3);
  display.println(0xDEADBEEF, HEX);
  display.println();
  display.setTextColor(ILI9341_GREEN);
  display.setTextSize(3);
  display.println("AFELETRONICA");
  display.setTextSize(2);
  display.println("PIA");
  display.setTextSize(1);
  display.println("IOT");
  display.println("SOLUTION");
  display.println("O CEU ");
  display.println("EH");
  display.println("O LIMITE");
  display.println("PARA AQUELES");
  display.println("QUE ACREDITAM");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = display.width(),
                h = display.height();

  display.fillScreen(ILI9341_BLACK);
  yield();
  
  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) display.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) display.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing

  yield();
  display.fillScreen(ILI9341_BLACK);
  yield();

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) display.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) display.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  yield();
  display.fillScreen(ILI9341_BLACK);
  yield();

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) display.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) display.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  yield();
  display.fillScreen(ILI9341_BLACK);
  yield();

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) display.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) display.drawLine(x1, y1, x2, y2, color);

  yield();
  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = display.width(), h = display.height();

  display.fillScreen(ILI9341_BLACK);
  start = micros();
  for(y=0; y<h; y+=5) display.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=5) display.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = display.width()  / 2,
                cy = display.height() / 2;

  display.fillScreen(ILI9341_BLACK);
  n     = min(display.width(), display.height());
  start = micros();
  for(i=2; i<n; i+=6) {
    i2 = i / 2;
    display.drawRect(cx-i2, cy-i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = display.width()  / 2 - 1,
                cy = display.height() / 2 - 1;

  display.fillScreen(ILI9341_BLACK);
  n = min(display.width(), display.height());
  for(i=n; i>0; i-=6) {
    i2    = i / 2;
    start = micros();
    display.fillRect(cx-i2, cy-i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    display.drawRect(cx-i2, cy-i2, i, i, color2);
    yield();
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = display.width(), h = display.height(), r2 = radius * 2;

  display.fillScreen(ILI9341_BLACK);
  start = micros();
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      display.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                w = display.width()  + radius,
                h = display.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for(x=0; x<w; x+=r2) {
    for(y=0; y<h; y+=r2) {
      display.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = display.width()  / 2 - 1,
                      cy = display.height() / 2 - 1;

  display.fillScreen(ILI9341_BLACK);
  n     = min(cx, cy);
  start = micros();
  for(i=0; i<n; i+=5) {
    display.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      display.color565(i, i, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = display.width()  / 2 - 1,
                   cy = display.height() / 2 - 1;

  display.fillScreen(ILI9341_BLACK);
  start = micros();
  for(i=min(cx,cy); i>10; i-=5) {
    start = micros();
    display.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      display.color565(0, i*10, i*10));
    t += micros() - start;
    display.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      display.color565(i*10, i*10, 0));
    yield();
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = display.width()  / 2 - 1,
                cy = display.height() / 2 - 1;

  display.fillScreen(ILI9341_BLACK);
  w     = min(display.width(), display.height());
  start = micros();
  for(i=0; i<w; i+=6) {
    i2 = i / 2;
    display.drawRoundRect(cx-i2, cy-i2, i, i, i/8, display.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = display.width()  / 2 - 1,
                cy = display.height() / 2 - 1;

  display.fillScreen(ILI9341_BLACK);
  start = micros();
  for(i=min(display.width(), display.height()); i>20; i-=6) {
    i2 = i / 2;
    display.fillRoundRect(cx-i2, cy-i2, i, i, i/8, display.color565(0, i, 0));
    yield();
  }

  return micros() - start;
}
