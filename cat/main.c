#include <types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, const char* argv[])
{
    if(argc != 2)
    {
        printf("No file specified\n");
        return E_INVAL;
    }

    int32_t fd = open(argv[1], O_RDONLY);

    if(fd == -1)
    {
        printf("File %s not found\n", argv[1]);
        return E_INVAL;
    }

    // Print 200 bytes at each time to avoid buffer overflow in printf
    const uint32_t MAX_PRINT_SIZE = 200;
    char buffer[MAX_PRINT_SIZE + 1];
    // Add termination character to buffer
    buffer[MAX_PRINT_SIZE] = '\0';

    while(read(fd, buffer, MAX_PRINT_SIZE) > 0)
    {
        printf("%s", buffer);
    }

    close(fd);

    return E_OK;
}