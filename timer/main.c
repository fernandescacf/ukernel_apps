#include <types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <timer.h>
#include <led.h>

static volatile uint32_t tick = 0;

void* TimerUserHandler(void *arg, uint32_t intr)
{
    (void)arg; (void)intr;

    tick++;
    
    return NULL;
}

int main(int argc, const char* argv[])
{
    printf("\nBlinker example started\n");

    printf("Open Led Driver\n");
    if(LedDriverInit() != E_OK)
    {
        printf("ERROR: Failed to open Led Driver\n");
        return -1;
    }

    printf("Initialize Timer\n");
    if(TimerInit(AUTO_RELOAD_TIMER) != E_OK)
    {
        printf("ERROR: Failed to Initialize Timer\n");
        return -1;
    }

    printf("Install Timer Interrupt\n");
    TimerEnableInterrupt(TimerUserHandler, (const void *)&tick);

    printf("Start Timer at 0.5Hz\n");
    TimerStart(500000);

    int32_t led = LED_OFF;

    while(TRUE)
    {
        TimerTickWait();

        LedSetState(led);

        led = ((led == LED_ON) ? (LED_OFF) : (LED_ON));
    }

    TimerKill();

    LedDriverClose();

    return 0;
}
