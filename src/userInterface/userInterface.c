

#include <ctype.h>
#include "esp_wifi.h"
#include <string.h>
#include "userInterface.h"
#include "../menuTree/menuTree.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../sd_card/sd_card.h"
#include "../dataStruct/dataStruct.h"


#define MAX_DATA_BUFFER 30



static MenuNode *menu = NULL;
char data_buffer[MAX_DATA_BUFFER] ; //Variable para almacenar los datos recibidos
unsigned char buffer_index = 0; //Índice para el buffer de datos
bool aceptandoDatos=false;
bool updateScreen=false;
static int lastMenuId = -1;

void printDataValues(void);
void printDataNames(void);
void moveCursor(int row, int col);


void procesarDatos(const char* data, unsigned char length) ;
static void onEnterNode(MenuNode* n);
static void onUpdateNode(MenuNode* n);
static bool nodeRequiresInput(int id);




bool userInterfaceInit(){

    initUart();
    clearScreen();//Borra mensajes del ESP32 al iniciar el programa
    menu=menuInit();
    if(menu){
        ESP_LOGI("userInterface", "Menu inicializado");
    }else{
        ESP_LOGI("userInterface", "Menu no inicializado");
    }


}


void userInterfaceUpdate() {
    if (menu == NULL) return;

    char charReceived = readUserChar();
    if(charReceived== GO_BACK){ // ESCAPE
        menuUpdate(charReceived, &menu);
        
        clearScreen();
        printNode(menu);
        onEnterNode(menu);
        lastMenuId = menu->id;
        
        //Si el nodo es nuevo y requiere datos, preparo el buffer para recibirlos
        //Si no es nuevo pero aun asi requiere datos(porque ya se enviaron datos previamente
        //y se quiere seguir enviando datos) tambien preparo el buffer
        aceptandoDatos = nodeRequiresInput(menu->id);

        
        return ;
    }
  

    if (charReceived == '\n') {
        if (aceptandoDatos) {
            // Terminamos de recibir datos
            data_buffer[buffer_index] = '\0';  // Terminador nulo
            procesarDatos(data_buffer,buffer_index);
            memset(data_buffer, 0, sizeof(data_buffer));
            buffer_index = 0;
            aceptandoDatos = false;
        } 
        return;
    }

   if (!aceptandoDatos) {

        
        menuUpdate(charReceived, &menu);

        if (lastMenuId != menu->id) {
            clearScreen();
            printNode(menu);
            onEnterNode(menu);
            lastMenuId = menu->id;
        }
        //Si el nodo es nuevo y requiere datos, preparo el buffer para recibirlos
        //Si no es nuevo pero aun asi requiere datos(porque ya se enviaron datos previamente
        //y se quiere seguir enviando datos) tambien preparo el buffer
        aceptandoDatos = nodeRequiresInput(menu->id);

    } else {
        // Captura de caracteres
        if(buffer_index < MAX_DATA_BUFFER - 1) {
            // Aceptamos solo números,caracteres , coma y espacios
            if (isdigit(charReceived) || isalpha(charReceived) || charReceived == ',' || isspace(charReceived)) {
                data_buffer[buffer_index++] = charReceived;
            } 
        }
    }




    // 🔹 Ejecutar siempre la lógica de actualización periódica
    onUpdateNode(menu);

    return;
}








void procesarDatos(const char* data, unsigned char length) {
    if (data == NULL || menu == NULL || length <= 0) { 
        return;
    }


/*
    if(menu->id == xx){
    
        //Accion a ejecutar al recibir datos en el nodo con id xx
        return;
    }
}
*/

    if(menu->id==12){
        
        if(strcasecmp(data, "y") == 0 || strcasecmp(data, "yes") == 0) {
            write_register(FLAG_DEBUG_0,1);
            esp_log_level_set("*", ESP_LOG_VERBOSE);
            ESP_LOGI("userInterface", "Modo debug 0 activado");

        } else if(strcasecmp(data, "n") == 0 || strcasecmp(data, "no") == 0) {
            write_register(FLAG_DEBUG_0,0);
            ESP_LOGI("userInterface", "Modo debug 0 desactivado");
            //Apago el LOG/impresion despues de imprimir el mensaje de desactivado(si no no se imprime)
            esp_log_level_set("*", ESP_LOG_NONE);


        } else {
            ESP_LOGI("userInterface", "Valor invalido. Use 'Y' para habilitar o 'N' para deshabilitar");
        }

    }

    if(menu->id==13){
        
        if(strcasecmp(data, "y") == 0 || strcasecmp(data, "yes") == 0) {
            write_register(FLAG_DEBUG_1,1);
            ESP_LOGI("userInterface", "Modo debug 1 activado");
            printDataNames();

        } else if(strcasecmp(data, "n") == 0 || strcasecmp(data, "no") == 0) {
            write_register(FLAG_DEBUG_1,0);
            esp_log_level_set("*", ESP_LOG_NONE);
            ESP_LOGI("userInterface", "Modo debug 1 desactivado");

        } else {
            ESP_LOGI("userInterface", "Valor invalido. Use 'Y' para habilitar o 'N' para deshabilitar");
        }

    }









}


static void onEnterNode(MenuNode* n) {
    if (!n) return;

    if (nodeRequiresInput(n->id)) {
        aceptandoDatos = false;
        memset(data_buffer, 0, sizeof(data_buffer));
        buffer_index = 0;
        //sendUartDataln("Nodo requiere entrada. Presiona 'ENTER' para comenzar.");

    } 

    // Acciones inmediatas (sin pedir datos) y automaticas en elupdate
    switch (n->id) {

 
                   
        /*
        casexx:  // Acción inmediata para el nodo con id xx
            break;
        */
       case 12:{

            char flag = read_register(FLAG_DEBUG_0);

            char msg[64];

            snprintf(msg, sizeof(msg),
                    "Flag de debug 0 :%s",
                    flag == 0 ? "DESACTIVADO" : "ACTIVADO");

            writeSerialComln(msg);


       }break;
       case 13:{

        /*
            char flag = read_register(FLAG_DEBUG_1);

            char msg[64];

            snprintf(msg, sizeof(msg),
                    "Flag de debug 1 :%s",
                    flag == 0 ? "DESACTIVADO" : "ACTIVADO");

            writeSerialComln(msg);*/
            printDataNames();
       }break;


        default:
            break;
    }

    // Nodos que requieren datos: activar captura y mostrar prompt
    if (nodeRequiresInput(n->id)) {
        aceptandoDatos = true;
        memset(data_buffer, 0, sizeof(data_buffer));
        buffer_index = 0;

        switch (n->id) {
            //case xx:  ESP_LOGI("userInterface", "Ingrese SSID y presione 'ENTER' para confirmar"); break;
            case 12: ESP_LOGI("userInterface", "Introdcuti <Y> para activar y <N> para desactivar el modo debug 0");break;
            default: break;
        }
    }
}

//Ejecuto acciones periódicas al estar en ciertos nodos
static void onUpdateNode(MenuNode* n) {
    if (!n) return;

    switch (n->id) {
        /*
        case xx :
            // Acción periódica para el nodo con id xx
            break;
        */
        case 13:{
                printDataValues();
        }break;


        default:
            // Otros menús no se refrescan constantemente
            break;
    }
}

static bool nodeRequiresInput(int id) {
    switch (id) {

        //case XX:  // Cambiar SSID

        case 12://Menu debug 0
        case 13://Menu debug 1

        return true;
      
        default:
            return false;
    }
}
void printDataNames(void)
{
    int i=1;
    moveCursor(i++, 1);
    writeSerialCom("LEDRUN_STATE");

    moveCursor(i++, 1);
    writeSerialCom("FLAG_DEBUG_0");

    moveCursor(i++, 1);
    writeSerialCom("FLAG_DEBUG_1");

    moveCursor(i++, 1);
    writeSerialCom("TAGS_TASK_STATE");

    moveCursor(i++, 1); writeSerialCom("TAGS_BUSCANDO_SD");    

    moveCursor(i++, 1);
    writeSerialCom("SERIAL_TASK_STATE");

    moveCursor(i++, 1);
    writeSerialCom("ANTENA_CONFIG_STATE");

    moveCursor(i++, 1);
    writeSerialCom("LEDS_DRIVER_STATE");

    moveCursor(i++, 1);
    writeSerialCom("SD_STATE");

    moveCursor(i++, 1);
    writeSerialCom("SD_FILE");

    moveCursor(i++, 1);
    writeSerialCom("USER_INTERFACE_STATE");

    moveCursor(i++, 1);
    writeSerialCom("DIGITAL_INPUTS_STATE");

    moveCursor(i++, 1);
    writeSerialCom("DI_1_STATE");

    moveCursor(i++, 1); 
    writeSerialCom("DIGITAL_OUTPUTS_STATE");  

    moveCursor(i++, 1); 
    writeSerialCom("DO_1_STATE");           

    moveCursor(i++, 1);
    writeSerialCom("TRAMBUS_DETECTADO");

    moveCursor(i++, 1); 
    writeSerialCom("TRAMBUS_COUNTER");        
    moveCursor(i++, 1); 
    writeSerialCom("CAR_COUNTER");         


}

void printDataValues(void)
{
    char buffer[20];
    int i=1;
 
    writeSerialCom("\033[s");   // guarda posición actual de la terminal (donde estaba el log normal)

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(LEDRUN_STATE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(FLAG_DEBUG_0));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(FLAG_DEBUG_1));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(TAGS_TASK_STATE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(TAGS_BUSCANDO_SD)); 
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(SERIAL_TASK_STATE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(ANTENA_CONFIG_STATE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(LEDS_DRIVER_STATE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(SD_STATE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(SD_FILE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(USER_INTERFACE_STATE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(DIGITAL_INPUTS_STATE));
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(DI_1_STATE));
    writeSerialComln(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(DIGITAL_OUTPUTS_STATE)); 
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(DO_1_STATE));           
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(TRAMBUS_DETECTADO));
    writeSerialComln(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(TRAMBUS_COUNTER));      
    writeSerialCom(buffer);

    moveCursor(i++, 35);
    snprintf(buffer, sizeof(buffer), "%u", read_register(CAR_COUNTER));        
    writeSerialCom(buffer);

    writeSerialCom("\033[u");   // restaura posición: el próximo mensaje de otro módulo continúa ahí



}

void moveCursor(int row, int col) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "\033[%d;%dH", row, col);
    writeSerialCom(buffer);

}


