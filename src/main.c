
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "src/userInterface/userInterface.h"
#include "src/sd_card/sd_card.h"
#include "freertos/queue.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "src/dataStruct/dataStruct.h"
#include "driver/gpio.h"
#include "src/digitalInputs/digitalInputs.h"
#include "defines.h"
#include "src/ledsDriver/ledsDriver.h"
#include "src/digitalOutputs/digitalOutputs.h"

#define MAX_UART_BUFFER_SIZE 64         //Cantidad de bytes que lee la UART antena cada vez


//Secuencia de debug 
#define ESC_CHAR       0x1B
#define ESC_REQUIRED   3
#define ESC_TIMEOUT_US (2000 * 1000) // 2 segundos
bool debugConsoleEnabled=false; 

void initUartAntena();
void timer_callback(void *arg);






typedef enum : uint8_t {
    MSG_TYPE_TAG_NUEVO = 0x01,
    MSG_TYPE_ESTADO    = 0x02,
    MSG_TYPE_ALARMA    = 0x03,
    MSG_TYPE_CONFIG    = 0x04
} type_msg;
// 2. Estructura del paquete
typedef struct {
    type_msg tipo;                      // 1 byte
    uint8_t datos[SIZE_PAYLOAD];      // 30 bytes de payload
} PaqueteMensaje_t;
//Cola de mensajes entre serialTask y TAGTask
QueueHandle_t xQueueMsg=NULL;
static uint16_t contador_TAGS_validos=0;

esp_timer_handle_t timed_oneshot_timer;
esp_timer_create_args_t timed_oneshot_timer_args = {
    .callback = &timer_callback,
    .arg = NULL,
    .name = "oneshot_timer"
};


void serialTask();
void TAGTask();

void TAGFileTask(void* pvParameters);
void userTask(void* pvParameters);
void digitalInputsTask(void* pvParameters);
void digitalOutputsTask(void* pvParameters);
void IOTask(void* pvParameters);
void ledsDriverTask(void*pvParameters);
//Funciones privadas 
bool checksum();
bool validarFormatoTrama(PaqueteMensaje_t* txPacket);
bool configurarAntena();
bool TAGInit();
bool debugSequence();

void app_main() {

    //Cambia el nivel de detalle de los mensajes impresos por la consola
    esp_log_level_set("*", ESP_LOG_NONE);

    //Si el usario envia la secuencia de debug enciendo los logs de errores
    if(debugSequence()==true){

        debugConsoleEnabled=true;
        write_register(FLAG_DEBUG_0,1);
        esp_log_level_set("*", ESP_LOG_VERBOSE);

    }

        





    ESP_LOGI("FIRMWARE", "Firmware version: %s", FIRMWARE_VERSION);

    // 1. Crear la cola con espacio para 10 paquetes
    xQueueMsg = xQueueCreate(10, sizeof(PaqueteMensaje_t));

    if (xQueueMsg != NULL) {
        // 2. Crear las dos tareas
        //La tarea que lee la UART antena tiene mas prioridad que la que consume los TAGs recividos
        xTaskCreate(serialTask,   "Task_Send", 2048*4, NULL, 2, NULL);
        xTaskCreate(TAGTask, "Task_Recv", 2048*4, NULL, 1, NULL);
    } else {
        write_register(SERIAL_TASK_STATE,1);
        write_register(TAGS_TASK_STATE,1);
        ESP_LOGE("MAIN", "Error al crear la cola de mensajes entre SerialTask y TAGTask.");
    }

    xTaskCreate(TAGFileTask,"TAGFileTask",2*8192,NULL,1,NULL);
    xTaskCreate(digitalInputsTask,"digitalTask",2*1024,NULL,1,NULL);
    xTaskCreate(digitalOutputsTask,"DOTask",1024,NULL,1,NULL);
    //Pongo un delay para evitar que se impriman mensajes del 
    //menu antes de que se terminene de inicializar el resto de las tareas
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(ledsDriverTask,"userTask",2*2048,NULL,3,NULL);
    if(debugConsoleEnabled==true)
        xTaskCreate(userTask,"userTask",2*2048,NULL,3,NULL);



    

    digitalOutputsInit();


    while(1){

        write_register(LEDRUN_STATE,read_register(LEDRUN_STATE)^0x01);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}


void TAGFileTask(void* pvParameters){



    write_register(SD_STATE,1);

    //Intento inicializar la SD, si falla se queda intentnado en un bucle infinito
    while(1){
        if (TAGInit() == true) {
            break; // SD y Archivo inicializados con éxito
        }
        if (read_register(FLAG_DEBUG_0) != 0) {
            ESP_LOGE("MAIN", "Error al inicializar la SD. Reintentando en 3s...");
        }      
        vTaskDelay(pdMS_TO_TICKS(3000));
        
    }


    



    while(1){
        // Si la SD o el archivo entraron en estado de fallo (por ejemplo, al extraer la SD)
        if (read_register(SD_STATE) != STATE_OK || read_register(SD_FILE) != STATE_OK) {
            if (read_register(FLAG_DEBUG_0) != 0) {
                ESP_LOGW("MAIN", "Fallo detectado en SD/Archivo. Intentando re-inicializar...");
            }
            
            // Reintentar reconexión limpia
            TAGInit();
        }
            
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}
bool TAGInit(){
    bool sd = false, file = false;

    if(sd_card_init() == 0) {
        sd = true;
    }

    if(sd == true){
        if(readTAGFile(TAG_FILE) == true){
            file = true;
            write_register(SD_FILE, STATE_OK);
        } else {
            write_register(SD_FILE, STATE_FAIL);
        }
    } else {
        write_register(SD_FILE, STATE_FAIL);
    }
    
    if(sd == true && file == true){
        write_register(SD_STATE, STATE_OK);
        if (read_register(FLAG_DEBUG_0) != 0) {
            ESP_LOGI("MAIN", "SD Inicializada correctamente.");
        }      
        return true;
    } else {
        write_register(SD_STATE, STATE_FAIL); // <-- Asegura marcar el error en el registro
        return false;
    }
}


void serialTask(void *pvParameters) {
    PaqueteMensaje_t txPacket;

    uint8_t line_buf[SIZE_PAYLOAD ]; 
    uint16_t line_len = 0;

    uint8_t rx_buffer[MAX_UART_BUFFER_SIZE];

    initUartAntena();

    for (;;) {
        int len = uart_read_bytes(
            UART_ANTENA,
            rx_buffer,
            sizeof(rx_buffer),
            pdMS_TO_TICKS(10)
        );

        for (int j = 0; j < len; j++) {
            uint8_t rx_byte = rx_buffer[j];

            // Ignorar caracteres  si es necesario
            if (rx_byte == '\r') {
                continue;
            }

            // Al encontrar fin de línea, procesamos la trama acumula
            if (rx_byte == '\n') {
                if (line_len > 0) { // Evito tramas vacías
                    memset(txPacket.datos, 0, SIZE_PAYLOAD);

                    // Acoto la longitud máxima al tamaño de txPacket.datos
                    uint16_t copy_len = (line_len < SIZE_PAYLOAD - 1) ? line_len : (SIZE_PAYLOAD - 1);
                    memcpy(txPacket.datos, line_buf, copy_len);
                    txPacket.datos[copy_len] = '\0';
                    txPacket.tipo = MSG_TYPE_TAG_NUEVO;

                    if (read_register(FLAG_DEBUG_0) != 0) {ESP_LOGI("UART_ANTENA", "Trama detectada (%d bytes): %s", copy_len, txPacket.datos);}
                    if(validarFormatoTrama(&txPacket)==true){
                        
                    
                        if(read_register(DI_1_STATE)){
                            //Espero hasta 50ms si la cola esta llena
                            if (xQueueSend(xQueueMsg, &txPacket, pdMS_TO_TICKS(50)) != pdPASS) {
                                if (read_register(FLAG_DEBUG_0) != 0) {ESP_LOGE("UART_ANTENA", "Error cola llena, decartando TAG:%s", txPacket.datos);}
                            } else {
                                if (read_register(FLAG_DEBUG_0) != 0){ESP_LOGI("UART_ANTENA", "ENVIANDO A COLA: %s", txPacket.datos);}
                            }
                        }

                    }
                    // Reiniciar el acumulador
                    line_len = 0;

                }
            } else {
                // Acumular byte en el buffer
                if (line_len < sizeof(line_buf) - 1) {
                    line_buf[line_len++] = rx_byte;
                } else {
                    // Si la trama supera el tamaño máximo permitido, descartar para evitar corrupción
                    if (read_register(FLAG_DEBUG_0) != 0){ESP_LOGI("UART_ANTENA", "Saturación de trama, descartando buffer");}
                    line_len = 0;
                }
            }
        }
    }
}

// -----------------------------------------------------------------
// TASK 2: Receptora
// -----------------------------------------------------------------
void TAGTask(void *pvParameters) {
    PaqueteMensaje_t rxPacket;

    ESP_ERROR_CHECK(esp_timer_create(&timed_oneshot_timer_args, &timed_oneshot_timer));

    for (;;) {
        if (xQueueReceive(xQueueMsg, &rxPacket, portMAX_DELAY) == pdPASS) {
            rxPacket.datos[SIZE_PAYLOAD - 1] = '\0';
            if(read_register(FLAG_DEBUG_0)!=0)
                ESP_LOGI("TAGTask", "Mensaje recibido -> Tipo: 0x%02X | Datos: %s", rxPacket.tipo, rxPacket.datos);

            if (buscarTAG((const char *)rxPacket.datos)) {

                unsigned short c=read_register(TRAMBUS_COUNTER);
                c++;
                write_register(TRAMBUS_COUNTER,c);
                if (esp_timer_is_active(timed_oneshot_timer)!=0) {
                    contador_TAGS_validos++;
                } else {
                    contador_TAGS_validos = 1;

                    write_register(DO_1_STATE,1);
                    write_register(TRAMBUS_DETECTADO,1);
                    ESP_ERROR_CHECK(
                        esp_timer_start_once(
                            timed_oneshot_timer,
                            3000000
                        )
                    );
                }

                if(read_register(FLAG_DEBUG_0)!=0){
                    ESP_LOGI("TAGTask", "----TAG TRAMBUS VALIDO----");
                }
            }else{
                if(read_register(FLAG_DEBUG_0)!=0)
                    ESP_LOGI("TAGTask", "----TAG TRAMBUS INVALIDO----");

            }
        }
    }
}


void initUartAntena(){


    // =========================
    // UART2 (Antena)
    // =========================
    uart_config_t uart_config2 = (uart_config_t){
        .baud_rate = UART_ANTENA_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0

    };
    uart_param_config(UART_ANTENA, &uart_config2);

    // ** ASIGNAR LOS PINES 16/17 **
    uart_set_pin(
        UART_ANTENA,
        UART_ANTENA_TX,     //TX
        UART_ANTENA_RX,     //RX
        UART_PIN_NO_CHANGE, // RTS → sin usar
        UART_PIN_NO_CHANGE  // CTS → sin usar
    );

    esp_err_t err= uart_driver_install(UART_ANTENA, UART_ANTENA_BUFFER, 256, 0, NULL, 0);

    if(err!=ESP_OK){
        write_register(SERIAL_TASK_STATE,1);
        ESP_LOGE("SerialTask", "Fallo al iniciar la UART antena");
        return;
    }

    if(configurarAntena()!=true){
        write_register(ANTENA_CONFIG_STATE,STATE_FAIL);
        if(read_register(FLAG_DEBUG_0))ESP_LOGE("SerialTask", "Uart antena error al configurar la antena");
    }else{
        ESP_LOGI("SerialTask", "Uart antena configurada correctamente");
    }

}




void userTask(void* pvParameters){


    if(userInterfaceInit()!=true){
        write_register(USER_INTERFACE_STATE,1);
        ESP_LOGE("userTask", "Error al inicializar la interfaz de usuario.");
    }



    for(;;){
        userInterfaceUpdate();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void timer_callback(void *arg){
    if (contador_TAGS_validos > 0) {
        contador_TAGS_validos--;
        if(read_register(FLAG_DEBUG_0)!=0)ESP_LOGE("callback timer", "TRAMBUS DETECTADO FIN");

        // Si aún quedan TAGs pendientes por procesar:
        if (contador_TAGS_validos > 0) {

            // Re disparo el timer para volver a contar 
            ESP_ERROR_CHECK(
                esp_timer_start_once(timed_oneshot_timer, 3000000)
            );
        } else {
            //Si no quedan tags validos dejo de contar y apago la salida.
            write_register(DO_1_STATE,0);
            write_register(TRAMBUS_DETECTADO,0);

        }
    }
}



void digitalInputsTask(void* pvParameters){

    digitalInputsInit();

    for(;;){

        digitalInputsUpdate();

        vTaskDelay(pdMS_TO_TICKS(100));

    }

}


bool validarFormatoTrama(PaqueteMensaje_t* txPacket){

    return true;


    if(txPacket==NULL)
        return false;
    
    if(checksum()==false)
        return false;


    return true;



}

bool checksum(){
    return true;

}

bool configurarAntena(){
    return true;
}


void digitalOutputsTask(void* pvParameters){


    if(digitalOutputsInit()==true){
        write_register(DIGITAL_OUTPUTS_STATE,STATE_OK);
    }else{
        write_register(DIGITAL_OUTPUTS_STATE,STATE_FAIL);
    }

    for(;;){
        if(read_register(DIGITAL_OUTPUTS_STATE)==STATE_OK)
            digitalOutputsUpdate();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



bool debugSequence(){

    initUart();
    

    int escCount = 0;
    int64_t start = esp_timer_get_time();

    while ((esp_timer_get_time() - start) < ESC_TIMEOUT_US) {
        char c = readUserChar();
        if (c == ESC_CHAR) {
            escCount++;
        }
    }

    if (escCount >= ESC_REQUIRED) {
        return true;
    }


    return false;
}


void ledsDriverTask(void*pvParameters){

    if(ledsDriverInit()==true){
        write_register(LEDS_DRIVER_STATE,STATE_OK);
    }else{
        write_register(LEDS_DRIVER_STATE,STATE_FAIL);

    }

    for(;;){
        if(read_register(LEDS_DRIVER_STATE)==STATE_OK)
            ledsDriverUpdate();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}