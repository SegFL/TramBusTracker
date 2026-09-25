    
//Este driver deberia encargarse de leer dataStruct
// y decidir que led hacer titilar y a que velocidad
#include "ledsDriver.h"
#include "esp_log.h"
#define cantidadleds 2
#define PERIODO_BASE_LEDS 20

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
    register_[1]=LED_TAG_STATE;
    gpio_reset_pin(register_[0]);
    gpio_set_direction(register_[0], GPIO_MODE_OUTPUT);
    gpio_set_level(register_[0],0);

    leds[0].led_value = false;
    leds[0].titilante_flag = false;
    leds[0].periodo = LED_SD_BASE_PERIOD;
    leds[0].contador = leds[0].periodo;
    leds[0].reg=register_[0];

    //Led de TAG
    gpio_reset_pin(register_[1]);
    gpio_set_direction(register_[1], GPIO_MODE_OUTPUT);
    gpio_set_level(register_[1],0);

    leds[1].led_value = false;
    leds[1].titilante_flag = false;
    leds[1].periodo = LED_TAG_BASE_PERIOD;
    leds[1].contador = leds[1].periodo;
    leds[1].reg=register_[1];

    

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

        gpio_set_level(register_[i],leds[i].led_value);
    }

    


    //La idea es sincronizar los cambios de estado/titilante para que titilen todos juntos


    
 
    
    return 0;

    

}

//Se modifican los registros de los leds, se acutalizan periodos de titilante y secuencias
void updateStates(){


    //Led SD
    if(read_register(SD_STATE) == 0){//Todo OK-> led apagado
        leds[0].titilante_flag=false;
        leds[0].led_value=false;
    }else if(read_register(SD_FILE)!=0){//Si el problema es el archivo titila rapido
        leds[0].titilante_flag=true;
        leds[0].periodo = LED_SD_BASE_PERIOD / 4;
    }else{                              //Si falla la SD pero no es el archivo titila lento
        leds[0].titilante_flag=true;
        leds[0].periodo = LED_SD_BASE_PERIOD ;
    }
    

    //Led TAG
    //Si solo aplico cambios cuando el contador es 0 hago que siempre se vea al menos un
    //ciclo del led
            
    
        if(read_register(TAGS_BUSCANDO_SD)==0){
            leds[1].titilante_flag=false;
            leds[1].led_value=false;
        }else{  
            leds[1].titilante_flag=true;
            leds[1].periodo = LED_TAG_BASE_PERIOD;
        }
    




}