

enum data {
    LEDRUN_STATE=0,
    FLAG_DEBUG_0,           //Para mensajes de error
    FLAG_DEBUG_1,
    TAGS_TASK_STATE,
    SERIAL_TASK_STATE,
    SD_STATE,
    USER_INTERFACE_STATE,   //1 En falla y 0 OK
    DI_STATE,               //Estado del dirver de las entradas digitales (0=ok. 1 = fail)
    DI_1_STATE,             //Estado de la entrada digital luego de ser filtrada
    DIM_DATOS
};


unsigned short read_register( unsigned short reg);
void write_register(unsigned short reg,unsigned short value);