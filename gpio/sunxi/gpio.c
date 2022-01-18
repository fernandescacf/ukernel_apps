/**
 * @file        gpio.c
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        18 March, 2018
 * @brief       Sunxi Allwiner H3 GPIO Driver file
*/



/* Includes ----------------------------------------------- */
#include <gpio.h>
#include <misc.h>

#include <mman.h>

/* Private types ------------------------------------------ */

struct sunxi_gpio {
	volatile uint32_t cfg[4];
	volatile uint32_t dat;
	volatile uint32_t drv[2];
	volatile uint32_t pull[2];
};

struct sunxi_gpio_reg {
	struct sunxi_gpio gpio_bank[10];
};


/* Private constants -------------------------------------- */

#define SUNXI_PIO_BASE		0x01C20800
#define SUNXI_R_PIO_BASE	0x01F02C00


/* Private macros ----------------------------------------- */

#define BANK_TO_GPIO(bank)	(((bank) < GPIO_L) ? \
	&(sunxiPioBase)->gpio_bank[bank] : 			 \
	&(sunxiRpioBase)->gpio_bank[(bank) - GPIO_L])

#define GPIO_BANK(pin)		((pin) >> 5)
#define GPIO_NUM(pin)		((pin) & 0x1F)

#define GPIO_CFG_INDEX(pin)	(((pin) & 0x1F) >> 3)
#define GPIO_CFG_OFFSET(pin)	((((pin) & 0x1F) & 0x7) << 2)

#define GPIO_PULL_INDEX(pin)	(((pin) & 0x1f) >> 4)
#define GPIO_PULL_OFFSET(pin)	((((pin) & 0x1f) & 0xf) << 1)


/* Private variables -------------------------------------- */

static struct sunxi_gpio_reg *sunxiPioBase;
static struct sunxi_gpio_reg *sunxiRpioBase;

/* Private function prototypes ---------------------------- */



/* Private functions -------------------------------------- */

int32_t GpioDriverInit(void){
	// Map GPIO
	sunxiPioBase = (struct sunxi_gpio_reg *)mmap(NULL, 1024, PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_PHYS | MAP_SHARED, NOFD, SUNXI_PIO_BASE);

	if(NULL == sunxiPioBase)
	{
		return E_NO_RES;
	}
	// Map RGPIO

	return E_OK;
}

void GpioDriverClose()
{
    munmap(sunxiPioBase, 1024);
}

/**
 * GpioSetCfgpin Implementation (See header file for description)
*/
int32_t GpioSetCfgpin(uint32_t pin, uint32_t val)
{
	uint32_t cfg;
	uint32_t bank = GPIO_BANK(pin);
	uint32_t index = GPIO_CFG_INDEX(pin);
	uint32_t offset = GPIO_CFG_OFFSET(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	cfg = readl(&pio->cfg[0] + index);
	cfg &= ~(0xf << offset);
	cfg |= val << offset;
	writel(cfg, &pio->cfg[0] + index);
	return 0;
}

int32_t GpioSetPull(uint32_t pin, uint32_t val)
{
	uint32_t cfg;
	uint32_t bank = GPIO_BANK(pin);
	uint32_t index = GPIO_PULL_INDEX(pin);
	uint32_t offset = GPIO_PULL_OFFSET(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	cfg = readl(&pio->pull[0] + index);
	cfg &= ~(0x3 << offset);
	cfg |= val << offset;
	writel(cfg, &pio->pull[0] + index);
	return 0;
}

int32_t GpioOutput(uint32_t pin, uint32_t val)
{
	uint32_t dat;
	uint32_t bank = GPIO_BANK(pin);
	uint32_t num = GPIO_NUM(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	dat = readl(&pio->dat);
	if(val)
		dat |= 1 << num;
	else
		dat &= ~(1 << num);
	writel(dat, &pio->dat);
	return 0;
}

int32_t GpioInput(uint32_t pin)
{
	uint32_t dat;
	uint32_t bank = GPIO_BANK(pin);
	uint32_t num = GPIO_NUM(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	dat = readl(&pio->dat);
	dat >>= num;
	return (dat & 0x1);
}
