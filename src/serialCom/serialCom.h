

#pragma once

#ifndef SERIAL_COM_H
#define SERIAL_COM_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h" 
#include "esp_err.h" 

#include "../defines.h"


#ifdef __cplusplus
extern "C" {
#endif

esp_err_t initUart();
void sendUartDataln(const uint8_t* data, size_t len) ;
void sendUartData(const uint8_t* data, size_t len)  ;
void writeSerialComln(const char* data);
void writeSerialCom(const char* data);
void clearScreen();
char readUserChar(void);
void updateUartBuffers();

void printFormat(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // SERIAL_COM_H