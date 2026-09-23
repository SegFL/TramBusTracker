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

static const char *TAG = "SD_CARD";

#define LEER_ARCHIVO_RAM

#ifdef LEER_ARCHIVO_RAM
    // Buffer de 200 TAGS de como máximo 64 caracteres
    char buffer[200][64];
    uint16_t total_lineas_ram = 0;
#endif

// Prototipos de funciones internas
void leer_archivo_sd(FILE* file);
void leer_archivo_ram(FILE* file);

esp_err_t sd_card_init(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Iniciando montaje de tarjeta SD...");

    // 1. Configuración de montaje FATFS
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;

    // 2. Configuración del bus SPI
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error [0x%X] al inicializar el bus SPI (%s)", ret, esp_err_to_name(ret));
        return ret;
    }

    // 3. Configuración del slot SD sobre SPI
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    // 4. Intento de montaje en el VFS
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Error: Fallo al montar el sistema de archivos (¿La SD está en FAT32?).");
        } else if (ret == ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "Error: Timeout al comunicarse con la SD.");
        } else if (ret == ESP_ERR_INVALID_RESPONSE) {
            ESP_LOGE(TAG, "Error: Respuesta inválida de la SD.");
        } else {
            ESP_LOGE(TAG, "Error [0x%X] al inicializar la tarjeta (%s).", ret, esp_err_to_name(ret));
        }
        
        spi_bus_free(SPI2_HOST);
        return ret;
    }

    ESP_LOGI(TAG, "SD inicializada y montada con éxito en '%s'", MOUNT_POINT);
    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}

void listarArchivosSD(void) {
    DIR* dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        printFormat("Error al abrir el directorio raíz (%s)\n", MOUNT_POINT);
        return;
    }

    struct dirent* entry;
    printFormat("--- Lista de Archivos en SD Card ---\n\r");

    uint16_t cont = 1;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
            printFormat("%u %s\n\r", cont++, entry->d_name);
        }
    }

    closedir(dir);
    printFormat("-------------------------------------\n");
}

void readTAGFile(const char * name_file){
    if (name_file == NULL) return;

    char rutaCompleta[128];
    snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s", MOUNT_POINT, name_file);

    FILE* f = fopen(rutaCompleta, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Error al abrir el archivo: %s", rutaCompleta);
        return;
    }

    ESP_LOGI(TAG, "Abriendo archivo: %s", rutaCompleta);

#ifdef LEER_ARCHIVO_RAM
    leer_archivo_ram(f);
#else   
    leer_archivo_sd(f);
#endif

    fclose(f);
}

void leer_archivo_sd(FILE* file){
    if (file == NULL) return;

    char linea[128];
    int64_t t_start = esp_timer_get_time();

    while (fgets(linea, sizeof(linea), file) != NULL) {
        linea[strcspn(linea, "\r\n")] = 0;
        if (strlen(linea) == 0) continue;

        ESP_LOGI("TAG_FILE", "Nueva linea SD: %s", linea);
    }

    int64_t t_end = esp_timer_get_time();
    ESP_LOGI("PERF", "Tiempo total leyendo e imprimiendo directo desde SD: %lld us\n\r", (t_end - t_start));
}

#ifdef LEER_ARCHIVO_RAM
void leer_archivo_ram(FILE* file){
    if (file == NULL) return;

    char linea[64];
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

    ESP_LOGI("PERF", "Tiempo de carga desde SD a RAM: %lld us (%u lineas)", (t_end - t_start), total_lineas_ram);

    // 2. Imprimir las líneas almacenadas en RAM
    printFormat("--- Lineas cargadas en RAM ---\n\r");
    for (uint16_t i = 0; i < total_lineas_ram; i++) {
        ESP_LOGI("TAG_RAM", "[%u] %s", i + 1, buffer[i]);
    }
}
#endif


bool buscarTAG(const char *tag_buscado) {
    if (tag_buscado == NULL || strlen(tag_buscado) == 0) {
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
    if (read_register(FLAG_DEBUG_0) != 0){    ESP_LOGW(TAG, "TAG no encontrado en RAM: %s", tag_buscado);}

    ESP_LOGW(TAG, "TAG no encontrado en RAM: %s", tag_buscado);
    return false;

#else
    // Búsqueda alternativa leyendo directamente el archivo en la SD si no se usa RAM
    char rutaCompleta[128];
    snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/TAG.TXT", MOUNT_POINT);

    FILE* f = fopen(rutaCompleta, "r");
    if (f == NULL) {
        if (read_register(FLAG_DEBUG_0) != 0){ESP_LOGE(TAG, "Error al abrir %s para buscar TAG", rutaCompleta);}

        ESP_LOGE(TAG, "Error al abrir %s para buscar TAG", rutaCompleta);
        return false;
    }

    char linea[64];
    bool encontrado = false;

    while (fgets(linea, sizeof(linea), f) != NULL) {
        linea[strcspn(linea, "\r\n")] = '\0';
        if (strcmp(linea, tag_buscado) == 0) {
            encontrado = true;
            break;
        }
    }

    fclose(f);
    return encontrado;
#endif
}