/**
 * @file        proc.c
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        24 August, 2020
 * @brief       Proc File System implementation
*/

/* Includes ----------------------------------------------- */
#include <proc.h>
#include <rfs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mman.h>


/* Private types ------------------------------------------ */


/* Private constants -------------------------------------- */
#define FILE_ACCESS_MASK    (0x3)
#define SYS_FILE            "/sys"
#define DEVICES_FILE        "/devices"
#define BOOT_FILES_PATH     "/boot/"


/* Private macros ----------------------------------------- */


/* Private variables -------------------------------------- */

// Proc root directory /proc/
static dir_t root;


/* Private function prototypes ---------------------------- */

int32_t ProcCreateSysFile()
{
    // Allocate memory to create the /proc/sys file
    char* sys = (char*)malloc(sizeof(char) * 64);

    // Get Ram data
    void* ramBase = NULL;
    size_t ramSize = 0;
    RfsGetRamInfo(&ramBase, &ramSize);

    // Create File
    size_t size = sprintf(sys, "Version: %s\nArch: %s\nMach: %s\nRam Size: 0x%x\n", RfsGetVersion(), RfsGetArch(), RfsGetMach(), ramSize);

    if(ProcFileCreate(NULL, SYS_FILE, sys, size, O_RDONLY, 0x0) == NULL)
    {
        return E_ERROR;
    }

    return E_OK;
}

int32_t ProcCreateDevicesFile()
{
    const size_t DEVICE_SIZE = 64;
    // Allocate memory to create the /proc/devices file (64 bytes per device)
    char* devices = (char*)malloc(DEVICE_SIZE * RfsDevicesCount());
    // Get devices
    char* ptr = devices;
    uint32_t i;
    for(i = 0; i < RfsDevicesCount(); i++)
    {
        char* name;
        void* addr;
        size_t size;
        RfsDeviceParse(i, &name, &addr, &size);
        // Ignore null character
        ptr += sprintf(ptr, "{\"%s\", @0x%x, 0x%x}\n", name, addr, size);
    }

    if(ProcFileCreate(NULL, DEVICES_FILE, devices, (++ptr - devices), O_RDONLY, 0x0) == NULL)
    {
        return E_ERROR;
    }

    return E_OK;
}

int32_t ProcCreateBootDir()
{
    uint32_t i;
    for(i = 0; i < RfsFilesCount(); i++)
    {
        char path[256];
        char* name;
        int32_t type;
        void* data;
        size_t size;
        // Get Files from Raw File System
        RfsFileParse(i, &name, &type, &data, &size);
        // Allocate Memory to copy file
        void* dataCopy = (void*)mmap(NULL, size, (PROT_READ |PROT_WRITE), (MAP_ANON | MAP_SHARED), NOFD, 0x0);
        // Copy file
        memcpy(dataCopy, data, size);
        // Generate full path for file
        sprintf(path, "%s%s", BOOT_FILES_PATH, name);

        if(ProcFileCreate(NULL, path, dataCopy, size, O_RDWR, FILE_MAP_PERMISSION | FILE_EXEC_PERMISSION) == NULL)
        {
            return E_ERROR;
        }
    }

    return E_OK;
}

void ProcDirectoryAdd(dir_t *parent, dir_t *child)
{
    child->owner = parent;
    child->sibling = parent->dirs;
    parent->dirs = child;
}

dir_t* ProcDirectoryCreate(dir_t *parent, char *path, char **remaining)
{
    uint32_t len = 0;
    for( ; path[len] && ('/' != path[len]); len++) {}

    if(path[len] == 0)
    {
        return NULL;
    }

    dir_t *current = (dir_t*)malloc(sizeof(dir_t) + len);
    current->len = len;
    current->dirs = NULL;
    current->files = NULL;

    memcpy(current->name, path, len);

    ProcDirectoryAdd(parent, current);

    *remaining = (path + len + 1);

    return current;
}

file_t* ProcFileAdd(dir_t* parent, char *name, void* data, size_t size, uint16_t access, uint16_t permission)
{
    uint32_t len = strlen(name);
    file_t* file = (file_t*)malloc(sizeof(file_t) + len);
    file->owner = parent;
    file->refs = 0;
    file->size = size;
    file->data = data;
    file->access = access;
    file->permission = permission;
    file->len = len;
    memcpy(file->name, name, len);
    file->sibling = parent->files;
    parent->files = file;

    return file;
}

/* Private functions -------------------------------------- */

int32_t ProcFileSystemBuild()
{
    // Get and parse Raw File System
    if(RfsInit() != E_OK)
    {
        return E_ERROR;
    }

    int32_t ret = E_OK;

    if((RfsParse() != E_OK) || (ProcCreateSysFile() != E_OK) || (ProcCreateDevicesFile() != E_OK) || (ProcCreateBootDir() != E_OK))
    {
        ret = E_ERROR;
    }

    // No longer needed so release memory
    RfsDelete();

    return ret;
}

dir_t* ProcPathResolve(dir_t* cwd, const char* path, char** remaining)
{
    dir_t *parent = cwd;

    if(cwd == NULL)
    {
        parent = &root;
    }

    if(path == NULL)
    {
        return parent;
    }
    
    dir_t *current;

    char *ptr = (char*)path;

    while(*ptr)
    {
        // Skip '/'
        if(ptr[0] == '/')
        {
            ptr++;
        }
        else if(ptr != path) // It's not mandatory to have the '/' at the beginning
        {
            *remaining = ptr;
            break;
        }

        uint32_t len = 0;
        for( ; ptr[len] && ('/' != ptr[len]); len++) {}
        
        for(current = parent->dirs; current != NULL; current = current->sibling)
        {
            if(current->len == len && !memcmp(ptr, current->name, len))
            {
                break;
            }
        }

        if(current == NULL)
        {
            *remaining = ptr;
            return parent;
        }

        ptr += len;
        parent = current;
    }

    if(ptr[0] == 0)
    {
        *remaining = NULL;
    }

    return parent;
}

file_t* ProcFileGet(dir_t* cwd, const char* path)
{
    // Get path length
    uint32_t length = strlen(path);

    // Last character has to be different from '/' but first character has to be '/'
    if((path[length - 1] == '/'))
    {
        return NULL;
    }

    // Fisrt we need to find the name space where the server is registered
    char* remaining;
    dir_t* parent = ProcPathResolve(cwd, path, &remaining);

    if(remaining == NULL)
    {
        // Nothing left to search for a server
        return NULL;
    }

    for(length = 0; remaining[length] != '\0'; length++)
    {
        if(remaining[length] == '/')
        {
            // We couldn't resolve the whole path
            return NULL;
        }
    }

    // Search for the server in the name space
    file_t *file;
    for(file = parent->files; file != NULL; file = file->sibling)
    {
        if(file->len == length && !memcmp(remaining, file->name, length))
        {
            return file;
        }
    }

    return NULL;
}

file_t* ProcFileCreate(dir_t* cwd, const char *path, void* data, size_t size, uint16_t access, uint16_t permission)
{
    // Get path length
    uint32_t length = strlen(path);

    // Last character has to be different from '/' but first character has to be '/'
    if(/*(path[0] != '/') || */(path[length - 1] == '/'))
    {
        return NULL;
    }

    // Get the last namespace in the specified path
    char *remaining;
    dir_t* parent = ProcPathResolve(cwd, path, &remaining);
    
    // If we consumed the whole path there is nothing more to be registered...
    // The path is invalid
    if(*remaining == 0)
    {
        return NULL;
    }
    
    // Create directories if required
    while(1)
    {
        dir_t *current = ProcDirectoryCreate(parent, remaining, &remaining);

        // All namesapces where resolved
        if(current == NULL)
        {
            break;
        }

        parent = current;
    }
    
    // At this point we only have the name of the server left
    return ProcFileAdd(parent, remaining, data, size, access, permission);
}

int32_t ProcFileDelete(file_t* file)
{
    if(file->refs > 0)
    {
        return E_BUSY;
    }

    dir_t* parent = file->owner;
    
    if(parent->files == file)
    {
        parent->files = file->sibling;
    }
    else
    {
        file_t* it;
        for(it = parent->files; it != NULL; it = it->sibling)
        {
            if(it->sibling == file)
            {
                it->sibling = file->sibling;
                break;
            }
        }

        if(it == NULL)
        {
            return E_INVAL;
        }
    }
    
    free(file);

    return E_OK;
}

int32_t ProcFileOpen(file_t* file, int32_t mode)
{
    if(mode == O_RDONLY || file->access & (uint16_t)(mode & FILE_ACCESS_MASK))
    {
        file->refs++;
        return E_OK;
    }

    return E_INVAL;
}

int32_t ProcFileClose(file_t* file)
{
    return --file->refs;
}
