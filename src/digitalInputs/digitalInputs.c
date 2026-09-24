


#include "digitalInputs.h"
#include "driver/gpio.h"
#include "../defines.h"
#include "esp_log.h"

#include "../dataStruct/dataStruct.h"



#define NUM_BITS 32

// Cantidad de entradas digitales
#define NUM_ENTRADAS 1



// Vector de 32 bits para el historial de cada entrada
// Vectores globales
static uint32_t historialEntradas[NUM_ENTRADAS] = {0};


const gpio_num_t pines_entrada[NUM_ENTRADAS] = {DI_1};

void scanInputs();



void digitalInputsInit(){


    // Definir las máscaras de pines para las entradas digitales
    uint64_t input_pins_mask = (1ULL << DI_1 ) ;

    // Configuración de GPIO
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = input_pins_mask;          // Establecer los pines de entrada
    io_conf.mode = GPIO_MODE_INPUT;                  // Modo de entrada digital
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;        // Habilitar resistencia pull-up (opcional)
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   // Deshabilitar resistencia pull-down
    io_conf.intr_type = GPIO_INTR_DISABLE;          // Deshabilitar interrupciones (si no se necesitan)

    // Configurar GPIO
    if(gpio_config(&io_conf)!=ESP_OK){
        write_register(DIGITAL_INPUTS_STATE,1);
        ESP_LOGE("DigitalInputsInit", "Error al inicilizar las entradas digitales");
    }


    //Leo las entradas y actulizo la estructura general de datos
    scanInputs();

}


unsigned char digitalInputsUpdate(){

    scanInputs();

    

    return 0;

}

void scanInputs(){
    //Leo las entradas y actulizo el historial

    //Leo la entrada digital y guardo el valor. 
    //ATENCION: como la entrada se activa se activa con 0(pullup) el valor guardado esta negado --? 1 : 0--
    historialEntradas[0] = (historialEntradas[0] << 1) | ( gpio_get_level(DI_1) ? 0 : 1);
    
    //Filtro y actualizo los registros

    //Si el nuevo valor en 0, apago el registro
    if(!(historialEntradas[0] & 0x01)){
        write_register(DI_1_STATE,0);
    }else if((historialEntradas[0] & 0x000f )== 0x0f){
    //Si tengo 4 '1' seguidos lo tomo como una entrada valida

        write_register(DI_1_STATE,1);
    }
}

unsigned long getHistorialEntrada(int entrada){
    if(entrada<0 || entrada>=NUM_ENTRADAS)
        return 0;
    return historialEntradas[entrada];
}









