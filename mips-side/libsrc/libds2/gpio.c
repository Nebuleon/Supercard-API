/*
 * gpio.c
 *
 * Init GPIO pins for PMP board.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "bsp.h"
#include "jz4740.h"

void _gpio_init(void)
{
#if USE_UART0
	__gpio_as_uart0();
#endif
#if USE_UART1
//	__gpio_as_uart1();
#endif
#if USE_NAND
	__gpio_as_nand();
#endif

#if USE_AIC
//	__gpio_as_aic();
#endif
#if USE_CIM
//	__gpio_as_cim();
#endif
}
