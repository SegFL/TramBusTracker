



#include "digitalOutputs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
bool digitalOutputsInit(){




    gpio_reset_pin(DO_1);
    gpio_set_direction(DO_1, GPIO_MODE_OUTPUT);
    gpio_set_level(DO_1,1);

    gpio_reset_pin(DO_2);
    gpio_set_direction(DO_2, GPIO_MODE_OUTPUT);
    gpio_set_level(DO_2,1);

    gpio_reset_pin(DO_3);
    gpio_set_direction(DO_3, GPIO_MODE_OUTPUT);
    gpio_set_level(DO_3,1);

    gpio_reset_pin(DO_4);
    gpio_set_direction(DO_4, GPIO_MODE_OUTPUT);
    gpio_set_level(DO_4,1);


}

void digitalOutputsUpdate(){

unsigned char delay=0;


/*
    #ifdef BYPASS_TAG_FILTER
        gpio_set_level(DO_1,!read_register(DI_1_STATE));
        gpio_set_level(DO_2,!read_register(DI_2_STATE));

    #else  /*  /*
        if(read_register(FORCE_OUTPUTS)!=0){
            gpio_set_level(DO_1,!read_register(DO_1_FORCED_STATE));
            gpio_set_level(DO_2,!read_register(DO_2_FORCED_STATE));
            gpio_set_level(DO_3,!read_register(DO_3_FORCED_STATE));
            gpio_set_level(DO_4,!read_register(DO_4_FORCED_STATE));

        }else{
        */
            gpio_set_level(DO_1,!read_register(DO_1_STATE));
            gpio_set_level(DO_2,!read_register(DO_2_STATE));
            gpio_set_level(DO_3,!read_register(DO_3_STATE));
            gpio_set_level(DO_4,!read_register(DO_4_STATE));
        /*
            if(read_register(MODO_INDUCTIVO_ENABLED)==1){
                if(read_register(DI_1_STATE))
                {
                    gpio_set_level(DO_3,!read_register(DI_1_STATE));
                    delay=1; 
                }
                if(read_register(DI_2_STATE))
                {
                    gpio_set_level(DO_4,!read_register(DI_2_STATE));
                    delay=1;
                    
                }
                if(delay==1)
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    
                delay=0;
           
            }else{
                //Dejo un rele prendido simepre y el otro apagado
                gpio_set_level(DO_3,0);//Encendido
                gpio_set_level(DO_4,1);//Apagado
            }
            
    */

        

}