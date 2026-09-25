
#define FIRMWARE_VERSION "0.0.0"

//#define LEER_ARCHIVO_RAM



#define TAG_FILE "TAG.TXT"      //Nombre del archivo de TAGs



#define SIZE_PAYLOAD 128        //Tamaño maximo - 1 permitido de un TAG. O sea si el valor es 128 se permiten tags de hasta 127 caracteres.
                                // NO se incluyen en esta cuenta /n ni /r

//Boton / Sensor inductivo
#define DI_1 GPIO_NUM_23


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


//Pines para la UART de la antena correspondientes a la UART_ANTENA
//ATENCION: NO USAR UART0 (CONSOLA DE DEBUG)
#define UART_ANTENA UART_NUM_2
#define UART_ANTENA_TX  16
#define UART_ANTENA_RX  17
#define UART_ANTENA_BAUD 9600
#define UART_ANTENA_BUFFER 2048
