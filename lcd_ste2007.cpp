#include "lcd_ste2007.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/mpu_wrappers.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include <stdint.h>
#include <string.h>

void Lcd2007::write(uint8_t dc, uint8_t data){

  if(dc){
    gpio_set_level(nokia_din,1);
  }else{
    gpio_set_level(nokia_din,0);
  }
  gpio_set_level(nokia_clk,1);
  vTaskDelay(1 / portTICK_RATE_MS); // asm ("nop");
  gpio_set_level(nokia_clk,0);
 
  for (uint8_t i = 0; i < 8; ++i){ // shiftout data
	  if( (data & (1 << (7 - i))) ){
          gpio_set_level(nokia_din,1);
		  }else{
          gpio_set_level(nokia_din,0);    
	  }
      gpio_set_level(nokia_clk,1);
	  vTaskDelay(1 / portTICK_RATE_MS); // asm ("nop");
      gpio_set_level(nokia_clk,0);
  } 
}

void Lcd2007::init(){

    gpio_config_t tmp_io_conf{
        .pin_bit_mask = ((1ULL<<nokia_clk) | (1ULL<<nokia_din) | (1ULL<<nokia_rst)),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,       
    };
    gpio_config(&tmp_io_conf);
    gpio_set_level(nokia_clk,0);

    //magic rite borrowed from Larry Bank https://github.com/bitbank2/bb_hx1230
    gpio_set_level(nokia_rst,1);
    vTaskDelay(50 / portTICK_RATE_MS);
    gpio_set_level(nokia_rst,0);
    vTaskDelay(5 / portTICK_RATE_MS);
    gpio_set_level(nokia_rst,1);
    vTaskDelay(10 / portTICK_RATE_MS);

    sht = someClass::someConst2;
    //vTaskDelay(someClass::someConst / portTICK_RATE_MS); // testing

    write(commandMode,INTERNAL_RESET);
    vTaskDelay(10 / portTICK_RATE_MS);
    write(commandMode,0xA4);  //Display all points OFF
    write(commandMode,0xA0);  //Segment driver direction select = normal
    write(commandMode,0x2F);  //power control set (on/on/on)
    write(commandMode,0xB0);  //set page address
    write(commandMode,0x10);  //set col=0 upper 3 bits
    write(commandMode,0x00);  //set col=0 lower 4 bits
    write(commandMode,0xAF);  //graphicsLCD display on 

  // btw, DS recommended:  0x2F,0x90,0xA6,0xA4,0xAF,0x40,0xB0,0x10,0x00
}

void Lcd2007::gotoXY( uint8_t x, uint8_t y){
  write(commandMode, 0xB0 + y );
  write(commandMode, 0x10 | ((x&0x7F)>>4) );
  write(commandMode, 0x0F & x );
}

void Lcd2007::clear(){
  gotoXY(0,0);
  for(uint8_t i = 0; i<9; ++i){ // it's actually 8.5
    for(uint8_t j = 0; j<96; ++j){
      write(dataMode,0x00);
    }
  }
  gotoXY(0,0);  
}

void Lcd2007::character(uint8_t ch, bool inverted){

  // we use only one spacer here - after the character
  for (uint8_t i = 0; i < 5; ++i){
    uint8_t pixels = ASCII_5x8[ch - 0x20][i];
    // uint8_t pixels = pgm_read_byte_near( &ASCII_5x8[ch - 0x20][i] );
    write(dataMode, inverted ?~pixels:pixels );
  }
  write(dataMode,inverted?255:0); // spacer(vertical line)
}

void Lcd2007::stretchedCharacter(uint8_t ch, uint8_t subRow, bool inverted){
  for (uint8_t i = 0; i < 5; ++i){
    uint8_t pixels = ASCII_5x8[ch - 0x20][i];
    uint8_t stretched_pixels = stretchVerticalBar(pixels,subRow);

    write(dataMode, inverted ?~stretched_pixels:stretched_pixels );
    write(dataMode, inverted ?~stretched_pixels:stretched_pixels );
  }
  write(dataMode,inverted?255:0); // spacer(vertical line)
  write(dataMode,inverted?255:0); // spacer(vertical line)
}

void Lcd2007::string(const char* ch, bool inverted){
  uint8_t const spacer = ' '; 
  bool alignmentIsNecessary = (strlen(ch)==12); // magic number
  
  if (alignmentIsNecessary){ character(spacer, inverted ); character(spacer, inverted );};
  while (*ch){character(*ch++, inverted );}
  if (alignmentIsNecessary){ character(spacer, inverted ); character(spacer, inverted );};
  
}

uint8_t Lcd2007::stretchVerticalBar(const uint8_t v, const uint8_t subRow){

    // Warning: Absolutely Unmaintainable Code (see unrolled.txt)
    uint8_t outcome = 0;
    for (uint8_t i=0;i<8;++i){
      if( (v & (1 << ( (subRow==0?3:7) - (i>>1)))) ){
        outcome |= 1<<(7-i);
      }
    }  
    return(outcome);
}

void Lcd2007::threeStretchedDigits(uint32_t value, uint8_t _row, uint8_t invertedDecimalPlace){
  // 16x24 -> 32x48
  const uint8_t digit_capacity = 3;
  uint8_t components[digit_capacity] = {0}; // array of digits : 123->[1,2,3]

  for (uint8_t i=0; i<digit_capacity; ++i){ // fill up the array
    uint16_t power10 = 1;
    for (uint8_t j=0; j<digit_capacity-i-1; ++j){ power10*=10; }
    components[i] = value / power10;
    value %= power10;
  }
  
  for(uint8_t row = 0; row < 3; ++row){ // original 16x24 font has 3 rows (3x8bit=24)
    for(uint8_t subRow=0;subRow<2;++subRow){ // every char will be stretched
      gotoXY(0,row*2 + subRow +_row);
      for(uint8_t digit = 0; digit < digit_capacity; ++digit){
        for(uint8_t col = 0; col < 16; ++col){ // original 16x24 font has 3x8bit rows
          uint8_t pixels = digits_16x24[components[digit]][row][col];
          uint8_t s_pixels = stretchVerticalBar(pixels,subRow);
          
          write(dataMode, (digit != invertedDecimalPlace) ? s_pixels : ~s_pixels );
          write(dataMode, (digit != invertedDecimalPlace) ? s_pixels : ~s_pixels );
        } // for every col [0..16]
        } // for every input digit, e.g. {5,4,2}
      } // for every subRow
    } // for every row, i.e. 3
}

void Lcd2007::sixBigDigits(uint32_t value, uint8_t _row, uint8_t invertedDecimalPlace){

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
        write(dataMode, (digit != invertedDecimalPlace) ? pixels : ~pixels );
      } // for every col [0..16] 16x24 font has 16 columns
      } // for every input digit, e.g. {0,0,5,4,2,9}
    } // for every row, i.e. 3
}

