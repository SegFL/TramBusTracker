
#pragma once

#ifndef SD_CARD_H
#define SD_CARD_H


#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "esp_timer.h"


#include "../serialCom/serialCom.h"
#include "../dataStruct/dataStruct.h"
#include  "../defines.h"

// Definición de tus pines


#define MOUNT_POINT "/sdcard"

// Prototipo de la función de inicialización
esp_err_t sd_card_init(void);
void listarArchivosSD(void);
bool obtenerNombrePorIndiceSD(int indiceDeseado, char* nombreSalida, size_t maxLen);
bool readTAGFile(const char * name_file);
bool buscarTAG(const char *tag_buscado);
#endif // SD_CARD_H