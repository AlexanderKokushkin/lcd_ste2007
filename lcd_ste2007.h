// STE2007 LCD display library
#ifndef __lcd_ste2007__
#define __lcd_ste2007__

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/mpu_wrappers.h"
#include "freertos/semphr.h"
#include <stdint.h>
#include <string.h>

#if CONFIG_IDF_TARGET_ESP32
 #include <esp32/rom/ets_sys.h> // for ets_delay_us()
#endif
#if CONFIG_IDF_TARGET_ESP32C3
 #include <esp32c3/rom/ets_sys.h> // for ets_delay_us()
#endif

#include <fonts_5x8.h>
#include <fonts_18x32.h> // mostly for clocks
#include <fonts_16x24.h> // 6 digits not stretched, 3 stretched

//#include <generic_constants.h> // just for tests

/*******************************************************************************************
* usage:
* class Settings{
*    public:
*        static constexpr gpio_num_t nokia_clk = GPIO_NUM_3;
*        static constexpr gpio_num_t nokia_din = GPIO_NUM_2;
*        static constexpr gpio_num_t nokia_rst = GPIO_NUM_1;
*};
*using Lcd = Lcd2007_T<Settings>;
********************************************************************************************/

template<class T>
class Lcd2007_T{ 
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
  // 16x24 -> stretched -> 32x48
  static void threeStretchedDigits(uint32_t value, uint8_t _row, uint8_t invertedDecimalPlace=4);
  // 16x24 not stretched
  static void sixBigDigits(uint32_t value, uint8_t _row=0, uint8_t invertedDecimalPlace=7);
 protected:
  static void character(uint8_t ch, bool inverted = false);
  static void stretchedCharacter(uint8_t ch, uint8_t subRow, bool inverted = false);
  static void bitmap_P(const uint8_t* bitmap, uint16_t size );
  static const uint8_t cmd = 0;
  static const uint8_t data = 1;
  static void write(uint8_t dc, uint8_t data);
  static const uint8_t INTERNAL_RESET = 0xE2;
  static const uint8_t maxX = 96; // not used here
  static const uint8_t maxY = 68; // not used here
  static uint8_t stretchVerticalBar(const uint8_t v, const uint8_t subRow);
};

template<class T> void Lcd2007_T<T>::write(uint8_t dc, uint8_t data){

  gpio_set_level(T::nokia_din,dc); // set data or cmd mode
  gpio_set_level(T::nokia_clk,1);
  ets_delay_us(5);
  gpio_set_level(T::nokia_clk,0);
 
  for (uint8_t i = 0; i < 8; ++i){ // shiftout data
	  if( (data & (1 << (7 - i))) ){
          gpio_set_level(T::nokia_din,1);
		  }else{
          gpio_set_level(T::nokia_din,0);    
	  }
      gpio_set_level(T::nokia_clk,1);
      ets_delay_us(5);
      gpio_set_level(T::nokia_clk,0);
  } 
}

template<class T> void Lcd2007_T<T>::init(){

    //sht = someClass::someConst2;

    gpio_config_t tmp_io_conf{
        .pin_bit_mask = ((1ULL<<T::nokia_clk) | (1ULL<<T::nokia_din) | (1ULL<<T::nokia_rst)),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,       
    };
    gpio_config(&tmp_io_conf);
    gpio_set_level(T::nokia_clk,0);

    //magic rite borrowed from Larry Bank https://github.com/bitbank2/bb_hx1230
    gpio_set_level(T::nokia_rst,1);
    vTaskDelay(50 / portTICK_RATE_MS);
    gpio_set_level(T::nokia_rst,0);
    vTaskDelay(5 / portTICK_RATE_MS);
    gpio_set_level(T::nokia_rst,1);
    vTaskDelay(10 / portTICK_RATE_MS);

    write(cmd,INTERNAL_RESET);
    vTaskDelay(10 / portTICK_RATE_MS);
    write(cmd,0xA4);  //Display all points OFF
    write(cmd,0xA0);  //Segment driver direction select = normal
    write(cmd,0x2F);  //power control set (on/on/on)
    write(cmd,0xB0);  //set page address
    write(cmd,0x10);  //set col=0 upper 3 bits
    write(cmd,0x00);  //set col=0 lower 4 bits
    write(cmd,0xAF);  //graphicsLCD display on 

  // btw, DS recommended:  0x2F,0x90,0xA6,0xA4,0xAF,0x40,0xB0,0x10,0x00
}

template<class T> void Lcd2007_T<T>::gotoXY( uint8_t x, uint8_t y){
  write(cmd, 0xB0 + y );
  write(cmd, 0x10 | ((x&0x7F)>>4) );
  write(cmd, 0x0F & x );
}

template<class T> void Lcd2007_T<T>::clear(){
  gotoXY(0,0);
  for(uint8_t i = 0; i<9; ++i){ // it's actually 8.5
    for(uint8_t j = 0; j<96; ++j){
      write(data,0x00);
    }
  }
  gotoXY(0,0);  
}

template<class T> void Lcd2007_T<T>::character(uint8_t ch, bool inverted){

  // we use only one spacer here - after the character
  for (uint8_t i = 0; i < 5; ++i){
    uint8_t pixels = ASCII_5x8[ch - 0x20][i];
    // uint8_t pixels = pgm_read_byte_near( &ASCII_5x8[ch - 0x20][i] );
    write(data, inverted ?~pixels:pixels );
  }
  write(data,inverted?255:0); // spacer(vertical line)
}

template<class T> void Lcd2007_T<T>::stretchedCharacter(uint8_t ch, uint8_t subRow, bool inverted){
  for (uint8_t i = 0; i < 5; ++i){
    uint8_t pixels = ASCII_5x8[ch - 0x20][i];
    uint8_t stretched_pixels = stretchVerticalBar(pixels,subRow);

    write(data, inverted ?~stretched_pixels:stretched_pixels );
    write(data, inverted ?~stretched_pixels:stretched_pixels );
  }
  write(data,inverted?255:0); // spacer(vertical line)
  write(data,inverted?255:0); // spacer(vertical line)
}

template<class T> void Lcd2007_T<T>::string(const char* ch, bool inverted){
  uint8_t const spacer = ' '; 
  bool alignmentIsNecessary = (strlen(ch)==12); // magic number
  
  if (alignmentIsNecessary){ character(spacer, inverted ); character(spacer, inverted );};
  while (*ch){character(*ch++, inverted );}
  if (alignmentIsNecessary){ character(spacer, inverted ); character(spacer, inverted );};
  
}

template<class T> uint8_t Lcd2007_T<T>::stretchVerticalBar(const uint8_t v, const uint8_t subRow){

    // Warning: Absolutely Unmaintainable Code (see unrolled.txt)
    uint8_t outcome = 0;
    for (uint8_t i=0;i<8;++i){
      if( (v & (1 << ( (subRow==0?3:7) - (i>>1)))) ){
        outcome |= 1<<(7-i);
      }
    }  
    return(outcome);
}

template<class T> void Lcd2007_T<T>::threeStretchedDigits(uint32_t value, uint8_t _row, uint8_t invertedDecimalPlace){
  // 16x24 -> 32x48
  const uint8_t digit_capacity = 3;
  uint8_t components[digit_capacity] = {0}; // array of digits : 123->[1,2,3]

  for (uint8_t i=0; i<digit_capacity; ++i){ // fill up the array
    uint16_t power10 = 1;
    for (uint8_t j=0; j<digit_capacity-i-1; ++j){ power10*=10; }
    components[i] = (value / power10)%10;
    value %= power10;
  }

  for(uint8_t row = 0; row < 3; ++row){ // original 16x24 font has 3 rows (3x8bit=24)
    for(uint8_t subRow=0;subRow<2;++subRow){ // every char will be stretched
      gotoXY(0,row*2 + subRow +_row);
      for(uint8_t digit = 0; digit < digit_capacity; ++digit){
        for(uint8_t col = 0; col < 16; ++col){ // original 16x24 font has 3x8bit rows
          uint8_t pixels = digits_16x24[components[digit]][row][col];
          uint8_t s_pixels = stretchVerticalBar(pixels,subRow);
          
          write(data, (digit != invertedDecimalPlace) ? s_pixels : ~s_pixels );
          write(data, (digit != invertedDecimalPlace) ? s_pixels : ~s_pixels );
        } // for every col [0..16]
        } // for every input digit, e.g. {5,4,2}
      } // for every subRow
    } // for every row, i.e. 3
}

template<class T> void Lcd2007_T<T>::sixBigDigits(uint32_t value, uint8_t _row, uint8_t invertedDecimalPlace){

  const uint8_t digit_capacity = 6;
  uint8_t components[digit_capacity] = {0};
  
  for (uint8_t i=0; i<digit_capacity; ++i){ // fill up the array
    uint32_t power10 = 1;
    for (uint8_t j=0; j<digit_capacity-i-1; ++j){ power10*=10; }
    components[i] = value / power10;
    value %= power10;
  }
  
  for(uint8_t row = 0; row < 3; ++row){ // 16x24 font has 3 rows
    gotoXY(0,row+_row);
    for(uint8_t digit = 0; digit < digit_capacity; ++digit){
      for(uint8_t col = 0; col < 16; ++col){
        uint8_t pixels = digits_16x24[components[digit]][row][col];
        write(data, (digit != invertedDecimalPlace) ? pixels : ~pixels );
      } // for every col [0..16] 16x24 font has 16 columns
      } // for every input digit, e.g. {0,0,5,4,2,9}
    } // for every row, i.e. 3
}

#endif