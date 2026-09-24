



#include "digitalOutputs.h"
#include "../defines.h"
#include "driver/gpio.h"
#include "../dataStruct/dataStruct.h"
bool digitalOutputsInit(){


    gpio_reset_pin(LED_RUN);
    gpio_set_direction(LED_RUN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_RUN, 1);


    gpio_reset_pin(DO_1);
    gpio_set_direction(DO_1, GPIO_MODE_OUTPUT);
    gpio_set_level(DO_1,0);


}

void digitalOutputsUpdate(){

    gpio_set_level(LED_RUN,read_register(LEDRUN_STATE));
    gpio_set_level(DO_1,read_register(DO_1_STATE));

}