#ifndef _TIMER_H_
#define _TIMER_H_

#include <types.h>

/*
 * Timer mode macro. Used as a parameter for timer_init()
 * Used to set the local timer to auto reload mode.
 */
#define AUTO_RELOAD_TIMER  		0

/*
 * Timer mode macro. Used as a parameter for timer_init()
 * Used to set the local timer to single shot mode.
 */
#define ONE_SHOT_TIMER     		1

/*
 * Timer mode macro. Used as a parameter for timer_init()
 * Used to disable the timer.
 */
#define DISABLE_TIMER      		2


int32_t TimerInit(uint32_t mode);

int32_t TimerEnableInterrupt(void* (*handler)(void*, uint32_t), const void *arg);

int32_t TimerStart(uint32_t u_sec);

int32_t TimerStop();

int32_t TimerKill();

int32_t TimerTickWait();

#endif /* _TIMER_H_ */
