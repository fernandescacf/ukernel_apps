#include <led.h>

int32_t LedDriverInit(void)
{
    return E_OK;
}

void LedDriverClose(void)
{

}

int32_t LedSetState(int32_t state)
{
    (void)state;
    return E_OK;
}