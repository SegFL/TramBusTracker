
#define FIRMWARE_VERSION "0.0.0"

//#define LEER_ARCHIVO_RAM



#define TAG_FILE "TAG.TXT"      //Nombre del archivo de TAGs



#define SIZE_PAYLOAD 128        //Tamaño maximo - 1 permitido de un TAG. O sea si el valor es 128 se permiten tags de hasta 127 caracteres.
                                // NO se incluyen en esta cuenta /n ni /r

//Boton / Sensor inductivo
#define DI_1 GPIO_NUM_36
#define DI_2 GPIO_NUM_39


//Salidas de demanda
#define DO_1 GPIO_NUM_18

//Led Buil-In
#define LED_RUN GPIO_NUM_2   
//Led de estados
#define LED_SD_STATE GPIO_NUM_19    //Titila si falla la SD 
#define LED_SD_BASE_PERIOD 20       //Titila mas rapido si el archivo .txt esta mal

#define LED_TAG_STATE GPIO_NUM_21   //Led para saber cuando se esta buscando un TAG en la SD
#define LED_TAG_BASE_PERIOD 1

//SPI Pins para la SD
#define PIN_NUM_MISO 14
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  27
#define PIN_NUM_CS   26
#define PIN_NUM_DETECT 35   //Card detect
#define PIN_NUM_NWP 34      //Write protect

//Pines para la UART de la antena correspondientes a la UART_ANTENA
//ATENCION: NO USAR UART0 (CONSOLA DE DEBUG)
#define UART_ANTENA UART_NUM_2
#define UART_ANTENA_TX  16
#define UART_ANTENA_RX  17
#define UART_ANTENA_BAUD 9600
#define UART_ANTENA_BUFFER 2048

//Bridge ANTENA-DB9(232)
#define UART_BRIDGE UART_NUM_1
#define UART_BRIDGE_TX  23
#define UART_BRIDGE_RX  22
#define UART_BRIDGE_BAUD 9600
#define UART_BRIDGE_BUFFER 2048


//Tiempo que titila el led de LED_TAG_STATE al haber buscado un tag en la SD
#define T_OFF_REGISTER_LED_TAG 1000000



//Caracter inicial para considerar una trama TAG valida
#define START_CHARACTER '#'
//#define CHECKSUM_DISABLED    
#define ACK_NACK_ENABLED
#define NACK_SEQUENCE "?"
#define ACK_SEQUENCE "@"

//Sirve para bypasear toda la logica de la SD y los TAGS
//Si el sensor inductivo se activa automaticamente se activa el rele(DO) 
//para encender la antena.
//#define BYPASS_TAG_FILTER