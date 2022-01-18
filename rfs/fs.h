#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_

#include <stdlib.h>
#include <types.h>

#define FILE_NO_ACCESS          (0)
#define FILE_READ_ACCESS        (1 << 0)
#define FILE_WRITE_ACCESS       (1 << 1)
#define FILE_RW_ACCESS          (FILE_WRITE_ACCESS | FILE_READ_ACCESS)
#define FILE_EXEC_PERMISSION    (1 << 2)
#define FILE_MAP_PERMISSION     (1 << 3)

typedef struct Directory dir_t;
typedef struct File file_t;

struct Directory
{
	dir_t*   owner;
	dir_t*   sibling;
	dir_t*   dirs;
	file_t*  files;
	uint32_t refs;
	uint16_t flags;
	uint16_t len;
	char name[1];
};

struct File
{
	dir_t*   owner;
	file_t*  sibling;
    uint32_t refs;
    size_t   size;
    void*    data;
	uint16_t flags;
	uint16_t len;
	char     name[1];
};

dir_t* PathResolve(dir_t* cwd, const char *path, char **remaining);

file_t* GetFile(dir_t* cwd, const char* name);

file_t* CreateFile(dir_t* cwd, const char *path, void* data, size_t size, uint32_t flags);

int32_t DeleteFile(file_t* file);

int32_t FileOpen(file_t* file, int32_t mode);

int32_t FileClose(file_t* file);

#endif