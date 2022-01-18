#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mman.h>
#include <ipc.h>
#include <io_types.h>
#include <list.h>
#include <dispatch.h>
#include <server.h>
#include <rfs.h>
#include <fs.h>
#include <connections.h>

#define PROC_PATH       "/proc"
#define SYS_FILE        "/sys"
#define DEVICES_FILE    "/devices"
#define BOOT_FILES_PATH "/boot/"

#define FILE_DEFAULT_SIZE   4096
#define O_RDONLY    0x00
#define O_WRONLY    0x01
#define O_RDWR      0x02
#define O_CREATE    0x10

void BuildFileSystem()
{
    char* sys = (char*)malloc(sizeof(char) * 62);

    void* ramBase = NULL;
    size_t ramSize = 0;
    RfsGetRamInfo(&ramBase, &ramSize);

    int size = sprintf(sys, "Version: %s\nArch: %s\nMach: %s\nRam Size: 0x%x\n", RfsGetVersion(), RfsGetArch(), RfsGetMach(), ramSize);

    (void)CreateFile(NULL, "/sys", sys, size, FILE_READ_ACCESS);

    char* devices = (char*)malloc(sizeof(char) * 256);
    char* ptr = devices;
    uint32_t i;
    for(i = 0; i < RfsDevicesCount(); i++)
    {
        char* name;
        void* addr;
        size_t size;
        RfsDeviceParse(i, &name, &addr, &size);
        ptr += sprintf(ptr, "{\"%s\", @0x%x, 0x%x}\n", name, addr, size);
    }

    (void)CreateFile(NULL, "/devices", devices, (devices - ptr), FILE_READ_ACCESS);

    for(i = 0; i < RfsFilesCount(); i++)
    {
        char path[256];
        char* name;
        int32_t type;
        void* data;
        size_t size;
        RfsFileParse(i, &name, &type, &data, &size);

        void* dataCopy = (void*)mmap(NULL, size, (PROT_READ |PROT_WRITE), (MAP_ANON | MAP_SHARED), NOFD, 0x0);
        memcpy(dataCopy, data, size);
        
        sprintf(path, "/boot/%s", name);

        (void)CreateFile(NULL, path, dataCopy, size, FILE_RW_ACCESS | FILE_MAP_PERMISSION | FILE_EXEC_PERMISSION);
    }
}

uint32_t CopyDirEntries(char* buffer, dir_t* cwd)
{
    uint32_t size = 0;
    dir_t* dir = cwd->dirs;
    for( ; dir != NULL; dir = dir->sibling)
    {
        dentry_t dentry = {INFO_DIR, (uint16_t)dir->len};
        
        memcpy(&buffer[size], &dentry, sizeof(dentry) - 4);
        size += (sizeof(dentry) - 4);
        memcpy(&buffer[size], dir->name, dir->len);
        size += dir->len + 1;
        buffer[size - 1] = 0;
    }

    return size;
}

uint32_t CopyFileEntries(char* buffer, dir_t* cwd)
{
    uint32_t size = 0;
    file_t* file = cwd->files;
    for( ; file != NULL; file = file->sibling)
    {
        fentry_t fentry = {INFO_FILE, file->size, (uint16_t)file->len};
        
        memcpy(&buffer[size], &fentry, sizeof(fentry) - 4);
        size += (sizeof(fentry) - 4);
        memcpy(&buffer[size], file->name, file->len);
        size += file->len + 1;
        buffer[size - 1] = 0;
    }

    return size;
}

int32_t ls(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset)
{
    if(hdr->code != INFO_LIST_ALL)
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
    }

    char* remaining = NULL;
    dir_t* cwd = PathResolve(NULL, buffer, &remaining);

    // Are we listing a file or is the path invalid?
    if(remaining != NULL)
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
        return E_OK;
    }

    uint32_t size = CopyDirEntries(buffer, cwd);
    size += CopyFileEntries(&buffer[size], cwd);

    return MsgRespond(rcvid, E_OK, buffer, size);
}

int32_t FsOpen(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    connection_t* client = ConnectionGet(scoid);

    if((client->file != NULL) && ((client->state =! CONNECT_INVALID) || (client->state =! CONNECT_CLOSE)))
    {
        return MsgRespond(rcvid, E_BUSY, NULL, 0);
    }

    // In case sender did not put a terminator character
    buffer[hdr->sbytes] = 0;

    file_t *file = GetFile(NULL, (const char*)buffer);

    if(file == NULL)
    {
        if(!(hdr->code & O_CREATE))
        {
            return MsgRespond(rcvid, E_NO_RES, NULL, 0);
        }

        void* data = mmap(NULL, FILE_DEFAULT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, NOFD, 0x0);
        file = CreateFile(NULL, (const char*)buffer, data, FILE_DEFAULT_SIZE, FILE_RW_ACCESS | FILE_MAP_PERMISSION);

        if(file == NULL)
        {
            return MsgRespond(rcvid, E_ERROR, NULL, 0);
        }

        FileOpen(file, FILE_RW_ACCESS | FILE_MAP_PERMISSION);
    }

    client->access = hdr->code & 0x3;
    client->state = CONNECT_OPEN;
    client->file = file;

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t FsClose(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    (void)hdr; (void)buffer;

    connection_t* client = ConnectionGet(scoid);

    if(client->state == CONNECT_SHARED)
    {
        UnshareObject(scoid);
    }

    if(client->file != NULL)
        FileClose(client->file);

    client->file = NULL;
    client->access = CONNECT_NO_ACCESS;
    client->seek = 0;
    client->state = CONNECT_CLOSE;

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t FsRead(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    connection_t* client = ConnectionGet(scoid);

    if(((client->state != CONNECT_OPEN) && (client->state != CONNECT_SHARED)) || (client->access == CONNECT_NO_ACCESS))
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    size_t size = ((hdr->rbytes < client->file->size) ? (hdr->rbytes) : (client->file->size));

    char *src = (char*)client->file->data;

    memcpy(buffer, &src[client->seek], size);

    client->seek += size;

    return MsgRespond(rcvid, size, buffer, size);
}

int32_t FsWrite(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    connection_t* client = ConnectionGet(scoid);

    if(((client->state != CONNECT_OPEN) && (client->state != CONNECT_SHARED)) || (client->access != O_RDWR))
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    char *dst = (char*)client->file->data;

    memcpy(&dst[client->seek], buffer, hdr->sbytes);

    client->seek += hdr->sbytes;

    return MsgRespond(rcvid, hdr->sbytes, NULL, 0);
}

int32_t FsSeek(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    (void)buffer;

    connection_t* client = ConnectionGet(scoid);

    if((client->state != CONNECT_OPEN) && (client->state != CONNECT_SHARED))
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    client->seek = (uint32_t)hdr->code;

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t FsShare(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    (void)hdr; (void)buffer;

    connection_t* client = ConnectionGet(scoid);

    if(client->state == CONNECT_SHARED)
    {
        return MsgRespond(rcvid, E_OK, NULL, 0);
    }

    if(client->state != CONNECT_OPEN)
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
    }

    if(ShareObject(client->file->data, scoid, 0) != client->file->data)
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    client->state = CONNECT_SHARED;

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t ServerStart()
{
    // Install Server Dispatcher to handle client messages
    dispatch_attr_t attr = {0x0, 2048, 2048, 1};
    ctrl_funcs_t fs_ctrl_funcs = { ConnectionAttachHandler, ConnectionDetachHandler, NULL };
    io_funcs_t fs_io_funcs = { ls,      FsRead, FsWrite, FsOpen, FsClose,
                               FsShare, NULL,   FsSeek, NULL, NULL};
    dispatch_t* fs_disp = DispatcherAttach(PROC_PATH, &attr, &fs_io_funcs, &fs_ctrl_funcs);

    // Sart the dispatcher, will not return
    DispatcherStart(fs_disp,  TRUE);

    return E_OK;
}

int main()
{
    StdOpen();

    printf("\nOpen Raw Filesystem\n");
    if(RfsInit() != E_OK)
    {
        printf("Failed to open Raw Filesystem\n");
        return -1;
    }

    printf("Parse Raw Filesystem\n");
    if(RfsParse() != E_OK)
    {
        printf("Failed to parse Raw Filesystem\n");

        RfsDelete();

        return -1;
    }

    RfsDumpHeader();

    void*  ramAddr = NULL;
    size_t ramSize = 0;
    RfsGetRamInfo(&ramAddr, &ramSize);
    printf("Ram address: 0x%x\nRam size:    0x%x\n", (uint32_t)ramAddr, ramSize);


    BuildFileSystem();

//    printf("Try to open Raw Filesystem again\n");
//    if(RfsInit() == E_OK)
//    {
//        printf("Raw Filesystem was open again...\n");
//    }
//    else
//    {
//        printf("Failed to open Raw Filesystem again\n");
//    }

//    printf("Delete Raw Filesystem\n");
    RfsDelete();

    printf("/proc Filesystem started!!!\n");
    ServerStart();

    StdClose();

    return 0;
}