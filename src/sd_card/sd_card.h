
#pragma once

#ifndef SD_CARD_H
#define SD_CARD_H

#include "esp_err.h"
#include <stdbool.h>
// Definición de tus pines
#define PIN_NUM_MISO 14
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  27
#define PIN_NUM_CS   26

#define MOUNT_POINT "/sdcard"

// Prototipo de la función de inicialización
esp_err_t sd_card_init(void);
void listarArchivosSD(void);
bool obtenerNombrePorIndiceSD(int indiceDeseado, char* nombreSalida, size_t maxLen);
void readTAGFile(const char * name_file);
bool buscarTAG(const char *tag_buscado);
#endif // SD_CARD_H