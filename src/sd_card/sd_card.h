
#pragma once

#ifndef SD_CARD_H
#define SD_CARD_H

#include "esp_err.h"
#include <stdbool.h>
// Definición de tus pines


#define MOUNT_POINT "/sdcard"

// Prototipo de la función de inicialización
esp_err_t sd_card_init(void);
void listarArchivosSD(void);
bool obtenerNombrePorIndiceSD(int indiceDeseado, char* nombreSalida, size_t maxLen);
bool readTAGFile(const char * name_file);
bool buscarTAG(const char *tag_buscado);
#endif // SD_CARD_H