
#define FIRMWARE_VERSION "1.0.0"

#define LEER_ARCHIVO_RAM

//Puerto UART para consola (logs, info, debug)
#define UART_DEBUG UART_NUM_0



#define TAG_FILE "TAG.TXT"      //Nombre del archivo de TAGs



#define SIZE_PAYLOAD 128        //Tamaño maximo - 1 permitido de un TAG. O sea si el valor es 128 se permiten tags de hasta 127 caracteres.
                                // NO se incluyen en esta cuenta /n ni /r

//Boton / Sensor inductivo
#define DI_1 GPIO_NUM_36
#define DI_2 GPIO_NUM_39


//Salidas de demanda
#define DO_1 GPIO_NUM_19
#define DO_2 GPIO_NUM_18
//Reles
#define DO_3 GPIO_NUM_26
#define DO_4 GPIO_NUM_27

//Led Buil-In
#define LED_RUN GPIO_NUM_2   
//Led de estados
#define LED_SD_STATE GPIO_NUM_4    //Titila si falla la SD 
#define LED_SD_BASE_PERIOD 20       //Titila mas rapido si el archivo .txt esta mal

#define LED_TAG_STATE GPIO_NUM_21   //Led para saber cuando se esta buscando un TAG en la SD
#define LED_TAG_BASE_PERIOD 1

//SPI Pins para la SD
#define PIN_NUM_MISO 32//14
#define PIN_NUM_MOSI 25//13
#define PIN_NUM_CLK  33//27
#define PIN_NUM_CS   14     //26
#define PIN_NUM_DETECT 35   //Card detect
#define PIN_NUM_NWP 34      //Write protect

//Pines para la UART de la antena correspondientes a la UART_ANTENA
//ATENCION: NO USAR UART0 (CONSOLA DE DEBUG)
#define UART_ANTENA UART_NUM_2
#define UART_ANTENA_TX  23          //16
#define UART_ANTENA_RX  22          //17
#define UART_ANTENA_BAUD 9600
#define UART_ANTENA_BUFFER 2048

//Bridge ANTENA-DB9(232)
#define UART_BRIDGE UART_NUM_1
#define UART_BRIDGE_TX  17
#define UART_BRIDGE_RX  16
#define UART_BRIDGE_BAUD 9600
#define UART_BRIDGE_BUFFER 2048


//Tiempo que titila el led de LED_TAG_STATE al haber buscado un tag en la SD
#define T_OFF_REGISTER_LED_TAG 1000000

#define T_DEMANDA_TRAMBUS_VALIDO 10000000     //En ms

//Caracter inicial para considerar una trama TAG valida
#define START_CHARACTER '#'
//#define ACK_NACK_ENABLED
#define NACK_SEQUENCE '?'
#define ACK_SEQUENCE '@'

#define CHAR_DELIMITER '&'   //Caracter que separa los datos de un TAG
//Si el TAG tiene menos de BYTES_BEFORE_DELIMITTER caracteres los mensajes de error imprimen basura
#define BYTES_BEFORE_DELIMITTER 12   //Caracteres a extraer antes del CHAR_DELIMITER. Por ejemplo si el TAG es #12345678&ABCDEF&1234567890& y BYTES_BEFORE_DELIMITTER=8, se extrae 12345678
#define SALTEAR_0_INICIALES   //Si el TAG tiene 0s iniciales los ignora. Por ejemplo si el TAG es #000011223344&ABCDEF&1234567890& y SALTEAR_0_INICIALES esta definido, se extrae 11223344

//Sirve para bypasear toda la logica de la SD y los TAGS
//Si el sensor inductivo se activa automaticamente se activa el rele(DO) 
//para encender la antena.
//#define BYPASS_TAG_FILTER





//Secuencia de debug 
#define ESC_CHAR       0x1B
#define ESC_REQUIRED   3
#define ESC_TIMEOUT_US (2000 * 1000) // 2 segundos