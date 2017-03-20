/*
**********************************************************************
*
*                            uC/MMC
*
*             (c) Copyright 2005 - 2007, Ingenic Semiconductor, Inc
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : mmc_config.h 
Purpose     : Define functions for the mmc/sd programs.

----------------------------------------------------------------------
Version-Date-----Author-Explanation
----------------------------------------------------------------------
1.00.00 20060831 WeiJianli     First release

----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
(none)
---------------------------END-OF-HEADER------------------------------
*/

#ifndef __MMC_CONFIG__
#define __MMC_CONFIG__
/* Data Type Definitions */

#define MMC_DEBUG_LEVEL     0		/* Enable Debug: 0 - no debug */

#define MMC_UCOSII_EN		0		/* Enable UCOS */

#define MMC_DMA_ENABLE		0       /*Enable DMA*/

#define MMC_BLOCKSIZE 		512		/* MMC/SD Block Size */

#define MMC_OCR_ARG 		 0x00ff8000	/* Argument of OCR */


//define the card exsit's level

#define MMC_EXSIT           1


#endif /* __MMC_CONFIG__ */
