#include "stdio.h"
#include "hi_unf_gpio.h"
#include "gpioCtrlApi.h"

static gpioCtrlM_t* pSgGpioCtrlM = NULL;

MS_S32 HI_UNF_GPIO_Init()
{
	pSgGpioCtrlM = gpioCtrlMMalloc();
	return 0;
}

MS_S32 HI_UNF_GPIO_DeInit()
{
    pSgGpioCtrlM->free(pSgGpioCtrlM);
	return 0;
}

MS_S32 HI_UNF_GPIO_WriteBit(MS_U32 u32GpioNo, MS_BOOL bHighVolt )
{
	pSgGpioCtrlM->write(pSgGpioCtrlM, u32GpioNo, bHighVolt);
    return 0;
}

MS_S32 HI_UNF_GPIO_ReadBit(MS_U32 u32GpioNo, MS_BOOL  *pbHighVolt)
{
	pSgGpioCtrlM->read(pSgGpioCtrlM, u32GpioNo, pbHighVolt);
    return 0;
}

MS_S32 HI_UNF_GPIO_SetDirBit(MS_U32 u32GpioNo, MS_BOOL bInput)
{
	pSgGpioCtrlM->setDir(pSgGpioCtrlM, u32GpioNo, !bInput);
    return 0;
}

MS_S32 HI_UNF_GPIO_Enable(MS_U32 u32GpioNo, MS_BOOL bInput)
{
	pSgGpioCtrlM->enable(pSgGpioCtrlM, u32GpioNo, bInput);
    return 0;
}

MS_S32 HI_UNF_GPIO_GetDirBit(MS_U32 u32GpioNo, MS_BOOL *pbInput)
{
    return 0;
}

void gpio_set_output_value(int pin, int value)
{
	pSgGpioCtrlM->setDir(pSgGpioCtrlM, pin, 1);
	pSgGpioCtrlM->enable(pSgGpioCtrlM, pin, 1);
	pSgGpioCtrlM->write(pSgGpioCtrlM, pin, value);
}

void gpio_set_input(int pin)
{
	pSgGpioCtrlM->setDir(pSgGpioCtrlM, pin, 0);
}

int gpin_read_value(int pin)
{
	int value = 0;
	pSgGpioCtrlM->read(pSgGpioCtrlM, pin, &value);
    return value;
}
