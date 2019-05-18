// blog.iharder.net
// Robert Harder
// Thanks to DFRobot.com for the LiquidCrystal_I2C code that helped me understand a lot
// August 2014

#ifndef PortExpander_I2C_h
#define PortExpander_I2C_h

#include <Wire.h>


class PortExpander_I2C  {
public:
  PortExpander_I2C(uint8_t lcd_Addr);
  void init();
  void pinMode( uint8_t port, uint8_t mode );
  void digitalWrite( uint8_t port, uint8_t level );
  void digitalWriteAll( uint8_t levels );
  void digitalToggle( uint8_t port );
  uint8_t digitalRead( uint8_t port );
  uint8_t digitalReadAll();

private:
  void init_priv();
  void i2cWrite(uint8_t);
  uint8_t _Addr;
  uint8_t _portValues;
};

#endif
