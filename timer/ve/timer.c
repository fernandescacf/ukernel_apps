#include <timer.h>
#include <interrupt.h>
#include <mman.h>


#define TIMER_INTERRUPT 		(34)
#define TIMER_EN       			(0b1 << 7)
#define TIMER_PERIODIC     		(0b1 << 6)
#define TIMER_INT_EN          	(0b1 << 7)
#define TIMER_PSC1          	(0b00 << 2)
#define TIMER_PSC16          	(0b01 << 2)
#define TIMER_PSC256          	(0b10 << 2)
#define TIMER_32B				(0b1 << 1)
#define TIMER_ONE_SHOT     		(0b1 << 0)
#define TIMER_IRQ_CLEAR 		(0xFF)


#define CLOCK_FREQUENCY					(800000000)		// 800MHz
#define SYSCTRL_BASE					(0x1c020000)	//  SP810 System Controller Register 0
#define SYSCTRL_SIZE					(0x10000)
#define SCCTRL0_TIMEREN0SEL_TIMCLK		(0b1 << 15)
#define SP804_TIMER0_BASE				(0x1c110000)	// SP804 Timer0 Base Address Register
#define SP804_TIMER0_SIZE				(0x10000)


typedef struct
{
    volatile uint32_t pt_load_reg;
    volatile uint32_t pt_value_reg;
    volatile uint32_t pt_control_reg;
    volatile uint32_t pt_interrupt_clear_reg;
    volatile uint32_t pt_raw_interrupt_status_reg;
    volatile uint32_t pt_mask_interrupt_status_reg;
    volatile uint32_t pt_background_load_reg;
} sp804_timer;


static sp804_timer *timer;

struct
{
	int32_t intr_id;
	void* (*handler)(void*, uint32_t);
	const void *arg;
}TimerHandler;

void *TimerISR(void* arg, uint32_t interrupt);

int32_t TimerInit(uint32_t mode)
{
	uint32_t init_done = FALSE;
	uint32_t scctrl0;
	uint32_t *scctrl;
	uint32_t scctrl0_timeren_timclk = SCCTRL0_TIMEREN0SEL_TIMCLK;
	uint32_t tim_ctrl;

	scctrl = (uint32_t*)  mmap(NULL, SYSCTRL_SIZE, (PROT_READ | PROT_WRITE | PROT_NOCACHE), (MAP_PHYS | MAP_SHARED), NOFD, SYSCTRL_BASE);
	timer = (sp804_timer*)mmap(NULL, SP804_TIMER0_SIZE, (PROT_READ | PROT_WRITE | PROT_NOCACHE), (MAP_PHYS | MAP_SHARED), NOFD, SP804_TIMER0_BASE);

	/* Read-Modify-Write operation for Config TIMCLK for SP804 timers */
	scctrl0 = (*scctrl);	// Read SCCTRL register

	init_done = ((scctrl0 & scctrl0_timeren_timclk) ? (TRUE) : (FALSE));

	if(init_done == FALSE) // Clock already selected then Return
	{
		scctrl0 |= scctrl0_timeren_timclk;	// Select 1MHz as reference for Timerx

		*scctrl = scctrl0;	// Write SCCTRL register
	}

	/* Config SP804 Peripheral now */
	timer->pt_load_reg = 0xFFFFFFFF;	// Max value by default
	timer->pt_value_reg = 0xFFFFFFFF;	// Max value by default
	timer->pt_interrupt_clear_reg = 0xFFFFFFFF;	// Clear interrupts by default

	switch (mode)
	{
		case AUTO_RELOAD_TIMER :
			tim_ctrl = timer->pt_control_reg;
			tim_ctrl |= TIMER_INT_EN | TIMER_PERIODIC | TIMER_32B;
			timer->pt_control_reg = tim_ctrl;
			break;

		case ONE_SHOT_TIMER :
			tim_ctrl = timer->pt_control_reg;
			tim_ctrl |= TIMER_INT_EN | TIMER_ONE_SHOT | TIMER_32B;
			timer->pt_control_reg = tim_ctrl;
			break;

		case DISABLE_TIMER :
		default:
			tim_ctrl = timer->pt_control_reg;
			tim_ctrl &= ~TIMER_EN;
			timer->pt_control_reg = tim_ctrl;

	}

	// No longer needed
	munmap(scctrl, SYSCTRL_SIZE);

	return E_OK;
}

int32_t TimerEnableInterrupt(void* (*handler)(void*, uint32_t), const void *arg)
{
	void *timerArg = NULL;

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
    uint32_t temp = timer->pt_control_reg;

    if (!(temp & TIMER_PERIODIC)) 						// Only if timer is not reload
    {
        temp &= ~TIMER_EN;       						// Disable timer
        timer->pt_control_reg = temp;
    }

	if(arg != NULL)										// Call user handler
	{
		TimerHandler.handler((void*)TimerHandler.arg, interrupt);
	}

    timer->pt_interrupt_clear_reg = TIMER_IRQ_CLEAR;

	return NULL;
}

int32_t TimerStart(uint32_t u_sec)
{
    timer->pt_load_reg = u_sec;

	uint32_t tim_ctrl = timer->pt_control_reg;
	tim_ctrl |= TIMER_EN;
	timer->pt_control_reg = tim_ctrl;

	return E_OK;
}

int32_t TimerStop()
{
	uint32_t tim_ctrl = timer->pt_control_reg;
	tim_ctrl &= ~TIMER_EN;
	timer->pt_control_reg = tim_ctrl;

	return E_OK;
}

int32_t TimerKill()
{
	TimerStop();
	InterrupDetach(TimerHandler.intr_id);
	munmap(timer, SP804_TIMER0_SIZE);
	timer = NULL;

	return E_OK;
}

int32_t TimerTickWait()
{
	return InterruptWait(TimerHandler.intr_id);
}
