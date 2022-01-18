/**
 * @file        proc_io.c
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        24 August, 2020
 * @brief       Proc File System IO functions implementation
*/

/* Includes ----------------------------------------------- */
#include <proc_io.h>
#include <proc.h>
#include <ipc.h>
#include <server.h>
#include <string.h>
#include <mman.h>
#include <unistd.h>


/* Private types ------------------------------------------ */


/* Private constants -------------------------------------- */
#define FILE_DEFAULT_SIZE   4096
#define FILE_ACCESS_MASK    (0x3)


/* Private macros ----------------------------------------- */
#define ALIGN_DOWN(m,a)	((m) & (~(a - 1)))
#define ALIGN_UP(m,a)	(((m) + (a - 1)) & (~(a - 1)))

/* Private variables -------------------------------------- */



/* Private function prototypes ---------------------------- */

uint32_t CopyDirEntries(char* buffer, dir_t* cwd)
{
    uint32_t size = 0;
    dir_t* dir = cwd->dirs;
    for( ; dir != NULL; dir = dir->sibling)
    {
        dentry_t dentry = {INFO_DIR, (uint16_t)dir->len};
        memcpy(&buffer[size], &dentry, offsetof(dentry_t, name));
        size += offsetof(dentry_t, name);
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
        
        memcpy(&buffer[size], &fentry, offsetof(fentry_t, name));
        size += offsetof(fentry_t, name);
        memcpy(&buffer[size], file->name, file->len);
        size += file->len + 1;
        buffer[size - 1] = 0;
    }

    return size;
}


/* Private functions -------------------------------------- */

int32_t _io_ProcInfo(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset)
{
    if(hdr->code != INFO_LIST_ALL)
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
    }

    char* remaining = NULL;
    dir_t* cwd = ProcPathResolve(NULL, buffer, &remaining);

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

int32_t _io_FileOpen(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    connect_t* con = ConnectionGet(scoid);

    // Only open connection if it is closed
    if((con->handler != NULL) || (con->state != CONNECTION_CLOSE))
    {
        return MsgRespond(rcvid, E_BUSY, NULL, 0);
    }

    // In case sender did not put a terminator character
    buffer[hdr->sbytes] = 0;

    file_t *file = ProcFileGet(NULL, (const char*)buffer);

    if(file == NULL)
    {
        if(!(hdr->code & O_CREAT))
        {
            return MsgRespond(rcvid, E_NO_RES, NULL, 0);
        }

        //void* data = mmap(NULL, FILE_DEFAULT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, NOFD, 0x0);
        //file = ProcFileCreate(NULL, (const char*)buffer, data, FILE_DEFAULT_SIZE, O_RDWR, FILE_MAP_PERMISSION);
        file = ProcFileCreate(NULL, (const char*)buffer, NULL, 0, O_RDWR, FILE_MAP_PERMISSION);

        if(file == NULL)
        {
            return MsgRespond(rcvid, E_ERROR, NULL, 0);
        }
    }

    if(ProcFileOpen(file, hdr->code & FILE_ACCESS_MASK) != E_OK)
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
    }

    ConnectionSetState(con, CONNECTION_OPEN);
    ConnectionSetAccess(con, hdr->code & 0x3);
    ConnectionSetHandler(con, file);
    con->seek = 0;

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t _io_FileClose(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    // Not used
    (void)hdr; (void)buffer;

#ifdef HANDLE_CLOSE_IO  
    connect_t* con = ConnectionGet(scoid);

    // If connection was already closed do nothing
    if(con->state == CONNECTION_CLOSE)
    {
        return MsgRespond(rcvid, E_OK, NULL, 0);
    }

    // If connection was shared (mapped) we have to unshare it
    if(con->state == CONNECTION_MAP)
    {
        UnshareObject(scoid);
    }

    // This check shouldn't be required!
    if(con->handler != NULL)
    {
        ProcFileClose((file_t*)con->handler);
    }

    ConnectionSetState(con, CONNECTION_CLOSE);
    ConnectionSetAccess(con, O_RDONLY);
    ConnectionSetHandler(con, NULL);
    con->seek = 0;
#endif

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t _io_FileRead(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    connect_t* con = ConnectionGet(scoid);

    // Only read id connection is not closed
    if((con->handler == NULL) || (con->state == CONNECTION_CLOSE))
    {
        return MsgRespond(rcvid, -1, NULL, 0);
    }

    file_t* file = (file_t*)con->handler;

    if(con->seek >= file->size)
    {
        return MsgRespond(rcvid, 0, NULL, 0);
    }

    uint32_t readPos = con->seek;

    // Compute read size
    size_t size = (((hdr->rbytes + readPos) < (file->size)) ? (hdr->rbytes) : (file->size - readPos));

    con->seek += size;

    char* src = (char*)file->data;

    return MsgRespond(rcvid, size, &src[readPos], size);
}

int32_t _io_FileWrite(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    connect_t* con = ConnectionGet(scoid);

    // Only write id connection is not closed and file is open we write enabled
    if(((con->handler == NULL) || (con->state == CONNECTION_CLOSE)) || (con->access == O_RDONLY))
    {
        return MsgRespond(rcvid, -1, NULL, 0);
    }

    file_t* file = (file_t*)con->handler;

    if(con->seek >= file->size)
    {
        return MsgRespond(rcvid, 0, NULL, 0);
    }

    uint32_t writePos = con->seek;

    // Compute write size
    size_t size = (((hdr->sbytes + writePos) < (file->size)) ? (hdr->sbytes) : (file->size - writePos));

    con->seek += size;

    char* dst = (char*)file->data;

    memcpy(&dst[writePos], buffer, hdr->sbytes);

    return MsgRespond(rcvid, size, NULL, 0);
}

int32_t _io_FileSeek(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    connect_t* con = ConnectionGet(scoid);

    // Only seek if file is opend
    if((con->handler == NULL) || (con->state == CONNECTION_CLOSE))
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    file_t* file = (file_t*)con->handler;

    switch (hdr->code)
    {
    case SEEK_SET:
        con->seek = *((off_t*)buffer);
        break;
    case SEEK_CUR:
        con->seek += *((off_t*)buffer);
        break;
    case SEEK_END:
        con->seek = (off_t)file->size + *((off_t*)buffer);
        break;
    default:
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
        break;
    }

    return MsgRespond(rcvid, E_OK, (const char*)&con->seek, sizeof(off_t));
}

int32_t _io_FileTruncate(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    connect_t* con = ConnectionGet(scoid);

    // Only truncate if file is opened
    if((con->handler == NULL) || (con->state == CONNECTION_CLOSE))
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    file_t* file = (file_t*)con->handler;

    // Does file allows mapping
    if(!(file->permission & FILE_MAP_PERMISSION))
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    off_t size = (off_t)ALIGN_UP((*(uint32_t*)buffer), 4096);

    if(file->size < (size_t)size)
    {
        void* newData = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, NOFD, 0x0);
        memcpy(newData, file->data, file->size);
        memset(newData + file->size, 0x0, size - file->size);
        munmap(file->data, file->size);
        file->size = size;
        file->data = newData;
    }
    else
    {
        void* newData = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, NOFD, 0x0);
        memcpy(newData, file->data, size);
        munmap(file->data, file->size);
        file->size = size;
        file->data = newData;
    }

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t _io_FileShare(int32_t rcvid, int32_t scoid, io_hdr_t *hdr, char *buffer, uint32_t offset)
{
    // Not used
    (void)hdr; (void)buffer;

    connect_t* con = ConnectionGet(scoid);

    // File is only shared once per connection
    if(con->state == CONNECTION_MAP)
    {
        return MsgRespond(rcvid, E_OK, NULL, 0);
    }

    // Can we share the file?
    if(ConnectionSetState(con, CONNECTION_MAP) != E_OK)
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
    }

    file_t* file = (file_t*)con->handler;

    if(ShareObject(file->data, scoid, 0) != file->data)
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

void _io_CloseCallBack(connect_t* con)
{
    // If connection was already closed do nothing
    if(con->state == CONNECTION_CLOSE)
    {
        return;
    }

    // If connection was shared (mapped) we have to unshare it
    if(con->state == CONNECTION_MAP)
    {
        UnshareObject(con->scoid);
    }

    // This check shouldn't be required!
    if(con->handler != NULL)
    {
        ProcFileClose((file_t*)con->handler);
    }

    ConnectionSetState(con, CONNECTION_CLOSE);
    ConnectionSetAccess(con, O_RDONLY);
    ConnectionSetHandler(con, NULL);
    con->seek = 0;
}