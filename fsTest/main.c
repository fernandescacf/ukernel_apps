#include <types.h>
#include <io_types.h>
#include <server.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <mman.h>
#include <ipc.h>
#include <task.h>

int ls_server(sentry_t *server)
{
    printf("%s pid: %d chid: %d\n", server->name, server->pid, server->chid);

    return E_OK;
}

int32_t newLsNamespace(const char* path)
{
    // Reply buffer
    char reply[256];
    // Message header
    io_hdr_t hdr;
    hdr.type = _IO_INFO;
    hdr.code = INFO_NAMESPACE_LS;
    hdr.sbytes = strlen(path) + 1;
    hdr.rbytes = sizeof(reply) - 1;
    uint32_t replySize;

    // Query System for the received path
    if(MsgSend(SYSTEM_SERVER, &hdr, path, reply, &replySize) == E_OK)
    {
        io_entry_t *entry = (io_entry_t *)reply;
        // Add termination character (not added by default)
        reply[replySize] = 0;

        if(entry->type != INFO_NAMESPACE)
        {
            return E_ERROR;
        }

        nentry_t *ns = &entry->namespace;

        int32_t entries = ns->nentries + ns->sentries;

        for(hdr.code = 1; hdr.code <= entries; hdr.code++)
        {
            uint32_t replySize = 0;

            if(MsgSend(SYSTEM_SERVER, &hdr, path, reply, &replySize) != E_OK)
            {
                return E_ERROR;
            }

            io_entry_t *entry = (io_entry_t *)reply;
            reply[replySize] = 0;

            if(entry->type == INFO_NAMESPACE)
            {
                printf("%s/\n", entry->namespace.name);
            }
            else if (entry->type == INFO_SERVER)
            {
                ls_server(&entry->server);
            }
        }
        return E_OK;
    }

    return E_ERROR;
}

int32_t newLsServer(int32_t fd, const char* path)
{
    // Reply buffer
    char reply[256];
    // Message header
    io_hdr_t hdr;
    hdr.type = _IO_INFO;
    hdr.code = INFO_LIST_ALL;
    hdr.sbytes = strlen(path) + 1;
    hdr.rbytes = sizeof(reply) - 1;
    uint32_t replySize;

    if(MsgSend(fd, &hdr, path, reply, &replySize) == E_OK)
    {
        uint32_t offset = 0;
        while(offset < replySize)
        {
            io_entry_t *entry = (io_entry_t *)&reply[offset];

            if(entry->type == INFO_DIR)
            {
                dentry_t *dir = &entry->dir;
                printf("%s/\n", dir->name);
                offset += (sizeof(dentry_t) + dir->len - 3);
            }
            else if(entry->type == INFO_FILE)
            {
                fentry_t *file = &entry->file;
                printf("%s\n", file->name);
                offset += (sizeof(fentry_t) + file->len - 3);
            }
            else
            {
                return E_ERROR;
            }   
        }

        return E_OK;
    }

    return E_ERROR;
}

int32_t newLs(const char* path)
{
    char* remaining = NULL;
    int32_t fd = connect(path, &remaining);

    // LS a server?
    if(fd != -1)
    {
        int32_t ret = newLsServer(fd, remaining);
        ConnectDetach(fd);
        return ret;
    }

    // LS System
    return newLsNamespace(path);
}


int main(int argc, const char* argv[])
{
    StdOpen();

//    uSleep(500);
    printf("\n\n/> ls /\n");
    newLs("/");
    printf("\n\n/> ls /dev/\n");
    newLs("/dev/");
    printf("\n\n/> ls /dev/stdin\n");
    newLs("/dev/stdin");
    printf("\n\n/> ls /proc\n");
    newLs("/proc/");
    printf("/> ls /proc/boot\n");
    newLs("/proc/boot");
/*
    if(argc == 1)
    {
        newLs("/proc/boot");
    }
    else
    {
        newLs(argv[1]);
    }
*/
    int32_t fd = open("/proc/sys", O_RDONLY);

    if(fd == -1)
    {
        printf("Error openning /proc/sys\n");

        return 0;
    }

    char buffer[256];
    read(fd, buffer, 256);

    printf("/> cat /proc/sys\n");
    printf("%s", buffer);
    
    close(fd);

    fd = open("/proc/devices", O_RDONLY);

    if(fd == -1)
    {
        printf("Error openning /proc/devices\n");

        return 0;
    }

    read(fd, buffer, 256);

    printf("/> cat /proc/devices\n");
    printf("%s", buffer);
    
    close(fd);

    fd = open("/proc/boot/file.txt", O_RDWR | O_CREAT);

    if(fd == -1)
    {
        printf("Error creating file /proc/boot/file.txt\n");
        return 0;
    }

    if(ftruncate(fd, 4096) != E_OK)
    {
        printf("Error truncating /proc/boot/file.txt\n");
        return 0;
    }

    printf(">");
    scanf("%s", buffer);
    size_t size = strlen(buffer) + 1;

    write(fd, buffer, size);

    memset(buffer, 0x0, 256);

    close(fd);

    fd = open("/proc/boot/file.txt", O_RDWR);
    lseek(fd, 1, SEEK_SET);
    read(fd, buffer, size);
    printf(">%s", buffer);

    printf("\n/> ls /proc/boot\n");
    newLs("/proc/boot");

    char *mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x0);

    if(mem == NULL)
    {
        printf("Failed to mmap /proc/boot/file.txt\n");
        return 0;
    }

    printf(">%s\n", mem);

    printf("Exec echo Hello\n");

    int32_t echo = open("/proc/boot/echo", O_RDWR);
    if(echo == -1)
    {
        printf("Error openning /proc/boot/echo\n");
        return 0;
    }

    size_t eSize = (size_t)lseek(echo, 0, SEEK_END);

    printf("echo size:%d\n", eSize);

    char* elf =  mmap(NULL, eSize, PROT_READ | PROT_WRITE, MAP_SHARED, echo, 0x0);

    pid_t pid = Spawn(elf, "echo Hello my dear!\n");

    if(pid == -1)
    {
        printf("Failed to execute: echo\n");
        return 0;
    }

    printf("Process echo created with pid: %d\n", pid);

     WaitPid(pid);

    StdClose();

    return 0;
}