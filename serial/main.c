#include <types.h>
#include <dispatch.h>
#include <ipc.h>
#include <mutex.h>
#include <mman.h>
#include <stdlib.h>
#include <string.h>

#include <uart.h>

#define ROUND_UP(m,a)			(((m) + ((a) - 1)) & (~((a) - 1)))

static mutex_t stdLock;

static io_funcs_t   out_io_funcs;
static ctrl_funcs_t out_ctrl_funcs;

static io_funcs_t   in_io_funcs;
static ctrl_funcs_t in_ctrl_funcs;

uint32_t StdRead(char *buffer)
{
    MutexLock(&stdLock);

    (void)gets(buffer);

    MutexUnlock(&stdLock);

    return strlen(buffer);
}

void StdWrite(const char *buffer)
{
    MutexLock(&stdLock);

    puts(buffer);

    MutexUnlock(&stdLock);
}

int32_t ReadStdIn(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset)
{
    if(hdr->rbytes == 0)
    {
        // Invalid request
        return MsgRespond(rcvid, -1, NULL, 0);
    }

    if(hdr->code == _IO_READ_SIZE)
    {
        size_t size = 0;
        size_t allocSize = hdr->rbytes;
        char *stream = NULL;

        if(allocSize > 256)
        {
            allocSize = ROUND_UP(allocSize, 4096);
            stream = (char*)mmap(NULL, allocSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, NOFD, 0x0);
        }
        else
        {
            stream = (char*)malloc(sizeof(char) * allocSize);
        }
        
        while(size < hdr->rbytes)
        {
            stream[size] = (char)getc();
            size++;
        }

        MsgRespond(rcvid, hdr->rbytes, (const char *)stream, hdr->rbytes);

        if(allocSize > 256)
        {
            munmap(stream, allocSize);
        }
        else
        {
            free(stream);
        }
    }
    else if(hdr->code == _IO_READ_TERMINATOR)
    {
        size_t size = StdRead(buffer) + 1;

        MsgRespond(rcvid, size, (const char *)buffer, size);
    }

    return E_OK;
}

int32_t WriteStdOut(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset)
{
    (void)scoid;

    buffer[offset] = '\0';
    StdWrite((const char *)buffer);

    while(offset < hdr->sbytes)
    {
        uint32_t size = (uint32_t)MsgRead(rcvid, (const char *)buffer, 1024 - 1, offset);

        if(size > 0)
        {
            offset += size;
            buffer[size] = '\0';
            StdWrite(buffer);
        }
        else
        {
            break;
        }
    }
    
    return MsgRespond(rcvid, hdr->sbytes, NULL, 0);
}

int main(/*int argc, const char* argv[]*/)
{
    UartOpen();

    if(MutexInit(&stdLock) != E_OK)
    {
        puts("\nWARNING: Failed to initialze mutex!\n");
    }

    dispatch_attr_t out_attr = {0x0, 1024, 1023, 1};
    dispatch_attr_t in_attr  = {0x0, 1024, 1023, 1};

    out_io_funcs.io_write = WriteStdOut;
    in_io_funcs.io_read   = ReadStdIn;

    dispatch_t* out_disp = DispatcherAttach("/dev/stdout", &out_attr, &out_io_funcs, &out_ctrl_funcs);
    dispatch_t* in_disp =  DispatcherAttach("/dev/stdin",  &in_attr,  &in_io_funcs,  &in_ctrl_funcs);

    DispatcherStart(out_disp, FALSE);
    DispatcherStart(in_disp,  TRUE);

    UartClose();

    return 0;
}