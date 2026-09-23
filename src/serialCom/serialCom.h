

#pragma once

#ifndef SERIAL_COM_H
#define SERIAL_COM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void initUart();
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