/*******************************************************************************
*   File: mmc_api.c
*   Version: 1.0
*   Author: jz
*   Date: 20060831
*   Description: first version
*
*-------------------------------------------------------------------------------
*   Modified by: dy
*   M_date:
*   M_version: 1.01
*   Description:
*******************************************************************************/

#include "ds2_msc_config.h"
#include "ds2_msc_protocol.h"
#include "ds2_msc_api.h"
#include "ds2_msc_core.h"

/* the information structure of MMC/SD Card */
mmc_info mmcinfo;
extern int sd2_0;
extern int limit_41;
extern int num_6;

#if MMC_UCOSII_EN
static OS_EVENT *pSemMMC = NULL;	/* semaphore that access card */
#endif

/*******************************************************************************************************************
** Name:	  void mmc_enter()
** Function:      request the right of operating mmc to OS
** Input:	  NULL
** return:	  NULL
********************************************************************************************************************/
static void mmc_enter(void)
{
#if MMC_UCOSII_EN
	unsigned char ret;
	OSSemPend(pSemMMC, 0, &ret);	/* wait for semaphore that accessed Card */
#endif
}

/*******************************************************************************************************************
** Name:	  void mmc_exit()
** Function:      return the right of operating mmc to OS
** Input:	  NULL
** return:	  NULL
********************************************************************************************************************/
static void mmc_exit(void)
{
#if MMC_UCOSII_EN
	OSSemPost(pSemMMC);	/* return the semaphore accessing Card to OS */
#endif
}

/*******************************************************************************
** Name:	  int MMC_Initialize()
** Function:      initialize MMC/MMC card
** Input:	  NULL
** Output:	  0:  right			>0:  error code
*******************************************************************************/
int MMC_Initialize(void)
{
	num_6 = 0;
	limit_41 = 0;
	sd2_0 = 0;

#if MMC_UCOSII_EN
	if (pSemMMC == NULL) {
		pSemMMC = OSSemCreate(1);	/* create MMC/SD semaphore */
		if (pSemMMC == NULL)
			return MMC_NO_RESPONSE;
	}
#endif

	mmc_enter();		/* Claim the lock */

	if(_mmc_hardware_init() != MMC_NO_ERROR)	/* Initialize the hardware that access SD Card */
    {
		mmc_exit();
		return MMC_NO_RESPONSE;
    }

	if (_mmc_chkcard() != 1) {	/* Check weather card is inserted */
		mmc_exit();
// printf("No card\n");
		return MMC_NO_RESPONSE;
	}

	if (mmc_init_card()) {	/* Initialize and detect SD card */
		mmc_exit();
		return MMC_NO_RESPONSE;
	}

	mmc_select_card();	/* Select the SD card */

	mmc_exit();		/* Release the lock */
// printf("MMC ----OK\n");
	return MMC_NO_ERROR;
}

/******************************************************************
//Note: MMC_GetSize() is important in mmc_drv.c
//return unit is 512-byte scetors
//so the maxinum capacitye is 2TB
******************************************************************/
unsigned int MMC_GetSize(void)
{
	unsigned int size;
    if(!sd2_0)
    {
    	size =
    	    (mmcinfo.csd.c_size +
    	     1) * (1 << (mmcinfo.csd.c_size_mult +
    			 2)) * (1 << mmcinfo.csd.read_bl_len);
        size >>= 9; //512-byte sector
    }
    else
    {
        size = (mmcinfo.csd.c_size + 1) * 512;
        size <<= 1; //512-byte sector
    }

	return size;
}

/******************************************************************
 *
 * Read a single block from SD/MMC card
 *
 ******************************************************************/
int MMC_ReadBlock(unsigned int blockaddr, unsigned char * recbuf)
{
	struct mmc_request request;
	struct mmc_response_r1 r1;
	int retval;

//    printf("MMC_ReadBlock\n"); 

	mmc_enter();		/* request semaphore acessed SD/MMC to OS */

	if (_mmc_chkcard() != 1) {
		mmc_exit();
		return MMC_NO_RESPONSE;	/* card is not inserted entirely */
	}

	if (blockaddr > mmcinfo.block_num) {
		mmc_exit();
		return MMC_ERROR_OUT_OF_RANGE;	/* operate over the card range */
	}

	mmc_simple_cmd(&request, MMC_SEND_STATUS, mmcinfo.rca,
		       RESPONSE_R1);
	retval = mmc_unpack_r1(&request, &r1, 0);
	if (retval && (retval != MMC_ERROR_STATE_MISMATCH)) {
		mmc_exit();
		return retval;
	}

	mmc_simple_cmd(&request, MMC_SET_BLOCKLEN, MMC_BLOCKSIZE,
		       RESPONSE_R1);
	if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
		mmc_exit();
		return retval;
	}
	if (sd2_0) {
//        printf("single mmc_send_cmd1 in\n");
		mmc_send_cmd(&request, MMC_READ_SINGLE_BLOCK, blockaddr, 1,
			     MMC_BLOCKSIZE, RESPONSE_R1, recbuf);
//        printf("single mmc_send_cmd1 over\n");
		if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
			mmc_exit();
			return retval;
		}
	} else {
//        printf("single mmc_send_cmd2 in\n");
		mmc_send_cmd(&request, MMC_READ_SINGLE_BLOCK,
			     blockaddr * MMC_BLOCKSIZE, 1, MMC_BLOCKSIZE,
			     RESPONSE_R1, recbuf);
//        printf("single mmc_send_cmd2 over\n");
		if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
			mmc_exit();
			return retval;
		}
	}
	mmc_exit();		/* return semaphore acessed MMC/SD to OS */

	return retval;
}

/******************************************************************
 *
 * Read multiple blocks from MMC/MMC card
 *
 ******************************************************************/
int MMC_ReadMultiBlock(unsigned int blockaddr, unsigned int blocknum, unsigned char * recbuf)
{
	struct mmc_request request;
	struct mmc_response_r1 r1;
	int retval;
    
//    printf("MMC_ReadMultiBlock\n"); 

	mmc_enter();		/* request semaphore acessed SD/MMC to OS */
	if (_mmc_chkcard() != 1) {
		mmc_exit();
		return MMC_NO_RESPONSE;	/* card is not inserted entirely */
	}

	if ((blockaddr + blocknum) > mmcinfo.block_num) {
		mmc_exit();
		return MMC_ERROR_OUT_OF_RANGE;	/* operate over the card range */
	}

	mmc_simple_cmd(&request, MMC_SEND_STATUS, mmcinfo.rca,
		       RESPONSE_R1);
	retval = mmc_unpack_r1(&request, &r1, 0);
	if (retval && (retval != MMC_ERROR_STATE_MISMATCH)) {
		mmc_exit();
		return retval;
	}

	mmc_simple_cmd(&request, MMC_SET_BLOCKLEN, MMC_BLOCKSIZE,
		       RESPONSE_R1);
	if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
		mmc_exit();
		return retval;
	}
	if (sd2_0) {
//        printf("multi mmc_send_cmd1 in\n");
		mmc_send_cmd(&request, MMC_READ_MULTIPLE_BLOCK, blockaddr,
			     blocknum, MMC_BLOCKSIZE, RESPONSE_R1, recbuf);
//        printf("multi mmc_send_cmd1 over\n");
		if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
			mmc_exit();
			return retval;
		}
	} else {
//        printf("multi mmc_send_cmd2 in\n");
        mmc_send_cmd(&request, MMC_READ_MULTIPLE_BLOCK,
			     blockaddr * MMC_BLOCKSIZE, blocknum,
			     MMC_BLOCKSIZE, RESPONSE_R1, recbuf);
//        printf("multi mmc_send_cmd2 over\n");
		if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
			mmc_exit();
			return retval;
		}

	}

	mmc_simple_cmd(&request, MMC_STOP_TRANSMISSION, 0, RESPONSE_R1B);
	if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
		mmc_exit();
		return retval;
	}

	mmc_exit();

	return retval;
}

/******************************************************************
 *
 * Write a block to SD/MMC card
 *
 ******************************************************************/
int MMC_WriteBlock(unsigned int blockaddr, unsigned char * sendbuf)
{
	struct mmc_request request;
	struct mmc_response_r1 r1;
	int retval;

	mmc_enter();		/* request semaphore acessed MMC/SD to OS */
	if (_mmc_chkcard() != 1) {
		mmc_exit();
		return MMC_NO_RESPONSE;	/* card is not inserted entirely */
	}

	if (blockaddr > mmcinfo.block_num) {
		mmc_exit();
		return MMC_ERROR_OUT_OF_RANGE;	/* operate over the card range */
	}

	if (_mmc_chkcardwp() == 1) {
		mmc_exit();
		return MMC_ERROR_WP_VIOLATION;	/* card is write protected */
	}

	mmc_simple_cmd(&request, MMC_SEND_STATUS, mmcinfo.rca,
		       RESPONSE_R1);
	retval = mmc_unpack_r1(&request, &r1, 0);
	if (retval && (retval != MMC_ERROR_STATE_MISMATCH)) {
		mmc_exit();
		return retval;
	}

	mmc_simple_cmd(&request, MMC_SET_BLOCKLEN, MMC_BLOCKSIZE,
		       RESPONSE_R1);
	if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
		mmc_exit();
		return retval;
	}
	if (sd2_0) {
		mmc_send_cmd(&request, MMC_WRITE_BLOCK, blockaddr, 1,
			     MMC_BLOCKSIZE, RESPONSE_R1, sendbuf);
		if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
			mmc_exit();
			return retval;
		}
	} else {
		mmc_send_cmd(&request, MMC_WRITE_BLOCK,
			     blockaddr * MMC_BLOCKSIZE, 1, MMC_BLOCKSIZE,
			     RESPONSE_R1, sendbuf);
		if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
			mmc_exit();
			return retval;
		}

	}

	mmc_exit();
	return retval;
}

/******************************************************************
 *
 * Write multiple blocks to SD/MMC card
 *
 ******************************************************************/
int MMC_WriteMultiBlock(unsigned int blockaddr, unsigned int blocknum, unsigned char * sendbuf)
{
	struct mmc_request request;
	struct mmc_response_r1 r1;
	int retval;

	mmc_enter();		/* request semaphore acessed SD/MMC to OS */
	if (_mmc_chkcard() != 1) {
		mmc_exit();
		return MMC_NO_RESPONSE;	/* card is not inserted entirely */
	}

	if (blockaddr > mmcinfo.block_num) {
		mmc_exit();
		return MMC_ERROR_OUT_OF_RANGE;	/* operate over the card range */
	}

	if (_mmc_chkcardwp() == 1) {
		mmc_exit();
		return MMC_ERROR_WP_VIOLATION;	/* card is write protected */
	}

	mmc_simple_cmd(&request, MMC_SEND_STATUS, mmcinfo.rca,
		       RESPONSE_R1);
	retval = mmc_unpack_r1(&request, &r1, 0);
	if (retval && (retval != MMC_ERROR_STATE_MISMATCH)) {
		mmc_exit();
		return retval;
	}

	mmc_simple_cmd(&request, MMC_SET_BLOCKLEN, MMC_BLOCKSIZE,
		       RESPONSE_R1);
	if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
		mmc_exit();
		return retval;
	}

	if (sd2_0) {
		mmc_send_cmd(&request, MMC_WRITE_MULTIPLE_BLOCK, blockaddr,
			     blocknum, MMC_BLOCKSIZE, RESPONSE_R1,
			     sendbuf);
		if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
			mmc_exit();
			return retval;
		}
	} else {
		mmc_send_cmd(&request, MMC_WRITE_MULTIPLE_BLOCK,
			     blockaddr * MMC_BLOCKSIZE, blocknum,
			     MMC_BLOCKSIZE, RESPONSE_R1, sendbuf);
		if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
			mmc_exit();
			return retval;
		}

	}
	mmc_simple_cmd(&request, MMC_STOP_TRANSMISSION, 0, RESPONSE_R1B);
	if ((retval = mmc_unpack_r1(&request, &r1, 0))) {
		mmc_exit();
		return retval;
	}
	mmc_exit();
	return retval;
}


#ifndef USE_MIDWARE
/******************************************************************
 *
 * Detect SD/MMC card
 *
 ******************************************************************/
int MMC_DetectStatus(void)
{
	if (_mmc_chkcard())
		return 0;	/* card exits */

	return 1;		/* card does not exit */
}

#endif
