/**
 * @file        proc.h
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        24 August, 2020
 * @brief       Proc File System Definition Header File
*/

#ifndef _PROC_H_
#define _PROC_H_

/* Includes ----------------------------------------------- */
#include <types.h>
#include <fcntl.h>


/* Exported types ----------------------------------------- */
typedef struct Directory dir_t;
typedef struct File file_t;

struct Directory
{
	dir_t*   owner;
	dir_t*   sibling;
	dir_t*   dirs;
	file_t*  files;
	uint16_t refs;
	uint16_t len;
	char name[1];
};

struct File
{
	dir_t*   owner;
	file_t*  sibling;
    size_t   size;
    void*    data;
    uint16_t refs;
	uint16_t access;
    uint16_t permission;
	uint16_t len;
	char     name[1];
};


/* Exported constants ------------------------------------- */
#define FILE_EXEC_PERMISSION    1
#define FILE_MAP_PERMISSION     2


/* Exported macros ---------------------------------------- */


/* Exported functions ------------------------------------- */

int32_t ProcFileSystemBuild();

dir_t* ProcPathResolve(dir_t* cwd, const char* path, char** remaining);

file_t* ProcFileGet(dir_t* cwd, const char* path);

file_t* ProcFileCreate(dir_t* cwd, const char *path, void* data, size_t size, uint16_t access, uint16_t permission);

int32_t ProcFileDelete(file_t* file);

int32_t ProcFileOpen(file_t* file, int32_t mode);

int32_t ProcFileClose(file_t* file);

#endif