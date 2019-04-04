#include <lvgl.h>
#include "lv_filesys.h"
#include <SD.h>
#include <FS.h>
#include <Arduino.h>
#include "freertos/queue.h"
#include "lv_swatch.h"
#include "struct_def.h"
#include "board_def.h"

#define DEBUG_DECODE(...) Serial.printf( __VA_ARGS__ )

#ifndef DEBUG_DECODER
#define DEBUG_DECODER(...)
#endif



#if USE_LV_FILESYSTEM
typedef  FILE *pc_file_t;
#define f_dev SD
static lv_fs_res_t pcfs_open(void *file_p, const char *fn, lv_fs_mode_t mode);
static lv_fs_res_t pcfs_close(void *file_p);
static lv_fs_res_t pcfs_read(void *file_p, void *buf, uint32_t btr, uint32_t *br);
static lv_fs_res_t pcfs_seek(void *file_p, uint32_t pos);
static lv_fs_res_t pcfs_tell(void *file_p, uint32_t *pos_p);
#endif



extern xQueueHandle g_event_queue_handle;


bool isSDVaild()
{
    if (digitalRead(SD_DETECT)) {
        Serial.println("No Detect SD Card");
        return false;
    }
    return true;
}

bool sd_init()
{
    pinMode(SD_DETECT, INPUT_PULLUP);
    if (digitalRead(SD_DETECT)) {
        Serial.println("No Detect SD Card");
        return false;
    }

    if (!f_dev.begin(SD_CS)) {
        Serial.println("Card Mount Failed");
        return false;
    }

    uint8_t cardType = f_dev.cardType();

    if (cardType == CARD_NONE) {
        Serial.println("No SD_MMC card attached");
        return false;
    }
    Serial.print("SD_MMC Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = f_dev.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
    return true;
}


static void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    File root = fs.open(dirname);
    if (!root) {
        lv_file_list_add(NULL, 0); //failed to open directory
        return;
    }
    if (!root.isDirectory()) {
        lv_file_list_add(NULL, 0); // not a directory
        return;
    }
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            lv_file_list_add(file.name(), DIR_TYPE);
            if (levels) {
                listDir(fs, file.name(), levels - 1);
            }
        } else {
            lv_file_list_add(file.name(), FILE_TYPE);
        }
        file = root.openNextFile();
    }
}

void file_handle(void *f)
{
    file_struct_t *file = (file_struct_t *)f;
    switch (file->event) {
    case LVGL_FILE_SCAN:
        Serial.println("LVGL_FILE_SCAN");
        listDir(f_dev, "/", 1);
        break;
    default:
        break;
    }
}

#if USE_LV_FILESYSTEM
/**
 * Open a file from the PC
 * @param file_p pointer to a FILE* variable
 * @param fn name of the file.
 * @param mode element of 'fs_mode_t' enum or its 'OR' connection (e.g. FS_MODE_WR | FS_MODE_RD)
 * @return LV_FS_RES_OK: no error, the file is opened
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t pcfs_open(void *file_p, const char *fn, lv_fs_mode_t mode)
{

    const char *flags = "";
    if (mode == LV_FS_MODE_WR) flags = "wb";
    else if (mode == LV_FS_MODE_RD) flags = "rb";
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = "a+";

    /*Make the path relative to the current directory (the projects root folder)*/
    char buf[256];
    sprintf(buf, "/sdcard/%s", fn);
    DEBUG_DECODE("Open %s \n", buf);
    pc_file_t f = fopen(buf, flags);
    if ((long int)f <= 0) return LV_FS_RES_UNKNOWN;
    else {
        fseek(f, 0, SEEK_SET);
        /* 'file_p' is pointer to a file descriptor and
         * we need to store our file descriptor here*/
        pc_file_t *fp = (pc_file_t *)file_p;         /*Just avoid the confusing casings*/
        *fp = f;
    }
    return LV_FS_RES_OK;
}


/**
 * Close an opened file
 * @param file_p pointer to a FILE* variable. (opened with lv_ufs_open)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv__fs_res_t enum
 */
static lv_fs_res_t pcfs_close(void *file_p)
{
    pc_file_t *fp = (pc_file_t *)file_p;         /*Just avoid the confusing casings*/
    fclose(*fp);
    return LV_FS_RES_OK;
}

/**
 * Read data from an opened file
 * @param file_p pointer to a FILE variable.
 * @param buf pointer to a memory block where to store the read data
 * @param btr number of Bytes To Read
 * @param br the real number of read bytes (Byte Read)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv__fs_res_t enum
 */
static lv_fs_res_t pcfs_read(void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    pc_file_t *fp = (pc_file_t *)file_p;         /*Just avoid the confusing casings*/
    *br = fread(buf, 1, btr, *fp);
    return LV_FS_RES_OK;
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param file_p pointer to a FILE* variable. (opened with lv_ufs_open )
 * @param pos the new position of read write pointer
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv__fs_res_t enum
 */
static lv_fs_res_t pcfs_seek(void *file_p, uint32_t pos)
{
    pc_file_t *fp = (pc_file_t *)file_p;         /*Just avoid the confusing casings*/
    fseek(*fp, pos, SEEK_SET);
    return LV_FS_RES_OK;
}

/**
 * Give the position of the read write pointer
 * @param file_p pointer to a FILE* variable.
 * @param pos_p pointer to to store the result
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv__fs_res_t enum
 */
static lv_fs_res_t pcfs_tell(void *file_p, uint32_t *pos_p)
{
    pc_file_t *fp = (pc_file_t *)file_p;         /*Just avoid the confusing casings*/
    *pos_p = ftell(*fp);
    return LV_FS_RES_OK;
}

#endif


bool lv_filesystem_init()
{
    if (!sd_init()) {
        Serial.println("begin sd fail");
        return false;
    }

#if USE_LV_FILESYSTEM
    /* Add a simple drive to open images from PC*/
    lv_fs_drv_t pcfs_drv;                           /*A driver descriptor*/
    memset(&pcfs_drv, 0, sizeof(lv_fs_drv_t));      /*Initialization*/
    pcfs_drv.file_size = sizeof(pc_file_t);            /*Set up fields...*/
    pcfs_drv.letter = 'P';
    pcfs_drv.open = pcfs_open;
    pcfs_drv.close = pcfs_close;
    pcfs_drv.read = pcfs_read;
    pcfs_drv.seek = pcfs_seek;
    pcfs_drv.tell = pcfs_tell;
    lv_fs_add_drv(&pcfs_drv);
#endif
    return true;
}


