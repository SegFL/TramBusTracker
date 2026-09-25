
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
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"


static const char *TAG = "SD_CARD";

//#define LEER_ARCHIVO_RAM

#define MAX_TAG_LENGTH 64

#ifdef LEER_ARCHIVO_RAM
    // Buffer de 200 TAGS de como máximo MAX_TAG_LENGTH caracteres
    char buffer[200][MAX_TAG_LENGTH];
    uint16_t total_lineas_ram = 0;
    void leer_archivo_ram(FILE* file);


#endif

// Prototipos de funciones internas
void leer_archivo_sd(FILE* file);

static bool s_bus_initialized = false;
static sdmmc_card_t *s_card = NULL;

void turnoff_register();
void turnon_register();
static uint16_t contador_busquedas_TAG=0;
esp_timer_handle_t timed_oneshot_timer_sd=NULL;
esp_timer_create_args_t timed_oneshot_timer_args_sd = {
    .callback = &turnoff_register,
    .arg = NULL,
    .name = "oneshot_timer_turnoff_register"
};

static void sd_card_force_spi_idle(void);


// Fuerza a la tarjeta SD a entrar en modo IDLE SPI antes del montaje
static void sd_card_force_spi_idle(void) {
    gpio_set_level(PIN_NUM_CS, 1); // CS deshabilitado
    
    // Enviamos 12 bytes dummy (96 pulsos de reloj) con MOSI en 1
    uint8_t dummy_bytes[12];
    memset(dummy_bytes, 0xFF, sizeof(dummy_bytes));

    spi_transaction_t t = {
        .length = sizeof(dummy_bytes) * 8,
        .tx_buffer = dummy_bytes,
    };
    
    // Usamos la API del driver SPI para transmitir los pulsos
    spi_device_handle_t spi_temp_handle;
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 400 * 1000, // 400 kHz (frecuencia de inicialización)
        .mode = 0,
        .spics_io_num = -1,           // Sin CS automático para controlar el pin manualmente
        .queue_size = 1,
    };

    if (spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_temp_handle) == ESP_OK) {
        spi_device_transmit(spi_temp_handle, &t);
        spi_bus_remove_device(spi_temp_handle);
    }
}

esp_err_t sd_card_init(void) {
    esp_err_t ret;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    // 1. Inicializar bus SPI una sola vez
    if (!s_bus_initialized) {
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = PIN_NUM_MOSI,
            .miso_io_num = PIN_NUM_MISO,
            .sclk_io_num = PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4000,
        };

        ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            if (read_register(FLAG_DEBUG_0) != 0) {
                ESP_LOGE(TAG, "Fallo al inicializar bus SPI: %s", esp_err_to_name(ret));
            }
            return ret;
        }

        // Habilitar pull-ups internos para prevenir ruido cuando no hay tarjeta insertada
        gpio_set_pull_mode(PIN_NUM_MISO, GPIO_PULLUP_ONLY);
        gpio_set_pull_mode(PIN_NUM_MOSI, GPIO_PULLUP_ONLY);
        gpio_set_pull_mode(PIN_NUM_CLK, GPIO_PULLUP_ONLY);
        gpio_set_pull_mode(PIN_NUM_CS, GPIO_PULLUP_ONLY);

        s_bus_initialized = true;
    }

    // 2. Limpieza de montajes previos
    if (s_card != NULL) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
        s_card = NULL;
    } else {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, NULL);
    }

    // 3. Secuencia de recuperación SPI (Pulsos de reloj dummy)
    sd_card_force_spi_idle();

    // Configuración del slot SD
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    if (read_register(FLAG_DEBUG_0) != 0) {
        ESP_LOGI(TAG, "Iniciando montaje de tarjeta SD...");
    }

    // 4. Montar sistema de archivos
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);

    if (ret != ESP_OK) {
        if (s_card != NULL) {
            esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
            s_card = NULL;
        }

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
    turnon_register();
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
    turnon_register();
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

void turnon_register(){
    if(timed_oneshot_timer_sd==NULL){
        ESP_ERROR_CHECK(esp_timer_create(&timed_oneshot_timer_args_sd, &timed_oneshot_timer_sd));

    }
    unsigned short c=read_register(CAR_COUNTER);
    c++;
    write_register(CAR_COUNTER,c);
    if (esp_timer_is_active(timed_oneshot_timer_sd)!=0) {
        contador_busquedas_TAG++;

    } else {
        contador_busquedas_TAG = 1;
        write_register(TAGS_BUSCANDO_SD,1); 


        ESP_ERROR_CHECK(
            esp_timer_start_once(
                timed_oneshot_timer_sd,
                1000000
            )
        );
    }   
}

void turnoff_register(void *arg){
    if (contador_busquedas_TAG > 0) {
        contador_busquedas_TAG--;

        // Si aún quedan TAGs pendientes por procesar:
        if (contador_busquedas_TAG > 0) {


            // Re disparo el timer para volver a contar 
            ESP_ERROR_CHECK(
                esp_timer_start_once(timed_oneshot_timer_sd, 1000000)
            );
        } else {
            write_register(TAGS_BUSCANDO_SD,0);

        }
    }
}
