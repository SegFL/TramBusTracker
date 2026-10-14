
Importante

En digitalOuptus se esta copiando la entrada de los sensores inductivos en la salida de reles. Deberia haber un filtro?Delay?

            gpio_set_level(DO_1,read_register(DO_1_STATE));
            gpio_set_level(DO_2,read_register(DO_2_STATE));
            gpio_set_level(DO_3,read_register(DI_1_STATE));
            gpio_set_level(DO_4,read_register(DI_2_STATE));




No se cambio la frecuencia del clock, fijarse si cambia en la temrinal al iniciar

#Cuand estoy en modo CRC tengo que esperar el timeout , ignorar el #00 de la retransmision
que envie la antena y esperar a que se reenvie el #Model... para recien ahi poder ver en que estado estoy