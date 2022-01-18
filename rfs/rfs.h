/**
 * @file        rfs.h
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        28 May, 2020
 * @brief       Raw File System Definition Header File
*/

#ifndef _RFS_H_
#define _RFS_H_


/* Includes ----------------------------------------------- */
#include <types.h>



/* Exported types ----------------------------------------- */



/* Exported constants ------------------------------------- */



/* Exported macros ---------------------------------------- */



/* Private functions -------------------------------------- */



/* Exported functions ------------------------------------- */

int32_t RfsInit();

int32_t RfsDumpHeader();

int32_t RfsParse();

int32_t RfsGetRamInfo(void **addr, size_t *size);

uint32_t RfsDevicesCount();

int32_t RfsDeviceParse(uint32_t dev, char** name, void** addr, size_t* size);

uint32_t RfsFilesCount();

int32_t RfsFileParse(uint32_t file, char** name, int32_t* type, void** data, size_t *size);

int32_t RfsRunStartupScript();

int32_t RfsRegisterDevices();

int32_t RfsDelete();

char* RfsGetVersion();

char* RfsGetArch();

char* RfsGetMach();

#endif /* _RFS_H_ */
