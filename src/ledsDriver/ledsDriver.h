#pragma once


#include  "driver/gpio.h"
#include "esp_log.h"


#include "../defines.h"
#include "../dataStruct/dataStruct.h"

bool ledsDriverInit();
unsigned char ledsDriverUpdate();
