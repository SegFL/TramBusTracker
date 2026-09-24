
/*

*/


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "sd_card.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "../serialCom/serialCom.h"
#include "../dataStruct/dataStruct.h"
#include "esp_timer.h"
#include  "../defines.h"
static const char *TAG = "SD_CARD";

//#define LEER_ARCHIVO_RAM

#define MAX_TAG_LENGTH 64

#ifdef LEER_ARCHIVO_RAM
    // Buffer de 200 TAGS de como máximo MAX_TAG_LENGTH caracteres
    char buffer[200][MAX_TAG_LENGTH];
    uint16_t total_lineas_ram = 0;
#endif

// Prototipos de funciones internas
void leer_archivo_sd(FILE* file);
void leer_archivo_ram(FILE* file);
esp_err_t sd_card_init(void) {
    esp_err_t ret;

    // Configuración del bus SPI y del Host
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // 1. Intentar inicializar el bus SPI. 
    // Si da ESP_ERR_INVALID_STATE (0x103), significa que el bus ya estaba listo.
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        if (read_register(FLAG_DEBUG_0) != 0) {
            ESP_LOGE(TAG, "Fallo al inicializar bus SPI: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // Configuración del slot SD
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    if (read_register(FLAG_DEBUG_0) != 0) {
        ESP_LOGI(TAG, "Iniciando montaje de tarjeta SD...");
    }

    // 2. Intentar montar el sistema de archivos
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        // Si fallò el montaje del filesystem,libero el bus SPI para reintentar mas adelante
        spi_bus_free(host.slot);
        
        if (read_register(FLAG_DEBUG_0) != 0) {
            ESP_LOGE(TAG, "Error [0x%X] al inicializar la tarjeta (%s).", ret, esp_err_to_name(ret));
        }
        return ret;
    }

    if (read_register(FLAG_DEBUG_0) != 0) {
        ESP_LOGI(TAG, "Sistema de archivos montado correctamente.");
    }
    return ESP_OK;
}

void listarArchivosSD(void) {
    DIR* dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        if(read_register(SD_STATE)){ESP_LOGE(TAG, "Error al abrir el directorio raíz (%s)\n", MOUNT_POINT);}
        return;
    }

    struct dirent* entry;
    if(read_register(SD_STATE)){ESP_LOGE(TAG,"--- Lista de Archivos en SD Card ---\n\r");}

    uint16_t cont = 1;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
            printFormat("%u %s\n\r", cont++, entry->d_name);
        }
    }

    closedir(dir);
    if(read_register(SD_STATE)){ESP_LOGE(TAG,"-------------------------------------\n");}
}

bool readTAGFile(const char * name_file){
    if (name_file == NULL) return false;

    char rutaCompleta[128];
    snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s", MOUNT_POINT, name_file);

    FILE* f = fopen(rutaCompleta, "r");
    if (f == NULL) {
        if(read_register(SD_STATE)){ESP_LOGE(TAG, "Error al abrir el archivo: %s", rutaCompleta);}
        return false;
    }

    if(read_register(SD_STATE)){ESP_LOGI(TAG, "Abriendo archivo: %s", rutaCompleta);}

#ifdef LEER_ARCHIVO_RAM
    leer_archivo_ram(f);
#else   
    leer_archivo_sd(f);
#endif

    fclose(f);
    return true;
}

void leer_archivo_sd(FILE* file){
    if (file == NULL) return;

    char linea[128];
    int64_t t_start = esp_timer_get_time();

    while (fgets(linea, sizeof(linea), file) != NULL) {
        linea[strcspn(linea, "\r\n")] = 0;
        if (strlen(linea) == 0) continue;

        if(read_register(FLAG_DEBUG_0)!=0){ESP_LOGI("TAG_FILE", "Nueva linea SD: %s", linea);}
    }

    int64_t t_end = esp_timer_get_time();
    if(read_register(FLAG_DEBUG_0)!=0){ESP_LOGI("PERF", "Tiempo total leyendo e imprimiendo directo desde SD: %lld us\n\r", (t_end - t_start));}
}

#ifdef LEER_ARCHIVO_RAM
void leer_archivo_ram(FILE* file){
    if (file == NULL) return;
    if(read_register(SD_STATE)!=0){
        if(read_register(FLAG_DEBUG_0)!=0)ESP_LOGE(TAG, "Error al leer el archivo en RAM, SD no inicilizada correctamente");
    }
    char linea[MAX_TAG_LENGTH];
    total_lineas_ram = 0;

    // 1. Cargar el archivo completo a la RAM y medir tiempo de transferencia
    int64_t t_start = esp_timer_get_time();

    while (fgets(linea, sizeof(linea), file) != NULL && total_lineas_ram < 200) {
        linea[strcspn(linea, "\r\n")] = 0;
        if (strlen(linea) == 0) continue;

        // Copiar la línea leída al buffer en RAM
        strncpy(buffer[total_lineas_ram], linea, sizeof(buffer[0]) - 1);
        buffer[total_lineas_ram][sizeof(buffer[0]) - 1] = '\0';
        total_lineas_ram++;
    }

    int64_t t_end = esp_timer_get_time();


    if(read_register(FLAG_DEBUG_0)!=0){
        ESP_LOGI("PERF", "Tiempo de carga desde SD a RAM: %lld us (%u lineas)", (t_end - t_start), total_lineas_ram);
        ESP_LOGE(TAG, "Error al buscar TAG, SD no inicilizada correctamente");
        for (uint16_t i = 0; i < total_lineas_ram; i++) {
            ESP_LOGI("TAG_RAM", "[%u] %s", i + 1, buffer[i]);
        }

    }

}
#endif


bool buscarTAG(const char *tag_buscado) {
    if (tag_buscado == NULL || strlen(tag_buscado) == 0) {
        return false;
    }
    if(read_register(SD_STATE)!=0){
        
        if(read_register(FLAG_DEBUG_0)!=0)ESP_LOGE(TAG, "Error al buscar TAG, SD no inicilizada correctamente");
        return false;
    }
        

#ifdef LEER_ARCHIVO_RAM
    // Búsqueda en el buffer precargado en memoria RAM
    for (uint16_t i = 0; i < total_lineas_ram; i++) {
        if (strcmp(buffer[i], tag_buscado) == 0) {
            if(read_register(FLAG_DEBUG_0)!=0)
                ESP_LOGI(TAG, "TAG encontrado en RAM [indice %u]: %s", i, tag_buscado);
            return true;
        }
    }
    if (read_register(FLAG_DEBUG_0) != 0){ESP_LOGW(TAG, "TAG no encontrado en RAM: %s", tag_buscado);}

    return false;

#else
    // Búsqueda alternativa leyendo directamente el archivo en la SD si no se usa RAM
    char rutaCompleta[128];
    snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s", MOUNT_POINT,TAG_FILE);

    FILE* f = fopen(rutaCompleta, "r");
    if (f == NULL) {
        write_register(SD_FILE,STATE_FAIL);
        if (read_register(FLAG_DEBUG_0) != 0){ESP_LOGE(TAG, "Error al abrir %s para buscar TAG", rutaCompleta);}

        ESP_LOGE(TAG, "Error al abrir %s para buscar TAG", rutaCompleta);
        return false;
    }
    write_register(SD_FILE,STATE_OK);


    char linea[MAX_TAG_LENGTH];
    bool encontrado = false;
    while (fgets(linea, sizeof(linea), f) != NULL) {
        linea[strcspn(linea, "\r\n")] = '\0';
        if (strcmp(linea, tag_buscado) == 0) {
            encontrado = true;
            break;
        }
    }

    //  Checkeo si termine el loop por ERROR o porque encontre/no encontre el TAG
    //Terminar con EOF no es un error
    if (ferror(f) && !feof(f)) {
        write_register(SD_FILE, SD_FILE_ERROR_READING_FILE);
        if (read_register(FLAG_DEBUG_0) != 0) {
            ESP_LOGE(TAG, "Error E/S al leer desde la SD durante la busqueda.");
        }
    } else {
        // Si salió por EOF (o si feof es true), el archivo está sano
        write_register(SD_FILE, STATE_OK);
    }


    fclose(f);
    return encontrado;
#endif
}