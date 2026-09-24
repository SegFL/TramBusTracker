    
//Este driver deberia encargarse de leer dataStruct
// y decidir que led hacer titilar y a que velocidad
#include "ledsDriver.h"

#define cantidadleds 1


void updateStates();

//El periodo real del led sera 2*T*periodo.
//Donde T es el tiempo de interrupcion de ledsDriverUpdate
typedef struct{
    bool led_value;
    bool titilante_flag;
    unsigned char periodo;
    unsigned char contador;
    unsigned short reg;

}info_led_t;


static info_led_t leds[cantidadleds];


static int register_ [cantidadleds];

bool ledsDriverInit(){


    //Asocio los registros de la estructura de datos a cada pin
    register_[0]=LED_SD_STATE;


    for(int i=0;i<cantidadleds;i++){
        gpio_reset_pin(register_[i]);
        gpio_set_direction(register_[i], GPIO_MODE_OUTPUT);
        gpio_set_level(register_[i],0);

    }




    for(int i=0;i<cantidadleds;i++){
        leds[i].led_value = false;
        leds[i].titilante_flag = false;
        leds[i].periodo = 20; //Periodo por defecto de 2 segundos (20*200ms*2)
        leds[i].contador = leds[i].periodo;
        leds[i].reg=register_[i];
    }

    

    ledsDriverUpdate();

    // Inicialmente lo reflejo en el registro de estados


    


}

unsigned char ledsDriverUpdate(){

    updateStates();
    //Actualizo los estados de los leds
    for(int i=0;i<cantidadleds;i++){
        if(leds[i].titilante_flag==true){
            if(leds[i].contador==0){
                leds[i].led_value = !leds[i].led_value;
                leds[i].contador=leds[i].periodo;
            }
            leds[i].contador--;

        }

        gpio_set_level(LED_SD_STATE,leds[i].led_value);
    }

    


    //La idea es sincronizar los cambios de estado/titilante para que titilen todos juntos


    
 
    
    return 0;

    

}


void updateStates(){


    if(read_register(SD_STATE) == 0){
        leds[0].titilante_flag=false;
        leds[0].led_value=false;
    }else{
        leds[0].titilante_flag=true;
    }
    



}