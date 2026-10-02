#pragma once


#include "driver/gpio.h"
#include "esp_log.h"

#include "../defines.h"
#include "../dataStruct/dataStruct.h"



void digitalInputsInit();
unsigned char digitalInputsUpdate();
unsigned long getHistorialEntrada(int entrada);