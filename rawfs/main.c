#include <server.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <task.h>

#define ROUND_UP(m,a)			(((m) + ((a) - 1)) & (~((a) - 1)))

typedef struct File
{
    struct File *next;
    struct File *prev;
    uint32_t    pages;
    size_t      size;
    void        *data;
    size_t      len;
    char        name[1];
}file_t;

file_t *CreateFile(const char *name, size_t size)
{
    size_t len = strlen(name);
    file_t *file = (file_t*)malloc(sizeof(file_t) + len + 1);

    if(file == NULL)
    {
        return NULL;
    }

    file->prev = file->next = NULL;
    file->size = size;
    file->pages = ROUND_UP(size, 4096) / 4096;
    file->len = len;
    memcpy(file->name, name, len + 1); // Also copy null character
    file->data = (void *)mmap(NULL, file->pages * 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, NOFD, 0x0);

    if(file->data == NULL)
    {
        free(file);
        return NULL;
    }

    return file;
}

void DestroyFile(file_t *file)
{
    if(file == NULL)
    {
        return;
    }

    munmap(file->data, file->pages * 4096);

    free(file);
}

static struct 
{
    uint32_t    files;
    file_t      *first;
    file_t      *last;
}fileSystem;

void FsAddFile(file_t *file)
{
    fileSystem.files++;

    file->next = NULL;

    if(fileSystem.first == NULL)
    {
        fileSystem.first = file;
        fileSystem.last = file;
        file->prev = NULL;
        return;
    }

    fileSystem.last->next = file;
    file->prev = fileSystem.last;
    fileSystem.last = file;
}

file_t *FsSearchFile(const char *name)
{
    if(fileSystem.files == 0)
    {
        return NULL;
    }

    size_t len = strlen(name);

    file_t *file = fileSystem.first;
    for( ; file != NULL; file = file->next)
    {
        if(file->len == len && !memcmp(file->name, name, len))
        {
            return file;
        }
    }

    return NULL;
}

file_t *FsRemoveFile(const char *name)
{
    file_t *file = FsSearchFile(name);

    if(file == NULL)
    {
        return NULL;
    }

    if(file == fileSystem.first)
    {
        fileSystem.first = file->next;
    }
    else
    {
        file->prev->next = file->next;
    }
    
    if(file == fileSystem.last)
    {
        fileSystem.last = file->prev;
    }
    else
    {
        file->next->prev = file->prev;
    }

    return file;
}



enum
{
    FList = 0,
    FLoad,
    FSearch,
    FDelete,
    FExec,
    SysInfo,
    PKill,
    Sleep,
    StackTest,
    MemAbort,
    Exit,
    Invalid
};

#define MAXCOMMANDS Invalid

static struct
{
    char        str[8];
    uint32_t    action;
}commandEntries[]
    = {{"ls", FList}, {"load", FLoad}, {"search", FSearch},
       {"delete", FDelete}, {"exec", FExec}, {"info", SysInfo},
       {"exit", Exit}, {"kill", PKill}, {"sleep", Sleep},
       {"memtest", StackTest}, {"abort", MemAbort}};

uint32_t CommandEntriesLookup(char *cmd)
{
    int i = 0;
    for ( ; i < MAXCOMMANDS; i++)
    {
        if (!strcmp(cmd, commandEntries[i].str)) return commandEntries[i].action;
    }
    return Invalid;
}

uint32_t CommandGet(const char *str, char *cmd)
{
    uint32_t len = 0;
    for( ; (str[len] != ' ') && (str[len] != 0); len++)
    {
        cmd[len] = str[len];
    }
    cmd[len] = 0;
    return ((str[len] == ' ') ? (len +1) : (len));
}

int32_t SerialLoad(void * addr, uint32_t size)
{
    printf("Loading file...");

    // TODO: Add a way to get the stdin and change to read(stdin, addr, size);
    read(stdin, addr, size);

    printf("\n");

	return E_OK;
}

int32_t LoadFile(char *cmd)
{
    printf("Load parameters: %s\n", cmd);
    // Get Name
    uint32_t len = 0;
    for( ; (cmd[len] != ' ') && (cmd[len] != 0); len++){}
    cmd[len] = 0;
    char *remaining;
    size_t size = strtoul(&cmd[len + 1], &remaining, 0);

    if(*remaining != 0 || size == 0)
    {
        printf("Invalid name or size: %s %d\n", cmd, size);
        return E_ERROR;
    }

    file_t *file = CreateFile((const char *)cmd, size);

    if(file == NULL)
    {
        printf("Failed to create file: %s\n", cmd);
        return E_ERROR;
    }

    if(SerialLoad(file->data, file->size) != E_OK)
    {
        // Delete File
        return E_ERROR;
    }

    FsAddFile(file);

    return E_OK;
}

int32_t Exec(char *cmd)
{
    uint32_t len = strlen(cmd);
    bool_t wait = TRUE;
    if(cmd[len - 1] == '&' && cmd[len - 2] == ' ')
    {
        wait = FALSE;
        cmd[len - 2] = 0;
    }
    // Get Name
    for(len = 0; (cmd[len] != ' ') && (cmd[len] != 0); len++){}
    char c = cmd[len];
    cmd[len] = 0;

    file_t *file = FsSearchFile((const char *)cmd);

    if(file == NULL)
    {
        printf("File %s not found\n", cmd);
        return E_ERROR;
    }

    cmd[len] = c;

    int32_t pid = Spawn(file->data, cmd);

    if(pid == -1)
    {
        printf("Failed to execute: %s\n", cmd);
        return E_ERROR;
    }

    cmd[len] = 0;
    printf("Process %s created with pid: %d\n", cmd, pid);

    if(wait == TRUE)
    {
        WaitPid(pid);
    }

    return E_OK;
}

int32_t ListFiles(char *cmd)
{
    if(*cmd == 0x0)
    {
        file_t *file = fileSystem.first;
        for( ; file != NULL; file = file->next)
        {
            printf("%s %d\n", file->name, file->size);
        }
    }
    else
    {
        file_t *file = FsSearchFile((const char *)cmd);

        if(file == NULL)
        {
            printf("File %s not found\n", cmd);
            return E_ERROR;
        }

        printf("%s %d\n", file->name, file->size);
    }
    
    return E_OK;
}

int32_t DeleteFile(char *name)
{
    file_t *file =FsRemoveFile(name);

    if(file == NULL)
    {
        printf("File %s not found\n", name);
        return E_ERROR;
    }

    DestroyFile(file);

    return E_OK;
}

typedef struct
{
	uint32_t ramtotal;
	uint32_t ramavailable;
	uint32_t ramusage;
	uint32_t runningprocs;
}sysinfo_t;

int32_t SysInfoGet(char *buff)
{
    (void)buff;

    sysinfo_t sysinfo;

    io_hdr_t hdr;
    hdr.type = _IO_READ;
    hdr.code = 0;
    hdr.sbytes = 0;
    hdr.rbytes = sizeof(sysinfo_t);

    if(MsgSend(SYSTEM_SERVER, &hdr, NULL, (const char *)&sysinfo, NULL) != E_OK)
    {
        printf("Failed to read system information\n");
        return E_ERROR;
    }

    printf("System Information\n");
    printf("Ram Total: %dKB\n", sysinfo.ramtotal/1024);
    printf("Ram Available: %dKB\n", sysinfo.ramavailable/1024);
    printf("Ram Used: %dKB\n", sysinfo.ramusage/1024);
    printf("Running Processes: %d\n", sysinfo.runningprocs);
    
    return E_OK;
}

int32_t KillPid(char *buff)
{
    pid_t pid = strtoul(buff, NULL, 10);

    Kill(pid);

    return E_OK;
}

int32_t TaskSleep(char *buff)
{
    uint32_t time = strtoul(buff, NULL, 10);

    uSleep(time);

    return E_OK;
}

int32_t TestStack(char *buff)
{
    uint32_t size = strtoul(buff, NULL, 10);
    uint32_t *sp = &size;

    sp -= (size >> 2);

    printf("*** Stack Test ***\n");
    printf(" - write @%x: 0xdeadbeef\n", sp);

    *sp = 0xdeadbeef;

    printf(" - read @%x: %x\n", sp, *sp);

    return E_OK;
}

int32_t MemoryAbort(char *buff)
{
    (void)buff;

    *(uint32_t*)(0) = 0xdeaddead;

    return E_OK;
}

int32_t (*CommandHandlers[])(char *) =
    {
        ListFiles,
        LoadFile,
        NULL,
        DeleteFile,
        Exec,
        SysInfoGet,
        KillPid,
        TaskSleep,
        TestStack,
        MemoryAbort,
    };

int main()
{
    StdOpen();

    char *buffer = (char*)malloc(126);
    char *cmd = (char*)malloc(32);

    printf("\n");

    while(TRUE)
    {
        printf("/> ");
        scanf("%s", buffer);

        uint32_t offset = CommandGet(buffer, cmd);
        uint32_t action = CommandEntriesLookup(cmd);

        if(action == Invalid)
        {
            printf("Invalid command\n");
            continue;
        }

        if(action == Exit)
        {
            break;
        }
        
        if(CommandHandlers[action])
        {
            CommandHandlers[action](&buffer[offset]);
        }

    }

    StdClose();

    return 0;
}