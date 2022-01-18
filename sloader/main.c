#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <mman.h>
#include <task.h>

int32_t SerialLoad(void * addr, uint32_t size)
{
    printf("Loading file...");

    read(stdin, addr, size);

    printf("\n");

	return E_OK;
}

int32_t LoadFile(const char* cmd, size_t size)
{
    // Create File
    printf("Create file: %s\n", cmd);
    int32_t fd = open(cmd, O_CREAT | O_RDWR);

    if(fd == -1)
    {
        printf("Faild to create file: %s\n", cmd);
        return E_ERROR;
    }

    // Set File Size
    if(ftruncate(fd, size) != E_OK)
    {
        close(fd);
        printf("Error truncating %s to size %d\n", cmd, size);
        return E_ERROR;
    }

    // Get Access to the file memory
    char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x0);

    if(data == NULL || SerialLoad(data, size) != E_OK)
    {
        close(fd);
        printf("Failed to load file: %s\n", cmd);
        return E_ERROR;
    }

    close(fd);

    return E_OK;
}

void* LoaderTask(void* arg)
{
    char** argv = (char**)arg;

    char *remaining;
    size_t size = strtoul(argv[2], &remaining, 0);

    if(*remaining != 0 || size == 0)
    {
        printf("Invalid size: %d\n", size);
        return (void*)E_ERROR;
    }

    return (void*)LoadFile(argv[1], size);
}

int main(int argc, const char* argv[])
{
    if(argc != 3)
    {
        printf("Invalid parameters\n");
        return E_INVAL;;
    }

    task_t loader;
    // Put high priority to avoid lossing data
    taskAttr_t attr = {20, FALSE, 0x200000};
    TaskCreate(&loader, &attr, LoaderTask, (void*)argv);
    TaskJoin(loader, NULL);
    return 0;

/*
    char *remaining;
    size_t size = strtoul(argv[2], &remaining, 0);

    if(*remaining != 0 || size == 0)
    {
        printf("Invalid size: %d\n", size);
        return E_ERROR;
    }

    return LoadFile(argv[1], size);
*/
}