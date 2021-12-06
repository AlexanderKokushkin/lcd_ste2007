#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <cxxabi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/mpu_wrappers.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <string.h>

#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "driver/rtc_io.h"

#include <lcd_ste2007.h>

using std::cout;

class Settings{
    public:
        static constexpr gpio_num_t nokia_clk = GPIO_NUM_3;
        static constexpr gpio_num_t nokia_din = GPIO_NUM_2;
        static constexpr gpio_num_t nokia_rst = GPIO_NUM_1;
};
using Lcd = Lcd2007_T<Settings>;


extern "C" void app_main(){
    
   nvs_flash_init();
   vTaskDelay(pdMS_TO_TICKS(100)); //Allow other core to finish initialization
   
   while(1){
        cout << " let's start! \n";

        Lcd::init();
        Lcd::clear();

        for (uint8_t y=0;y<Lcd::txtHeigth;++y){ 
            for (uint8_t x=0;x<Lcd::txtWidth;++x){
                Lcd::gotoXY(x*6,y);
                Lcd::string("D");
         }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        Lcd::sixBigDigits(123456);
        vTaskDelay(pdMS_TO_TICKS(1500));
        Lcd::clear();
        vTaskDelay(pdMS_TO_TICKS(1000));
   }

}
