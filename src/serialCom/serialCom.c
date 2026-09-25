
#include "serialCom.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h" 
uart_config_t uart_config1;

bool consolaIniciada=false;

//Puerto UART para consola (logs, info, debug)
#define UART_DEBUG UART_NUM_0


void print(const char* str) ;


esp_err_t initUart(void)
{
    esp_err_t ret;

    // Guarda: ya inicializada -> verificar consistencia y salir
    if (consolaIniciada) {
        if (uart_is_driver_installed(UART_DEBUG)) {
            ESP_LOGW("SerialCom", "UART ya inicializada, se omite reinit");
            return ESP_OK;
        } else {
            // Bandera desincronizada del estado real del driver
            ESP_LOGW("SerialCom", "Flag inconsistente, reinicializando");
            consolaIniciada = false;
        }
    }

    // ===== UART0 (CONSOLA) =====
    uart_config1 = (uart_config_t){
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0
    };

    ret = uart_param_config(UART_DEBUG, &uart_config1);
    if (ret != ESP_OK) {
        ESP_LOGE("SerialCom", "uart_param_config fallo: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_driver_install(UART_DEBUG, 1024, 1024, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("SerialCom", "uart_driver_install fallo: %s", esp_err_to_name(ret));
        return ret;
    }

    consolaIniciada = true;
    ESP_LOGI("SerialCom", "Consola iniciada");
    return ESP_OK;
}



void sendUartDataln(const uint8_t* data, size_t len) {
    static const uint8_t crlf[2] = { '\r', '\n' };

    sendUartData(data, len);
    sendUartData(crlf, sizeof(crlf));
}



void sendUartData(const uint8_t* data, size_t len) {
    uart_write_bytes(UART_DEBUG, data, len);


}



void writeSerialCom(const char* data)
{
    print(data);
    //uart_write_bytes(UART_DEBUG, data, strlen(data));

}

void writeSerialComln(const char* data){
    writeSerialCom(data);
    writeSerialCom("\n\r");
}



void clearScreen() {
    writeSerialCom("\033[2J\033[H");  // Borra pantalla ANSI
}






//Funcion para leer de a un caracter de la CONSOLA

// Función para leer un caracter de la consola o TCP
char readUserChar(void)
{
    uint8_t ch;

    int len = uart_read_bytes(UART_DEBUG, &ch, 1, 0);
    if (len > 0) {
        // Echo del carácter
        uart_write_bytes(UART_DEBUG, (const char *)&ch, 1);

        // Si la terminal envía \r, lo descartamos si viene seguido de \n,
        // lo convierto a \n directamente.
        if (ch == '\r') {
            return '\n'; // Normaliza cualquier Enter (\r o \n) a '\n'
        }

        return (char)ch;
    }

    return '\0';
}





void print(const char* str) {
    if(str!=NULL){
        sendUartData((const uint8_t*)str, strlen(str));
    }
}void printFormat(const char* format, ...) {
    char buffer[128]; // Ajusta el tamaño según tus necesidades
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    print(buffer);
}

