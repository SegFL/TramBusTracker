


Importante

Esta linea CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_80=y en el sdkconfig no hace nada, sigo sin poder cambiar la frecuencia del clock


En digitalOuptus se esta copiando la entrada de los sensores inductivos en la salida de reles. Deberia haber un filtro?Delay?

            gpio_set_level(DO_1,read_register(DO_1_STATE));
            gpio_set_level(DO_2,read_register(DO_2_STATE));
            gpio_set_level(DO_3,read_register(DI_1_STATE));
            gpio_set_level(DO_4,read_register(DI_2_STATE));




No se cambio la frecuencia del clock, fijarse si cambia en la temrinal al iniciar

#Cuand estoy en modo CRC tengo que esperar el timeout , ignorar el #00 de la retransmision
que envie la antena y esperar a que se reenvie el #Model... para recien ahi poder ver en que estado estoy


Cuando en la SD tiene el modo CRC en 1 y la antena tambien esta en modo ECP, se pierde el primer mensaje
#0Model, salta el timeout, se pierde otra vez un #0Model y recien ahi se detecta que el modo y se sale del
bucle de configuracion detectando correctamente el modo y saliendo

Deberia chequear bien la secuencia de inicilizacion de los drivers de las tareas.
Deberia hacer los inits a mano a iniciar los drivers


Si se pone la SD despues de inicilizar el sistema no se detecta automaticamente el modo crc ni la configuracion, deberia poner una interrupcion por flanco positivo con el card detect( si hago lo mismo que con el modo crc inicilizando la vaiable en 0xff puedo solucionar este tema)

Cuando esta iniciando la esp32 se prende el rele por un instante.

En modo CRC si fuerzo una salida de rele desde la consola se activa y sobreescribe al instante.


El modo CRC en 1 no esta del todo probado