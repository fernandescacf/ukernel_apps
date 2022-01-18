/**
 * @file        led.c
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        28 June, 2020
 * @brief		Sunxi Systimer implementation
*/

/* Includes ----------------------------------------------- */
#include <led.h>
#include <fcntl.h>
#include <unistd.h>


/* Private types ------------------------------------------ */


/* Private constants -------------------------------------- */
#define LED_PATH    "/dev/gpio/15"


/* Private macros ----------------------------------------- */


/* Private variables -------------------------------------- */
static int32_t gpio_fd = -1;


/* Private function prototypes ---------------------------- */
int32_t LedDriverInit(void)
{
    if(gpio_fd != -1)
    {
        return E_BUSY;
    }

    gpio_fd = open(LED_PATH, O_WRONLY);

    if(gpio_fd == -1)
    {
        return E_ERROR;
    }

    return E_OK;
}

void LedDriverClose(void)
{
    if(gpio_fd != -1)
    {
        close(gpio_fd);
        gpio_fd = -1;
    }
}

int32_t LedSetState(int32_t state)
{
    if(gpio_fd == -1)
    {
        return E_ERROR;
    }

    if(write(gpio_fd, (char*)&state, sizeof(int32_t)) != sizeof(int32_t))
    {
        return E_ERROR;
    }

    return E_OK;
}
