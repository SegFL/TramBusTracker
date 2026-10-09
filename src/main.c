
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
#include "src/auxFunc/auxFunc.h"

//Si pongo 128 provoca un stackoverflow y no se porque
#define MAX_UART_BUFFER_SIZE 80         //Cantidad de bytes que lee la UART antena cada vez

//Secuencia de debug 
#define ESC_CHAR       0x1B
#define ESC_REQUIRED   3
#define ESC_TIMEOUT_US (2000 * 1000) // 2 segundos
bool debugConsoleEnabled=false; 



/*
----------------------------Variables
*/

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

typedef enum : uint8_t {
    buscando = 0x01,
    mensaje    = 0x02
} state_uart;

/*
----------------------------Tareas
*/
void serialTask();
void TAGTask();
void TAGFileTask(void* pvParameters);
void userTask(void* pvParameters);
void digitalInputsTask(void* pvParameters);
void digitalOutputsTask(void* pvParameters);
void IOTask(void* pvParameters);
void ledsDriverTask(void*pvParameters);
void transmitUartBridgeTASK(void* pvParameters);
void leerSensorInductivoTask(void* pvParameters);

/*
---------------------------Funciones privadas 
*/

void initUartAntena();
void initUartBridge();
void timer_callback(void *arg);

bool checksum(uint8_t *datos, size_t len);
bool validarFormatoTrama(PaqueteMensaje_t* txPacket);
bool configurarAntena();
bool TAGInit();
bool debugSequence();
void sendAck(uint8_t num_seq);
void sendNack(uint8_t num_seq);
void sendConfirmation(uint8_t num_seq, uint8_t confirmation_character);
bool procesarDatosTAG(const char *datos,
                   char *TAGProcesado,
                   size_t buffer_length,
                   char delimiter,
                   uint8_t num_bytes_to_extract);
void cargarArchivoConfiguracion();
void enviarTrama(const char *line);
int myReadUart(uart_port_t uart_num, void* buf, uint32_t length, TickType_t ticks_to_wait);
uint8_t buscarFinDeLinea(const uint8_t* data,uint8_t size);
uint16_t extractMiCrc(uint8_t* rx_buffer,uint8_t size);
void actualizarSensorInductivo();

esp_timer_handle_t timed_oneshot_timer;
esp_timer_create_args_t timed_oneshot_timer_args = {
    .callback = &timer_callback,
    .arg = NULL,
    .name = "oneshot_timer"
};





void app_main() {

    //Cambia el nivel de detalle de los mensajes impresos por la consola
    esp_log_level_set("*", ESP_LOG_NONE);

    //Si el usario envia la secuencia de debug enciendo los logs de errores
    if(debugSequence()==true){

        debugConsoleEnabled=true;
        write_register(FLAG_DEBUG_0,1);
        
        //esp_log_level_set("*", CONFIG_LOG_MAXIMUM_LEVEL_DEBUG);

        esp_log_level_set("*", ESP_LOG_VERBOSE);

    }

    ESP_LOGI("FIRMWARE", "Firmware version: %s", FIRMWARE_VERSION);
    //Primero necestio los pine sdigital habilitados porque los uso para ver si inicilizo la sd card o no

    xTaskCreate(digitalInputsTask,"digitalTask",2*1024,NULL,1,NULL);

    xTaskCreate(transmitUartBridgeTASK,"BRIDGETASK",2048,NULL,1,NULL);

    vTaskDelay(pdMS_TO_TICKS(100));

    xTaskCreate(TAGFileTask,"TAGFileTask",2*8192,NULL,1,NULL);

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




    xTaskCreate(digitalOutputsTask,"DOTask",1024,NULL,1,NULL);
    //Pongo un delay para evitar que se impriman mensajes del 
    //menu antes de que se terminene de inicializar el resto de las tareas
    xTaskCreate(ledsDriverTask,"userTask",2*2048,NULL,3,NULL);
    xTaskCreate(leerSensorInductivoTask,"leerSensorInductivo",1024,NULL,1,NULL);

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

    bool sd_in=false;

    while(1){
        
        switch(read_register(DI_CARD_DETECT)){

            //SD detectada
            case CARD_DETECTED:{
                if(read_register(SD_STATE)!=STATE_OK){
                    if(TAGInit()==true){
                        //Los registros se actualizan dentro de la funcion TAGInit
                        sd_in=true;
                    }
  

                }
            }break;

            case !CARD_DETECTED:{
                sd_in=false;
                write_register(SD_STATE,1);
                write_register(SD_FILE,1);
                write_register(MODO_INDUCTIVO_ENABLED,0);
            }break;
        }
        vTaskDelay(pdMS_TO_TICKS(10000));

    }
/*

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

*/
}
bool TAGInit(){
    bool sd = false, file = false;

    if(sd_card_init() == 0) {
        sd = true;
        write_register(SD_STATE, STATE_OK);

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
        

        cargarArchivoConfiguracion();
        

        return true;
    } else {
        write_register(SD_STATE, STATE_FAIL); 
        return false;
    }
}


void serialTask(void *pvParameters) {

    uint8_t rx_buffer[MAX_UART_BUFFER_SIZE];
    uint8_t aux=0;
    bool reciviendo_datos=false;
    bool carrier_return_received=false;
    int len = 0;
    bool init =true;
    state_uart estado = buscando;

    uint8_t nacks_counter =0;
    uint8_t num_seq_impar=0;
    for(;;){

        switch(estado){
            case buscando:{
                //Los ticks son cada 10ms en general, si poner un valor menor redondea a 0 y bloquea la tarea indefinidamente(WT)
                len = myReadUart(UART_ANTENA,rx_buffer,1,10);
                if(len==1 && rx_buffer[0]==START_CHARACTER){
                    estado = mensaje;
                }
        
            }break;
            case mensaje:{
                len = myReadUart(UART_ANTENA,rx_buffer,MAX_UART_BUFFER_SIZE,100);
                if(len>0){
                    uint8_t size = buscarFinDeLinea(rx_buffer,len);
                    if(size>7 && size<255){

                            //Si el num de seq es impar lo descarto
                        num_seq_impar = hexchar_to_val(rx_buffer[0])  & 0x01;

                        //extraer crc y pisar con un /0
                        uint16_t mycrc = extractMiCrc(rx_buffer+size-4,4);
                        aux=rx_buffer[size-4];
                        rx_buffer[size-4]=0;
                        uint16_t crc = calccrc(rx_buffer); 
                        rx_buffer[size-4]=aux;
                        ESP_LOGI("CRC mycrc", "CRC: 0x%04X", mycrc);
                        ESP_LOGI("CRC crc", "CRC: 0x%04X", crc);

                        if(init==true){//Me fijo si tengo que configurar la antena en un cierto modo( con CRC o sin CRC)
                            if((rx_buffer[0]=='0' )){
                                //Pisa el valor de la SD
                                    write_register(MODO_CRC_ENABLED,1);
                                    ESP_LOGI("Autoconfig","Entre en modo CRC automatico");
                            }
                            init=false;
                        }
                        if(read_register(MODO_CRC_ENABLED)==0){
                            //Si no estoy en modo CRC deberia enviar la trama sin el CRC(ultimos 4 bytes)
                            rx_buffer[size-4]=0;
                            enviarTrama(rx_buffer);
                        }else if(crc == mycrc && num_seq_impar == 0){//Estoy en modo CRC
                            sendConfirmation(rx_buffer[0],ACK_SEQUENCE );
                            enviarTrama(rx_buffer);
                            nacks_counter=0;
                        }else if( num_seq_impar == 0 ){
                            if(nacks_counter > 5){
                                sendConfirmation(rx_buffer[0],ACK_SEQUENCE );
                                enviarTrama(rx_buffer);
                                nacks_counter =0;
                            }else{
                                sendConfirmation(rx_buffer[0],NACK_SEQUENCE );
                                nacks_counter++;
                            }
                        }
                    }
                }
                estado=buscando;
            }break;
        }
    }

}



    


uint16_t extractMiCrc(uint8_t* rx_buffer,uint8_t size){

    uint16_t crc_buffer;
    uint16_t var[4];
    unsigned char i;
    if(rx_buffer==NULL)
        return 0;
    for(i=0;i<4;i++)
        var[i]=hexchar_to_val(rx_buffer[i]);
    //onvierto con shift, los chars
    var[0]=(var[0]<<12);
    var[1]=(var[1]<<8);
    var[2]=(var[2]<<4);
    crc_buffer=(var[0]|var[1]|var[2]|var[3]);          
    return crc_buffer;
}
// -----------------------------------------------------------------
// TASK 2: Receptora
// -----------------------------------------------------------------
void TAGTask(void *pvParameters) {
    PaqueteMensaje_t rxPacket;

    ESP_ERROR_CHECK(esp_timer_create(&timed_oneshot_timer_args, &timed_oneshot_timer));

    char TAGProcesado[SIZE_PAYLOAD];
    for (;;) {


        if (xQueueReceive(xQueueMsg, &rxPacket, portMAX_DELAY) == pdPASS) {
            rxPacket.datos[SIZE_PAYLOAD - 1] = '\0';
            if(read_register(FLAG_DEBUG_0)!=0)
                ESP_LOGI("TAGTask", "Mensaje recibido -> Tipo: 0x%02X | Datos: %s", rxPacket.tipo, rxPacket.datos);


            //Proceso el mensaje recibido
            if(procesarDatosTAG((const char*)rxPacket.datos,
                   TAGProcesado,
                   SIZE_PAYLOAD,
                   CHAR_DELIMITER,
                   BYTES_BEFORE_DELIMITTER)!=true)
            {
                if(read_register(FLAG_DEBUG_0)!=0)
                    ESP_LOGI("TAGTask", "Error al procesar TAG: %s", TAGProcesado);
            }else{
                  
                if(read_register(FLAG_DEBUG_0)!=0)
                    ESP_LOGI("TAGTask", "Se procesó TAG correctamente: %s", TAGProcesado);

                if (buscarTAG(TAGProcesado)) {

                    write_register(TRAMBUS_COUNTER, read_register(TRAMBUS_COUNTER) + 1);
                    if (esp_timer_is_active(timed_oneshot_timer)!=0) {
                        contador_TAGS_validos++;
                    } else {
                        contador_TAGS_validos = 1;

                        write_register(DO_1_STATE,1);
                        write_register(TRAMBUS_DETECTADO,1);
                        ESP_ERROR_CHECK(
                            esp_timer_start_once(
                                timed_oneshot_timer,
                                T_DEMANDA_TRAMBUS_VALIDO
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
}


void initUartAntena(){


    // =========================
    // UART (Antena)
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

    // ** ASIGNAR LOS PINES DEFINIDOS EN DEFINES.H **
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
                esp_timer_start_once(timed_oneshot_timer, T_DEMANDA_TRAMBUS_VALIDO)
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
/*
Envia un mensaje de confirmacion( NACK / ACK ) segun el caracter enviado como confirmation_caracter
*/

void sendConfirmation(uint8_t num_seq, uint8_t confirmation_character) {
    uint8_t buffer[8];

    // Calculo el crc
    buffer[0]='#';
    buffer[1] = num_seq;
    buffer[2] = confirmation_character;
    buffer[3] =0x60;
    buffer[4] =0x60;
    buffer[5] =0x60;
    buffer[6] =0x60;
    buffer[7] =0x0D;

   // buffer[2] = 0;
    /*uint16_t crc = calccrc(buffer); // CRC calculado sobre seq + confirmation_character
    // Construyo la trama
    uint8_t tx_frame[6];
    tx_frame[0] = START_CHARACTER;
    tx_frame[1] = num_seq;
    tx_frame[2] = confirmation_character;
    tx_frame[3] = crc >> 8;   // Byte alto del CRC
    tx_frame[4] = crc & 0xFF; // Byte bajo del CRC*/
   // tx_frame[5] = '\r'; 
    uart_write_bytes(UART_BRIDGE, (const char*)buffer, 8);
    uart_write_bytes(UART_ANTENA, (const char*)buffer, 8);
    uart_write_bytes(UART_DEBUG,  (const char*)buffer, 8);
}

bool configurarAntena(){

    ESP_LOGI("CONFIGURANDO ANTENA","NO HICE NADA");
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



bool debugSequence() {
    initUart();
    
    int escCount = 0;
    int64_t start = esp_timer_get_time();

    while ((esp_timer_get_time() - start) < ESC_TIMEOUT_US) {
        char c = readUserChar();
        if (c == ESC_CHAR) {
            escCount++;
            if (escCount >= ESC_REQUIRED) {
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); 
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


void initUartBridge(){

    
    // =========================
    // UART (Bridge)
    // =========================
    uart_config_t uart_config3 = (uart_config_t){
        .baud_rate = UART_BRIDGE_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0

    };
    uart_param_config(UART_BRIDGE, &uart_config3);

    //Asigno pines
    uart_set_pin(
        UART_BRIDGE,
        UART_BRIDGE_TX,     //TX
        UART_BRIDGE_RX,     //RX
        UART_PIN_NO_CHANGE, // RTS → sin usar
        UART_PIN_NO_CHANGE  // CTS → sin usar
    );

    esp_err_t err= uart_driver_install(UART_BRIDGE, UART_BRIDGE_BUFFER, 2048, 0, NULL, 0);

    if(err!=ESP_OK){
        write_register(SERIAL_TASK_STATE,2);
        ESP_LOGE("SerialTask", "Fallo al iniciar la UART bridge");
        return;
    }


}



bool procesarDatosTAG(const char *datos,
                      char *TAGProcesado,
                      size_t buffer_length,
                      char delimiter,
                      uint8_t num_bytes_to_extract)
{
    if (datos == NULL || TAGProcesado == NULL || buffer_length == 0) {
        return false;
    }

    TAGProcesado[0] = '\0';                      // salida definida ante cualquier fallo

    if (datos[0] == '\0' || delimiter == '\0') {
        return false;
    }
    datos++;                                     // salteo el caracter de inicio

    // Posición del delimitador (o fin de cadena)
    size_t pos = 0;
    while (datos[pos] != '\0' && datos[pos] != delimiter) {
        pos++;
    }

    if (read_register(FLAG_DEBUG_0) != 0) {
        ESP_LOGI("TAGTask", "pos=%u delim=0x%02X pedidos=%u buf=%u",
                 (unsigned)pos, (unsigned char)delimiter,
                 (unsigned)num_bytes_to_extract, (unsigned)buffer_length);
    }

    // Condiciones para que la extracción sea válida
    if (datos[pos] != delimiter) return false;                    // no hay delimitador
    if (pos < num_bytes_to_extract) return false;                 // faltan caracteres previos
    if ((size_t)num_bytes_to_extract > buffer_length - 1) return false;  // no entra en el buffer

    const size_t n = num_bytes_to_extract;

    // Copio los N caracteres inmediatamente anteriores al delimitador
    memcpy(TAGProcesado, datos + pos - n, n);
    TAGProcesado[n] = '\0';

#ifdef SALTEAR_0_INICIALES
    // "000011223344" -> "11223344" (si es todo ceros deja un solo '0')
    size_t skip = strspn(TAGProcesado, "0");
    if (skip == n && n > 0) {
        skip = n - 1;
    }
    if (skip > 0) {
        memmove(TAGProcesado, TAGProcesado + skip, n - skip + 1);
    }
#endif

    return true;
}

void cargarArchivoConfiguracion(){

    int state = buscarConfiguracion("CONFIG.TXT", "MODO_INDUCTIVO");
    if(state>0){
        write_register(MODO_INDUCTIVO_ENABLED,1);
        ESP_LOGI("MAIN", "MODO INDUCTIVO HABILITADO");
    }else{
        write_register(MODO_INDUCTIVO_ENABLED,0);
        ESP_LOGI("MAIN", "MODO INDUCTIVO DESHABILITADO");
    }


    int crc_state = buscarConfiguracion("CONFIG.TXT", "MODO_CRC");
    if(crc_state>0){
        write_register(MODO_CRC_ENABLED,1);
        ESP_LOGI("MAIN", "MODO CRC HABILITADO");
    }else{
        write_register(MODO_CRC_ENABLED,0);
        ESP_LOGI("MAIN", "MODO CRC DESHABILITADO");
    }




}


int myReadUart(uart_port_t uart_num, void* buf, uint32_t length, TickType_t ticks_to_wait){
    int len = uart_read_bytes(uart_num, buf, length, pdMS_TO_TICKS(ticks_to_wait));
    if (len < 0) {
        ESP_LOGE("UART", "Error al leer de la UART %d", uart_num);
      //  return -1;
   }else if (len >0) {

        uart_write_bytes(UART_BRIDGE, (const char *) buf, len);
        uart_write_bytes(UART_DEBUG, (const char *) buf, len);
        // No se recibieron datos en el tiempo de espera

   }


    return len;

}

void enviarTrama(const char *line)
{
    PaqueteMensaje_t txPacket;

    if (line == NULL) {
        return;
    }

    const bool debug = (read_register(FLAG_DEBUG_0) != 0);

    // Copio como mucho SIZE_PAYLOAD - 1 bytes para dejar espacio al /0
    size_t copy_len = strnlen(line, SIZE_PAYLOAD - 1);

    memset(&txPacket, 0, sizeof(txPacket));      
    memcpy(txPacket.datos, line, copy_len);
    txPacket.datos[copy_len] = '\0';            
    txPacket.tipo = MSG_TYPE_TAG_NUEVO;

    if (debug) {
        ESP_LOGI("UART_ANTENA", "Trama detectada (%u bytes): %s",
                 (unsigned)copy_len, (const char *)txPacket.datos);
    }

    if(read_register(MODO_INDUCTIVO_ENABLED)==1){
        //Si la entrada no esta activa me voy sin hacer nada, no envio el TAG a la cola
        if (!read_register(DI_1_STATE)) {
            if (debug) {
                ESP_LOGE("UART_ANTENA", "TAG recibido valido pero no hay entrada del sensor inductivo : %s",
                        (const char *)txPacket.datos);
            }
            return;
        }
    }



    // no bloquea, si la cola está llena se descarta
    if (xQueueSend(xQueueMsg, &txPacket, 0) != pdPASS) {
        if (debug) {
            ESP_LOGE("UART_ANTENA", "Cola llena, descartando TAG: %s",
                     (const char *)txPacket.datos);
        }
    } else if (debug) {
        ESP_LOGI("UART_ANTENA", "Enviado a cola: %s",
                 (const char *)txPacket.datos);
    }
}


void transmitUartBridgeTASK(void* pvParameters){

    //Buffer para el bridge ANTENA(422)-BRIDGE(232)
    uint8_t rx_buffer_2[MAX_UART_BUFFER_SIZE];


    initUartAntena();
    initUartBridge();


    for(;;){
        int len2 = uart_read_bytes(UART_BRIDGE,rx_buffer_2,sizeof(rx_buffer_2),pdMS_TO_TICKS(100));
        if(len2>0){
            uart_write_bytes(UART_ANTENA,(const char *)rx_buffer_2,len2);
            uart_write_bytes(UART_DEBUG,(const char *)rx_buffer_2,len2);
        }


        

    }

}



uint8_t buscarFinDeLinea(const uint8_t *data, uint8_t size)
{
    if (data == NULL || size < 2)
        return 0;

    uint8_t pos = 0;
    for (int i = 0; i < size - 1; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            pos = i;
            break;
        }
    }

    if (pos >= 0)
        ESP_LOGI("DEBUG", "Fin de linea encontrado en %"PRIu16, pos);
    else
        ESP_LOGI("DEBUG", "Fin de linea no encontrado");

    return pos;
}


void actualizarSensorInductivo(){

    unsigned char cambio=0,val=0;
    if(read_register(MODO_INDUCTIVO_ENABLED)==0){
        write_register(DO_3_STATE,1);
        write_register(DO_4_STATE,0);

    }else{

        for(int i=0;i<2;i++)
        {
            val=read_register(DI_1_STATE+i);
            if(val==1)
            {//encender el rele i
                cambio=1;
            }
            write_register(DO_3_STATE+i,val);
            
        }
        if(cambio==1)
        {//delay  5 segundos
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        //write_register(DO_3_STATE,0);
        //write_register(DO_4_STATE,0);
        //Apagar todos los Reles
    }


}

void leerSensorInductivoTask(void* pvParameters){


    for(;;){
        actualizarSensorInductivo();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

}
