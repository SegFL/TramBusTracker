#pragma once

#define STATE_OK 0
#define STATE_FAIL 1
#define SD_FILE_ERROR_READING_FILE 2
enum data {
    LEDRUN_STATE=STATE_OK,
    FLAG_DEBUG_0,           //Para mensajes de error
    FLAG_DEBUG_1,           //Para imprimir por consola el estado de dataStruct
    TAGS_TASK_STATE,
    TAGS_BUSCANDO_SD,       //Esta en 1 cuando se esta buscando un TAG en la SD(RAM (1)/FLASH(2))
    SERIAL_TASK_STATE,      //Si pones 1 falla la uart de la antena, si pones 2 es el bridge
    ANTENA_CONFIG_STATE,    //Estado de la configuracion de la antena
    LEDS_DRIVER_STATE,      //Estado del driver de leds
    SD_STATE,               //Estado de la conexion con la SD
    SD_FILE,                //Estado de la lectura del archivo en la SD
    USER_INTERFACE_STATE,   //Estado de la interfaz de usuario conectada al USB
    DIGITAL_INPUTS_STATE,   //Estado del dirver de las entradas digitales (0=ok. 1 = fail)
    DI_1_STATE,             //Estado de la entrada digital luego de ser filtrada
    DI_2_STATE,             //Estado de la entrada digital luego de ser filtrada
    DIGITAL_OUTPUTS_STATE,  //Estado del driver de salidas digitales
    DO_1_STATE,             //Hacia el controlador
    DO_2_STATE,             //Hacia el controlador
    DO_3_STATE,             //Rele
    DO_4_STATE,             //Rele
    TRAMBUS_DETECTADO,      //0 Significa que no hay trambus 1 que si hay con TAG valido
    TRAMBUS_COUNTER,
    CAR_COUNTER,
    FORCE_OUTPUTS,          //Si esta en 1 significa que se estan forzando las salida digitales
    DO_1_FORCED_STATE,      //Hacia el controlador  
    DO_2_FORCED_STATE,      //Hacia el controlador
    DO_3_FORCED_STATE,      //Rele
    DO_4_FORCED_STATE,      //Rele
    DIM_DATOS
};


unsigned short read_register( unsigned short reg);
void write_register(unsigned short reg,unsigned short value);