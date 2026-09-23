
#include "dataStruct.h"

struct{
    unsigned short data[DIM_DATOS];
}info_struct;


unsigned short read_register( unsigned short reg){
    
    return info_struct.data[reg];
}


void write_register(unsigned short reg,unsigned short value){
    //Se puede modificar cualquier registro menos el de DIM_DATOS

    if(reg >=DIM_DATOS)
        return;
    
    info_struct.data[reg]=value;
}