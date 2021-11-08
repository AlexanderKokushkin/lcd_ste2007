// STE2007 LCD display library
#ifndef __lcd_ste2007__
#define __lcd_ste2007__

#include "driver/gpio.h"
#include <stdint.h>

#include <fonts_5x8.h>
#include <fonts_18x32.h> // mostly for clocks
#include <fonts_16x24.h> // 6 digits not stretched, 3 stretched

#include <generic_constants.h>

class Lcd2007{ 
  static constexpr gpio_num_t nokia_clk = GPIO_NUM_27;
  static constexpr gpio_num_t nokia_din = GPIO_NUM_26;
  static constexpr gpio_num_t nokia_rst = GPIO_NUM_25;

 public:
  inline static uint8_t sht = 0;
  static const uint16_t type = 2007;
  static const uint8_t txtWidth = 16; // 96/(5+1) = 16
  static const uint8_t txtHeigth = 8; // 68/8 = 8.5
  inline static char lazy[20] = {0}; 
  static void init(); 
  static void gotoXY( uint8_t x, uint8_t y);
  static void string(const char* ch, bool inverted = false);
  static void stringX(const char* ch, bool inverted = false);
  static void stretchedString(uint8_t x, uint8_t y, const char* ch, bool inverted = false);  
  static void string_P(const char* ch, bool inverted = false);
  static void stretchedString_P(uint8_t x, uint8_t y, const char* ch, bool inverted = false);
  static void clear();

  // forced (?) to became public
  static void character(uint8_t ch, bool inverted = false);
  static void stretchedCharacter(uint8_t ch, uint8_t subRow, bool inverted = false);
  static void bitmap_P(const uint8_t* bitmap, uint16_t size );
  // 16x24 -> stretched -> 32x48
  static void threeStretchedDigits(uint32_t value, uint8_t _row, uint8_t invertedDecimalPlace=4);
  // 16x24 not stretched
  static void sixBigDigits(uint32_t value, uint8_t _row=0, uint8_t invertedDecimalPlace=7);

  // forced as well
  static const uint8_t commandMode = 0;
  static const uint8_t dataMode = 1;
  static void write(uint8_t dc, uint8_t data);
  inline static bool doubled = false; 
 //protected:
  static const uint8_t INTERNAL_RESET = 0xE2;
  static const uint8_t maxX = 96; // not used here
  static const uint8_t maxY = 68; // not used here
  static uint8_t stretchVerticalBar(const uint8_t v, const uint8_t subRow);
};



#endif