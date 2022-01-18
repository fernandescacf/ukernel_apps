#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mman.h>
#include <server.h>
#include <task.h>
#include <mutex.h>
#include <errno.h>
#include <semaphore.h>
#include <cond.h>

#define ROUND_UP(m,a)			(((m) + ((a) - 1)) & (~((a) - 1)))

const char PATH[] = "/proc/:/proc/boot/:/proc/user/";

enum
{
    SysInfo,
    PKill,
    Exit,
    Invalid
};

#define MAXCOMMANDS Invalid

static struct
{
    char        str[8];
    uint32_t    action;
}commandEntries[]
    = {{"info", SysInfo}, {"exit", Exit}, {"kill", PKill}};

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

int32_t PathGen(char* path, int32_t offset, const char* name)
{
    uint32_t len;
    for(len = 0; PATH[offset] != '\0' && PATH[offset] != ':'; offset++, len++)
    {
        path[len] = PATH[offset];
    }

    if(path[len - 1] != '/')
    {
        path[len++] = '/';
    }
    else
    {
        strcpy(&path[len], name);
    }

    return ((PATH[offset] == '\0') ? (-1) : (offset + 1));
}

const char* strstr(const char* str1, const char* str2)
{
    uint32_t index = 0;
    for( ; str1[index] != 0; index++)
    {
        if(strcmp(&str1[index], str2) == 0)
        {
            return &str1[index];
        }
    }

    return NULL;
}

int32_t Exec(char* cmd)
{
    uint32_t len = strlen(cmd);
    bool_t wait = TRUE;
    bool_t redirect = FALSE;
    if(cmd[len - 1] == '&' && cmd[len - 2] == ' ')
    {
        wait = FALSE;
        cmd[len - 2] = 0;
    }

    if(wait == TRUE && cmd[len - 1] == '>' && cmd[len - 2] == ' ')
    {
        redirect = TRUE;
        cmd[len - 2] = 0;
    }

    char path[256];
    // Get Process name and append it to default path
    for(len = 0; (cmd[len] != ' ') && (cmd[len] != 0); len++) {}
    char c = cmd[len];
    cmd[len] = 0;

    int32_t fd;
    int32_t offset = 0;

    while(TRUE)
    {
        offset = PathGen(path, offset, cmd);

        fd = open(path, O_RDONLY);

        if(fd != -1)
        {
            break;
        }

        if(offset == -1)
        {
            printf("File %s not found\n", cmd);
            return E_ERROR;
        }
    }

    cmd[len] = c;

    off_t off = lseek(fd, 0, SEEK_END);

    if(off == -1)
    {
        close(fd);
        printf("%s is not a valid file\n", path);
        return E_ERROR;
    }

    void* elf = mmap(NULL, (size_t)off, PROT_READ, MAP_SHARED, fd, 0x0);

    if(elf == NULL)
    {
        close(fd);
        printf("Fail to load %s\n", path);
        return E_ERROR;
    }

    uint32_t fd_count = 0;
    int32_t fd_map[10];

    if(redirect == TRUE)
    {
        int32_t r_fd = open("/proc/user/test", O_CREAT | O_RDWR);
        if(fd == -1)
        {
            printf("Faild to create file: /proc/user/test\n");
        }
        if(ftruncate(r_fd, 4096) != E_OK)
        {
            close(fd);
            printf("Error truncating\n");
        }

        fd_count = 3;
        fd_map[0] = -1;
        fd_map[1] = -1;
        fd_map[2] = r_fd;
    }
    
    int32_t pid = Spawn(elf, cmd, fd_count, fd_map);

    close(fd);

    if(pid == -1)
    {
        printf("Failed to execute: %s\n", cmd);
        return E_ERROR;
    }

    printf("Process %s created with pid: %d\n", path, pid);

    if(wait == TRUE)
    {
        WaitPid(pid);
    }

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

int32_t (*CommandHandlers[])(char *) =
    {
        SysInfoGet,
        KillPid,
    };

key_t key;

void clean(void* arg)
{
    printf("Clean %d...\n", arg);
}

void* TaskToCancel(void* arg)
{
    TaskCleanupPush(clean, arg);
    while(TRUE)
    {
        SchedYield();
    }
    TaskCleanupPop(1);

    return 0;
}

void* Task(void* val)
{
    key_set(key, (const void*) 50);
    printf("Task %d -> key[%d] = %d\n", GetTid() & 0xFFFF, key, key_get(key));
    printf("Task %d will exit with %x\n", GetTid() & 0xFFFF, (uint32_t)val);
    TaskCleanupPush(clean, (void*)12);
    TaskCleanupPush(clean, (void*)40);
    TaskCleanupPop(1);
    TaskExit(0);
    TaskCleanupPop(1);
    return val;
}

void destructor(void* key)
{
    printf("Deleting key %d...\n", key);
}

mutex_t dummy_lock = MUTEX_INITIALIZER;

sem_t sem;

void* lockTask(void* arg)
{
    TimeoutSet(500, TIMER_NO_RELOAD);
    //MutexLock(&dummy_lock);
    printf("Task timeout %d\n", SemWait(&sem));
    return 0;
}

mutex_t condMutex = MUTEX_INITIALIZER;
cond_t cond = COND_INITIALIZER;

void* condTask(void* arg)
{
    MutexLock(&condMutex);
    printf("Task %d started and waiting of cond variable\n", GetTid() & 0xFFFF);
    TimeoutSet((uint32_t)arg, TIMER_NO_RELOAD);
    CondWait(&cond, &condMutex);
    printf("Task %d waked form cond variable\n", GetTid() & 0xFFFF);
    MutexUnlock(&condMutex);
    return 0;
}


void* worker(void* arg)
{
    uint32_t val = (uint32_t)arg;

    printf("Worker started with: %d\n", val);

    while(1)
    {
        val += val;

        uint32_t i = val;
        for(; i > 0; --i);

    }

    return NULL;
}

int main()
{
    StdOpen();

    char buffer[256];
    char cmd[32];

    printf("\n");

    printf("CMD tid: %x\n", GetTid());
    printf("CMD errno: %x\n", errno);

    errno = 0xABCD;

    printf("CMD errno: %x\n", errno);

    key_create(&key, destructor);
    key_set(key, (const void*) 20);
    printf("Task %d -> key[%d] = %d\n", GetTid() & 0xFFFF, key, key_get(key));

    task_t task;
    taskAttr_t attr = {20, FALSE, 0x200000};
    TaskCreate(&task, &attr, Task, (void*)0xDEADFACE);

    task_t taskCancel;
    TaskCreate(&taskCancel, NULL, TaskToCancel, (void*)0x12345);

    SchedYield();

    TaskCancel(taskCancel);

    MutexLock(&dummy_lock);
    SemInit(&sem, 0x0, 0);
    task_t taskLock;
    TaskCreate(&taskLock, NULL, lockTask, (void*)0x12345);

    SchedYield();

    SemPost(&sem);

    // Conditional variables test
    {
        task_t taskCond1;
        task_t taskCond2;
        task_t taskCond3;
        taskAttr_t taskAttr = {20, FALSE, 0x200000};
        TaskCreate(&taskCond1, &taskAttr, condTask, (void*)0x1000);
        TaskCreate(&taskCond2, &taskAttr, condTask, (void*)0x1000);
        TaskCreate(&taskCond3, &taskAttr, condTask, (void*)0x1000);

        //CondSignal(&cond);
        //CondBroadcast(&cond);
    }


    task_t workers[5];
    taskAttr_t workerAttr = {10, FALSE, 0x200000};
    TaskCreate(&workers[0], &workerAttr, worker, (void*)1);
    TaskCreate(&workers[1], &workerAttr, worker, (void*)2);
    TaskCreate(&workers[2], &workerAttr, worker, (void*)3);
    TaskCreate(&workers[3], &workerAttr, worker, (void*)4);
    TaskCreate(&workers[5], &workerAttr, worker, (void*)5);


    while(TRUE)
    {
        printf("/> ");
        scanf("%s", buffer);

        CondBroadcast(&cond);

        uint32_t offset = CommandGet(buffer, cmd);
        uint32_t action = CommandEntriesLookup(cmd);

        if(action == Invalid)
        {
            // If the command is not valid try to find a process that fulfills this request
            Exec(buffer);
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
