#include <types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define LED_PATH    "/dev/gpio/15"
#define LED_ON      "ON"
#define LED_OFF     "OFF"


int main(int argc, const char* argv[])
{
    StdOpen();

    if(argc < 2)
    {
        printf("\nERROR: Missing gpio value\n");
        return -1;
    }

    int32_t value;
    if(!strcmp(argv[1], LED_ON)) value = 1;
    else if(!strcmp(argv[1], LED_OFF)) value = 0;
    else
    {
        printf("\nERROR: Invalid gpio value: %s\n", argv[1]);
        return -1;
    }

    int32_t fd = open(LED_PATH, O_WRONLY);
    if(fd == -1)
    {
        printf("\nERROR: Failed to connect to: %s\n", LED_PATH);
        return -1;
    }

    if(write(fd, (char*)&value, sizeof(int32_t)) != sizeof(int32_t))
    {
        printf("\nERROR: Failed to write to: %s\n", LED_PATH);
        return -1;
    }

    close(fd);

    StdClose();

    return 0;
}