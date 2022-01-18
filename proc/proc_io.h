/**
 * @file        proc_io.h
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        24 August, 2020
 * @brief       Proc File System IO functions Definition Header File
*/

#ifndef _PROC_IO_H_
#define _PROC_IO_H_

/* Includes ----------------------------------------------- */
#include <types.h>
#include <io_types.h>
#include <fcntl.h>
#include <connection.h>


/* Exported types ----------------------------------------- */



/* Exported constants ------------------------------------- */



/* Exported macros ---------------------------------------- */


/* Exported functions ------------------------------------- */

int32_t _io_ProcInfo(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset);

int32_t _io_FileOpen(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset);

int32_t _io_FileClose(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset);

int32_t _io_FileRead(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset);

int32_t _io_FileWrite(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset);

int32_t _io_FileShare(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset);

int32_t _io_FileSeek(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset);

int32_t _io_FileTruncate(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset);

void _io_CloseCallBack(connect_t* con);

#endif