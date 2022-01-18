/**
 * @file        timer.c
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        18 December, 2018
 * @brief		Sunxi H3 Led Driver implementation
*/

/* Includes ----------------------------------------------- */
#include <timer.h>
#include <interrupt.h>
#include <mman.h>

/* Private types ------------------------------------------ */

typedef struct {
	volatile uint32_t irqen;				// 0x00 - Timer IRQ Enable Register
	volatile uint32_t irqsta;				// 0x04 - Timer Status Register
	volatile uint32_t reserved[2];
	struct{
		volatile uint32_t ctrl;				// Timer Control Register
		volatile uint32_t intv;				// Timer Interval Value Register
		volatile uint32_t cur;				// Timer 0 Current Value Register
		volatile uint32_t reserved;
	}timer[2];								// Timer 0 & Timer 1
}h3_timer_t;


/* Private constants -------------------------------------- */

#define TIMER_BASE			(0x01C20C00)
#define TIMER_SIZE			(0x0400)

#define NUM_TIMERS 				2
#define TIMER_0					0
#define TIMER_1					1

#define TIMER0_IRQ      		50
#define TIMER1_IRQ      		51

#define	CTRL_ENABLE				(0x1 << 0)
#define	CTRL_RELOAD				(0x1 << 1)
#define	CTRL_SRC_32K			(0x0 << 2)
#define	CTRL_SRC_24M			(0x1 << 2)

#define	CTRL_PRE_1				(0x0 << 4)
#define	CTRL_PRE_2				(0x1 << 4)
#define	CTRL_PRE_4				(0x2 << 4)
#define	CTRL_PRE_8				(0x3 << 4)
#define	CTRL_PRE_16				(0x4 << 4)
#define	CTRL_PRE_32				(0x5 << 4)
#define	CTRL_PRE_64				(0x6 << 4)
#define	CTRL_PRE_128			(0x7 << 4)

#define	CTRL_AUTO				(0x0 << 7)
#define	CTRL_SINGLE				(0x1 << 7)

#define CLOCK_24M				24000000

#define TIMER_HZ_VALUE(hz)		(CLOCK_24M/hz)
#define TIMER_USEC_VALUE(usec)	(24 * (usec))
#define TIMER_AUTO_MODE			(0)
#define TIMER_SINGLESHOT_MODE	(1)

#define TIMER					TIMER_1
#define TIMER_INTERRUPT			TIMER1_IRQ

/* Private macros ----------------------------------------- */


/* Private variables -------------------------------------- */
static h3_timer_t* h3Timers = NULL;

struct
{
	int32_t intr_id;
	void* (*handler)(void*, uint32_t);
	const void *arg;
}TimerHandler;

/* Private function prototypes ---------------------------- */

int32_t TimerConfig(uint32_t timerId, uint32_t loadValue, uint32_t config);

void TimerInterruptEnable(uint32_t timerId);

void TimerInterruptAck(uint32_t timerId);

void *TimerISR(void* arg, uint32_t interrupt);

/* Private functions -------------------------------------- */

int32_t TimerInit(uint32_t mode)
{
	h3Timers = (h3_timer_t*)mmap(NULL, TIMER_SIZE, (PROT_READ | PROT_WRITE | PROT_NOCACHE), (MAP_PHYS | MAP_SHARED), NOFD, TIMER_BASE);

	if(h3Timers == NULL)
	{
		return E_ERROR;
	}

	uint32_t loadValue = TIMER_USEC_VALUE(0);

	switch (mode)
	{
		case AUTO_RELOAD_TIMER :
			(void)TimerConfig(TIMER, loadValue, TIMER_AUTO_MODE);
			break;

		case ONE_SHOT_TIMER :
			(void)TimerConfig(TIMER, loadValue, TIMER_SINGLESHOT_MODE);
			break;

		case DISABLE_TIMER :
			h3Timers->timer[TIMER].ctrl &= ~CTRL_ENABLE;
			break;
		default:
			break;
	}

	return E_OK;
}

int32_t TimerEnableInterrupt(void* (*handler)(void*, uint32_t), const void *arg)
{
	void *timerArg = NULL;

	TimerInterruptEnable(TIMER);

	TimerHandler.handler = handler;
	TimerHandler.arg = arg;

	if(handler != NULL)
	{
		timerArg = (void*)&TimerHandler;
	}

	TimerHandler.intr_id = InterruptAttach(TIMER_INTERRUPT, 10, TimerISR, timerArg);

	return E_OK;
}

void *TimerISR(void* arg, uint32_t interrupt)
{
	if(arg != NULL)										// Call user handler
	{
		TimerHandler.handler((void*)TimerHandler.arg, interrupt);
	}

    TimerInterruptAck(TIMER);

	return NULL;
}

int32_t TimerStart(uint32_t u_sec)
{
	h3Timers->timer[TIMER].intv = TIMER_USEC_VALUE(u_sec);

    h3Timers->timer[TIMER].ctrl |= CTRL_ENABLE;

	return E_OK;
}

int32_t TimerStop()
{
	h3Timers->timer[TIMER].ctrl &= ~CTRL_ENABLE;

	return E_OK;
}

int32_t TimerKill()
{
	TimerStop();
	InterrupDetach(TimerHandler.intr_id);
	munmap(h3Timers, TIMER_SIZE);
	h3Timers = NULL;

	return E_OK;
}

int32_t TimerTickWait()
{
	return InterruptWait(TimerHandler.intr_id);
}

int32_t TimerConfig(uint32_t timerId, uint32_t loadValue, uint32_t config)
{
	h3Timers->timer[timerId].intv = loadValue;
	h3Timers->timer[timerId].ctrl = 0x0;

	// By default disable the interrupt
	h3Timers->irqen &= (~(1 << timerId));

	if(TIMER_SINGLESHOT_MODE == config)
	{
		h3Timers->timer[timerId].ctrl |= CTRL_SINGLE;
	}

	h3Timers->timer[timerId].ctrl |= CTRL_SRC_24M;
	h3Timers->timer[timerId].ctrl |= CTRL_RELOAD;

	while (h3Timers->timer[timerId].ctrl & CTRL_RELOAD){}

	return 0;
}

void TimerInterruptEnable(uint32_t timerId)
{
	h3Timers->irqen |= (1 << timerId);
}

void TimerInterruptAck(uint32_t timerId)
{
	h3Timers->irqsta |= (1 << timerId);
}
