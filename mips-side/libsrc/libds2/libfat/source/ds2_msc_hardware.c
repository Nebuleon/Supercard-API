/*******************************************************************************
*   File: mmc_jz4740.c
*   Version: 1.0
*   Author: jz
*   Date: 20060831
*   Description: first version
*
*-------------------------------------------------------------------------------
*   Modified by: dy
*   M_date:
*   M_version: 1.1
*   Description: use only 1 data bus that would be running well @ SD0 controler

*-------------------------------------------------------------------------------
*   Modified by: dy
*   M_date: 20100306
*   M_version: 1.2
*   Description: use only 1 data bus that would be running well @ SD0 controler
*                int jz_mmc_hardware_init(void) return problem
*******************************************************************************/


/*******************************************************************************
*   SD/MMC controler porting information
*   __gpio_as_msc   (file: jz4740.h)
*   MMC_CD_PIN      (file: this)
*   __cpm_start_msc (file: this)
*   MSC_BASE        (file: jz4740.h)
*   DMAC_DRSR_RS_MSCOUT	(file: jz4740.h)
*   DMAC_DRSR_RS_MSCIN  (file: jz4740.h)
*   IRQ_MSC         (file: jz4740.h)
*   REG_CPM_MSCCDR  (file: jz4740.h)
*******************************************************************************/

#include "ds2_msc_config.h"
#include "ds2_msc_protocol.h"
#include "ds2_msc_api.h"
#include "ds2_msc_core.h"

/***************************************************************************
 * Define for DMA mode
 *
 */
#include "jz4740.h"
#include <unistd.h>
#include <asm/cachectl.h>

#define MMC_DBG(fmt, ...)

#if MMC_UCOSII_EN
#include "ucos_ii.h"
static OS_EVENT *mmc_dma_rtx_sem;
static OS_EVENT *mmc_msc_irq_sem;
#endif

#if MMC_DMA_ENABLE
#define MMC_DMA_CHANNEL 0
#define MMC_LOW_LEVEL_DMA  0

#if !MMC_LOW_LEVEL_DMA
#include "dma.h"
#include "cache.h"
#endif
#endif

#define PHYSADDR(x) ((x) & 0x1fffffff)

/***************************************************************************
 * Define card detect and power function etc.
 * This may be modified for a different platform.
 */
#define MMC_CD_PIN      (29 + 3*32)     /* CS2_1 */
//#define MMC_POWER_PIN (17 + 3 * 32)	/* Pin to enable/disable card power */
//#define MMC_PW_PIN    (14 + 3 * 32)	/* Pin to check protect card */


void inline MMC_INIT_GPIO()
{
	__gpio_as_msc(); 
#ifdef MMC_POWER_PIN
	__gpio_as_output(MMC_POWER_PIN);
	__gpio_disable_pull(MMC_POWER_PIN);
	__gpio_set_pin(MMC_POWER_PIN);
#endif
#ifdef MMC_CD_PIN
	__gpio_as_input(MMC_CD_PIN);
	__gpio_enable_pull(MMC_CD_PIN);
#endif
#ifdef MMC_PW_PIN
	__gpio_as_input(MMC_PW_PIN);
	__gpio_disable_pull(MMC_PW_PIN);
#endif
}


#define MMC_POWER_OFF()						\
do {										\
      	__gpio_set_pin(MMC_POWER_PIN);		\
} while (0)

#define MMC_POWER_ON()						\
do {										\
      	__gpio_clear_pin(MMC_POWER_PIN);	\
} while (0)

#define MMC_INSERT_STATUS() __gpio_get_pin(MMC_CD_PIN)

#define MMC_RESET() __msc_reset()

#define MMC_IRQ_MASK()						\
do {										\
      	REG_MSC_IMASK = 0xffff;				\
      	REG_MSC_IREG = 0xffff;				\
} while (0)

#define MMC_LOW_POWER()   REG_MSC_LPM = MSC_LPM_ON

/***********************************************************************
 *  MMC Events
 */
#define MMC_EVENT_NONE	        0x00	/* No events */
#define MMC_EVENT_RX_DATA_DONE	0x01	/* Rx data done */
#define MMC_EVENT_TX_DATA_DONE	0x02	/* Tx data done */
#define MMC_EVENT_PROG_DONE	0x04	/* Programming is done */

static int use_4bit;		/* Use 4-bit data bus */
extern int num_6;
extern int sd2_0;

/* Stop the MMC clock and wait while it happens */
static inline int _mmc_stop_clock(void)
{
	int timeout = 1000;

	MMC_DBG("stop MMC clock\n");
	REG_MSC_STRPCL = MSC_STRPCL_CLOCK_CONTROL_STOP;

	while (timeout && (REG_MSC_STAT & MSC_STAT_CLK_EN)) {
		timeout--;
		if (timeout == 0) {
			MMC_DBG("Timeout on stop clock waiting\n");
			return MMC_ERROR_TIMEOUT;
		}
		usleep(1);
	}
	MMC_DBG("clock off time is %d microsec\n", timeout);
	return MMC_NO_ERROR;
}

/* Start the MMC clock and operation */
static inline int _mmc_start_clock(void)
{
	REG_MSC_STRPCL =
	    MSC_STRPCL_CLOCK_CONTROL_START;
	return MMC_NO_ERROR;
}

static inline int _mmc_start_op(void)
{
	REG_MSC_STRPCL =
	    MSC_STRPCL_CLOCK_CONTROL_START | MSC_STRPCL_START_OP;
	return MMC_NO_ERROR;
}

static inline unsigned int _mmc_calc_clkrt(int is_sd, unsigned int rate)
{
	unsigned int clkrt;
	unsigned int clk_src = is_sd ? 24000000 : 20000000;

	clkrt = 0;
	while (rate < clk_src) {
		clkrt++;
		clk_src >>= 1;
	}
	return clkrt;
}

static int _mmc_check_status(struct mmc_request *request)
{
	unsigned int status = REG_MSC_STAT;

	/* Checking for response or data timeout */
	if (status & (MSC_STAT_TIME_OUT_RES | MSC_STAT_TIME_OUT_READ)) {
		MMC_DBG("MMC/SD timeout, MMC_STAT 0x%x CMD %d\n", status,
		       request->cmd);
		return MMC_ERROR_TIMEOUT;
	}

	/* Checking for CRC error */
	if (status &
	    (MSC_STAT_CRC_READ_ERROR | MSC_STAT_CRC_WRITE_ERROR |
	     MSC_STAT_CRC_RES_ERR)) {
		MMC_DBG("MMC/CD CRC error, MMC_STAT 0x%x\n", status);
		return MMC_ERROR_CRC;
	}

	return MMC_NO_ERROR;
}

/* Obtain response to the command and store it to response buffer */
static void _mmc_get_response(struct mmc_request *request)
{
	int i;
	unsigned char *buf;
	unsigned int data;

	MMC_DBG("fetch response for request %d, cmd %d\n", request->rtype,
	      request->cmd);
	buf = request->response;
	request->result = MMC_NO_ERROR;

	switch (request->rtype) {
	case RESPONSE_R1:
	case RESPONSE_R1B:
	case RESPONSE_R6:
	case RESPONSE_R3:
	case RESPONSE_R4:
	case RESPONSE_R5:
		{
			data = REG_MSC_RES;
			buf[0] = (data >> 8) & 0xff;
			buf[1] = data & 0xff;
			data = REG_MSC_RES;
			buf[2] = (data >> 8) & 0xff;
			buf[3] = data & 0xff;
			data = REG_MSC_RES;
			buf[4] = data & 0xff;

			MMC_DBG("request %d, response [%02x %02x %02x %02x %02x]\n",
			      request->rtype, buf[0], buf[1], buf[2],
			      buf[3], buf[4]);
			break;
		}
	case RESPONSE_R2_CID:
	case RESPONSE_R2_CSD:
		{
			for (i = 0; i < 16; i += 2) {
				data = REG_MSC_RES;
				buf[i] = (data >> 8) & 0xff;
				buf[i + 1] = data & 0xff;
			}
			MMC_DBG("request %d, response [", request->rtype);
#if CONFIG_MMC_DEBUG_VERBOSE > 2
			if (g_mmc_debug >= 3) {
				int n;
				for (n = 0; n < 17; n++)
					MMC_DBG("%02x ", buf[n]);
				MMC_DBG("]\n");
			}
#endif
			break;
		}
	case RESPONSE_NONE:
		MMC_DBG("No response\n");
		break;

	default:
		MMC_DBG("unhandled response type for request %d\n",
		      request->rtype);
		break;
	}
}


#if (MMC_DMA_ENABLE && MMC_UCOSII_EN)
void _mmc_rtx_handler(unsigned int arg)
{
	if (__dmac_channel_address_error_detected(arg)) {
		MMC_DBG("%s: DMAC address error.\n", __FUNCTION__);
		__dmac_channel_clear_address_error(arg);
	}
	if (__dmac_channel_transmit_end_detected(arg)) {
        dma_stop(arg);
		OSSemPost(mmc_dma_rtx_sem);
	}
}
#endif


#if MMC_DMA_ENABLE
static int _mmc_receive_data_dma(struct mmc_request *req)
{
	int ch;
	unsigned int size = req->block_len * req->nob;

#if MMC_LOW_LEVEL_DMA
    REG_DMAC_DCKE = DMAC_DCKE_CH0_ON;
	/* flush dcache */
	dcache_writeback_range(req->buffer, size);
	/* setup dma channel */
	REG_DMAC_DSAR(ch) = PHYSADDR(MSC_RXFIFO);	/* DMA source addr */
	REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long) req->buffer);	/* DMA dest addr */
	REG_DMAC_DTCR(ch) = (size + 3) / 4;	/* DMA transfer count */
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_MSCIN;	/* DMA request type */
	REG_DMAC_DCMD(ch) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
	    DMAC_DCMD_DS_32BIT;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;

    while (!__dmac_channel_transmit_end_detected(ch));
    /* clear status and disable channel */
	REG_DMAC_DCCSR(ch) = 0;

    return 0;

#else
    dcache_writeback_range(req->buffer, size);
#if MMC_UCOSII_EN
    dma_request(ch, _mmc_rtx_handler, ch,
        DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT,
        DMAC_DRSR_RS_MSCIN);
    dma_start(ch, MSC_RXFIFO, (unsigned int)req->buffer, size);

    unsigned char err = 0;

    OSSemPend(mmc_dma_rtx_sem, 0, &err);
    if(!err)    return MMC_NO_ERROR;
    else        return MMC_NO_RESPONSE;
#else
    ch = dma_request(NULL, 0, DMAC_DRSR_RS_MSCIN,
        DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT);

    dma_start(ch, (void*) MSC_RXFIFO, req->buffer, size);
    dma_join(ch);
    dma_free(ch);

    return MMC_NO_ERROR;
#endif    
#endif
}

static int _mmc_transmit_data_dma(struct mmc_request *req)
{
	int ch;
	unsigned int size = req->block_len * req->nob;
	
#if MMC_LOW_LEVEL_DMA
    REG_DMAC_DCKE = DMAC_DCKE_CH0_ON;
	/* flush dcache */
	dcache_writeback_range(req->buffer, size);
	/* setup dma channel */
	REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long) req->buffer);	/* DMA source addr */
	REG_DMAC_DTAR(ch) = PHYSADDR(MSC_TXFIFO);	/* DMA dest addr */
	REG_DMAC_DTCR(ch) = (size + 3) / 4;	/* DMA transfer count */
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_MSCOUT;	/* DMA request type */

	REG_DMAC_DCMD(ch) = DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
	    DMAC_DCMD_DS_32BIT;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
	/* wait for dma completion */

	while (!__dmac_channel_transmit_end_detected(ch));
	/* clear status and disable channel */
	REG_DMAC_DCCSR(ch) = 0;

    return 0;
#else
    dcache_writeback_range(req->buffer, size);
 #if MMC_UCOSII_EN
    dma_request(ch, _mmc_rtx_handler, ch,
        DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT,
        DMAC_DRSR_RS_MSCOUT);
    dma_start(ch, (unsigned int)req->buffer, MSC_TXFIFO, size);

    unsigned char err = 0;

    OSSemPend(mmc_dma_rtx_sem, 0, &err);
    if(!err)    return MMC_NO_ERROR;
    else        return MMC_NO_RESPONSE;
 #else
    ch = dma_request(NULL, 0, DMAC_DRSR_RS_MSCOUT,
        DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT);

    dma_start(ch, req->buffer, (void*) MSC_TXFIFO, size);
    dma_join(ch);
    dma_free(ch);

    return MMC_NO_ERROR;
 #endif    
#endif
}
#endif/* MMC_DMA_ENABLE */

static int _mmc_receive_data(struct mmc_request *req)
{
	unsigned int nob = req->nob;
	unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;	/* length in word */
	unsigned char *buf = req->buffer;
	unsigned int *wbuf = (unsigned int *) buf;
	unsigned int waligned = (((unsigned int) buf & 0x3) == 0);	/* word aligned ? */
	unsigned int stat, timeout, data, cnt;

	for (; nob >= 1; nob--) {
		timeout = 0x3FFFFFF;

		while (timeout) {
			timeout--;
			stat = REG_MSC_STAT;

			if (stat & MSC_STAT_TIME_OUT_READ)
				return MMC_ERROR_TIMEOUT;
			else if (stat & MSC_STAT_CRC_READ_ERROR)
				return MMC_ERROR_CRC;
			else if (!(stat & MSC_STAT_DATA_FIFO_EMPTY)
				 || (stat & MSC_STAT_DATA_FIFO_AFULL)) {
				/* Ready to read data */
				break;
			}
			usleep(1);
		}

		if (!timeout)
			return MMC_ERROR_TIMEOUT;

		/* Read data from RXFIFO. It could be FULL or PARTIAL FULL */
		cnt = wblocklen;
		while (cnt) {
			while (REG_MSC_STAT & MSC_STAT_DATA_FIFO_EMPTY);
		    data = REG_MSC_RXFIFO;

			if (waligned) {
				*wbuf++ = data;
			} else {
				*buf++ = (unsigned char) (data >> 0);
				*buf++ = (unsigned char) (data >> 8);
				*buf++ = (unsigned char) (data >> 16);
				*buf++ = (unsigned char) (data >> 24);
			}
			cnt--;
			while (cnt
			       && (REG_MSC_STAT &
				   MSC_STAT_DATA_FIFO_EMPTY));
		}
	}

	return MMC_NO_ERROR;
}

static int _mmc_transmit_data(struct mmc_request *req)
{
	unsigned int nob = req->nob;
	unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;	/* length in word */
	unsigned char *buf = req->buffer;
	unsigned int *wbuf = (unsigned int *)buf;
	unsigned int waligned = (((unsigned int) buf & 0x3) == 0);	/* word aligned ? */
	unsigned int stat, timeout, data, cnt;

	for (; nob > 0; nob--) {
		timeout = 0x3FFFFFF;

		while (timeout) {
			timeout--;
			stat = REG_MSC_STAT;

			if (stat &
			    (MSC_STAT_CRC_WRITE_ERROR |
			     MSC_STAT_CRC_WRITE_ERROR_NOSTS))
				return MMC_ERROR_CRC;
			else if (!(stat & MSC_STAT_DATA_FIFO_FULL)) {
				/* Ready to write data */
				break;
			}

			usleep(1);
		}

		if (!timeout)
			return MMC_ERROR_TIMEOUT;

		/* Write data to TXFIFO */
		cnt = wblocklen;
		while (cnt) {
			while (REG_MSC_STAT & MSC_STAT_DATA_FIFO_FULL);

			if (waligned) {
				REG_MSC_TXFIFO = *wbuf++;
			} else {
				data = *buf++;
                data |= *buf++ << 8;
                data |= *buf++ << 16;
                data |= *buf++ << 24;
				REG_MSC_TXFIFO = data;
			}
			cnt--;
		}
	}

	return MMC_NO_ERROR;
}

/********************************************************************************************************************
** Name:	  int _mmc_exec_cmd()
** Function:      send command to the card, and get a response
** Input:	  struct mmc_request *req	: MMC/SD request
** Output:	  0:  right		>0:  error code
********************************************************************************************************************/
int _mmc_exec_cmd(struct mmc_request *request)
{
	unsigned int cmdat = 0, events = 0;
	int retval, timeout = 0x3fffff;

    MMC_DBG("MMC: cmd %d\n", request->cmd);

	/* Indicate we have no result yet */
	request->result = MMC_NO_RESPONSE;

	if (request->cmd == MMC_CIM_RESET) {//The 1st cmd
		/* On reset, 1-bit bus width */
		use_4bit = 0;

		/* Reset MMC/SD controller */
        MMC_RESET();		    /* reset mmc/sd controller */
	    MMC_IRQ_MASK();		    /* mask all IRQs */
        MMC_LOW_POWER();        /* Set low power mode */
	    _mmc_stop_clock();	/* stop MMC/SD clock */

		/* On reset, drop MMC clock down */
		_mmc_set_clock(0, MMC_CLOCK_SLOW);
		/* On reset over, start MMC clock */
        _mmc_start_clock();
	}
	if (request->cmd == MMC_SEND_OP_COND) {
		DEBUG(3, "Have an MMC card\n");
		/* always use 1bit for MMC */
		use_4bit = 0;
	}
	if (request->cmd == SET_BUS_WIDTH) {
		if (request->arg == 0x2) {
			DEBUG(2, "Use 4-bit bus width\n");
			use_4bit = 1;
		} else {
			DEBUG(2, "Use 1-bit bus width\n");
			use_4bit = 0;
		}
	}

	/* stop clock */
//	_mmc_stop_clock();

	/* mask all interrupts */
	REG_MSC_IMASK = 0xffff;
	/* clear status */
	REG_MSC_IREG = 0xffff;

	/*open interrupt */
	REG_MSC_IMASK = (~7);   //END_CMD_RES, PROG_DONE, DATA_TRANS_DONE

	/* use 4-bit bus width when possible */
	if (use_4bit)
		cmdat |= MSC_CMDAT_BUS_WIDTH_4BIT;

	/* Set command type and events */
	switch (request->cmd) {
		/* MMC core extra command */
	case MMC_CIM_RESET:
		cmdat |= MSC_CMDAT_INIT;	/* Initialization sequence clock sent prior to command */
		break;

		/* bc - broadcast - no response */
	case MMC_GO_IDLE_STATE:
	case MMC_SET_DSR:
		break;

		/* bcr - broadcast with response */
	case MMC_SEND_OP_COND:
	case MMC_ALL_SEND_CID:
	case MMC_GO_IRQ_STATE:
		break;

		/* adtc - addressed with data transfer */
	case MMC_READ_DAT_UNTIL_STOP:
	case MMC_READ_SINGLE_BLOCK:
	case MMC_READ_MULTIPLE_BLOCK:
	case SEND_SCR:
#if MMC_DMA_ENABLE
		cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ | MSC_CMDAT_DMA_EN;
#else
		cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ;
#endif
		events = MMC_EVENT_RX_DATA_DONE;
		break;

	case 6:
		if (num_6 < 2) {
#if MMC_DMA_ENABLE
			cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ | MSC_CMDAT_DMA_EN;
#else
			cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ;
#endif
			events = MMC_EVENT_RX_DATA_DONE;
		}
		break;

	case MMC_WRITE_DAT_UNTIL_STOP:
	case MMC_WRITE_BLOCK:
	case MMC_WRITE_MULTIPLE_BLOCK:
	case MMC_PROGRAM_CID:
	case MMC_PROGRAM_CSD:
	case MMC_SEND_WRITE_PROT:
	case MMC_GEN_CMD:
	case MMC_LOCK_UNLOCK:
#if MMC_DMA_ENABLE
		cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_WRITE | MSC_CMDAT_DMA_EN;
#else
		cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_WRITE;
#endif
		events = MMC_EVENT_TX_DATA_DONE | MMC_EVENT_PROG_DONE;

		break;

	case MMC_STOP_TRANSMISSION:
		events = MMC_EVENT_PROG_DONE;
		break;

		/* ac - no data transfer */
	default:
		break;
	}

	/* Set response type */
	switch (request->rtype) {
	case RESPONSE_NONE:
		break;

	case RESPONSE_R1B:
		cmdat |= MSC_CMDAT_BUSY;
	 /*FALLTHRU*/
    case RESPONSE_R1:
		cmdat |= MSC_CMDAT_RESPONSE_R1;
		break;
	case RESPONSE_R2_CID:
	case RESPONSE_R2_CSD:
		cmdat |= MSC_CMDAT_RESPONSE_R2;
		break;
	case RESPONSE_R3:
		cmdat |= MSC_CMDAT_RESPONSE_R3;
		break;
	case RESPONSE_R4:
		cmdat |= MSC_CMDAT_RESPONSE_R4;
		break;
	case RESPONSE_R5:
		cmdat |= MSC_CMDAT_RESPONSE_R5;
		break;
	case RESPONSE_R6:
		cmdat |= MSC_CMDAT_RESPONSE_R6;
		break;
	default:
		break;
	}

	/* Set command index */
	if (request->cmd == MMC_CIM_RESET) {
		REG_MSC_CMD = MMC_GO_IDLE_STATE;
	} else {
		REG_MSC_CMD = request->cmd;
	}

	/* Set argument */
	REG_MSC_ARG = request->arg;

	/* Set block length and nob */
	if (request->cmd == SEND_SCR) {	/* get SCR from DataFIFO */
		REG_MSC_BLKLEN = 8;
		REG_MSC_NOB = 1;
	} else {
		REG_MSC_BLKLEN = request->block_len;
		REG_MSC_NOB = request->nob;
	}

	/* Set command */
	REG_MSC_CMDAT = cmdat;

	DEBUG(1, "Send cmd %d cmdat: %x arg: %x resp %d\n", request->cmd,
	      cmdat, request->arg, request->rtype);
	/* Start MMC/SD clock and send command to card */
//	_mmc_start_clock();
    _mmc_start_op();

	/* Wait for command completion */
#if MMC_UCOSII_EN
	__intc_unmask_irq(IRQ_MSC);
	OSSemPend(mmc_msc_irq_sem, 100, &err);
    if(OS_TIMEOUT == err)
        return MMC_ERROR_TIMEOUT;
#else
	while (timeout-- && !(REG_MSC_STAT & MSC_STAT_END_CMD_RES));
	if (timeout == 0)
        return MMC_ERROR_TIMEOUT;
#endif

	REG_MSC_IREG = MSC_IREG_END_CMD_RES;	/* clear flag */

	/* Check for status */
	retval = _mmc_check_status(request);
	if (retval) {
		return retval;
	}

	/* Complete command with no response */
	if (request->rtype == RESPONSE_NONE) {
		return MMC_NO_ERROR;
	}

	/* Get response */
	_mmc_get_response(request);

	/* Start data operation */
	if (events & (MMC_EVENT_RX_DATA_DONE | MMC_EVENT_TX_DATA_DONE)) {
		if (events & MMC_EVENT_RX_DATA_DONE) {
			if (request->cmd == SEND_SCR) {
				/* SD card returns SCR register as data. 
				   MMC core expect it in the response buffer, 
				   after normal response. */
				request->buffer =
				    (unsigned char *) ((unsigned int) request->response + 5);
			}
#if MMC_DMA_ENABLE
            if((unsigned int) request -> buffer & 0x3)
                _mmc_receive_data(request);
            else
    			_mmc_receive_data_dma(request);
#else
			_mmc_receive_data(request);
#endif
		}

		if (events & MMC_EVENT_TX_DATA_DONE) {
#if MMC_DMA_ENABLE
            if((unsigned int) request -> buffer & 0x3)
    			_mmc_transmit_data(request);
            else
    			_mmc_transmit_data_dma(request);
#else
			_mmc_transmit_data(request);
#endif
		}

#if MMC_UCOSII_EN
		__intc_unmask_irq(IRQ_MSC);
		OSSemPend(mmc_msc_irq_sem, 100, &err);
#else
		/* Wait for Data Done */
		while (!(REG_MSC_IREG & MSC_IREG_DATA_TRAN_DONE));
#endif
		REG_MSC_IREG = MSC_IREG_DATA_TRAN_DONE;	/* clear status */
	}

	/* Wait for Prog Done event */
	if (events & MMC_EVENT_PROG_DONE) {
#if MMC_UCOSII_EN
		__intc_unmask_irq(IRQ_MSC);
		OSSemPend(mmc_msc_irq_sem, 100, &err);
#else
        /* Wait for program done */
		while (!(REG_MSC_IREG & MSC_IREG_PRG_DONE));
#endif
		REG_MSC_IREG = MSC_IREG_PRG_DONE;	/* clear status */
	}

	/* Command completed */
	return MMC_NO_ERROR;	/* return successfully */
}

/*******************************************************************************************************************
** Name:	  int mmc_chkcardwp()
** Function:      check weather card is write protect
** Input:	  NULL
** Output:	  1: insert write protect	0: not write protect
********************************************************************************************************************/
int _mmc_chkcardwp(void)
{
	return 0;
}

/*******************************************************************************************************************
** Name:	  int mmc_chkcard()
** Function:      check weather card is insert entirely
** Input:	  NULL
** Output:	  1: insert entirely	0: not insert entirely
********************************************************************************************************************/
int _mmc_chkcard(void)
{
#if 0
	return 1;           /* assume always inserted */
#else
	if (MMC_INSERT_STATUS() == 0) {
		if (MMC_EXSIT == 0)
			return 1;	/* insert entirely */
		else
			return 0;	/* not microsd insert entirely */
	}
	else {
		if (MMC_EXSIT == 0)
			return 0;	/* not insert entirely */
		else
			return 1;	/* microsd insert entirely */
	}
#endif
}

/* Set the MMC clock frequency */
void _mmc_set_clock(int sd, unsigned int rate)
{
	int clkrt= 0;

	sd = sd ? 1 : 0;

	_mmc_stop_clock();

	if (sd2_0) {
        if(rate >= 48000000)
    		__cpm_select_msc_hs_clk(sd);	/* select clock source from CPM */
        else
    		__cpm_select_msc_clk(sd);	/* select clock source from CPM */
		REG_CPM_CPCCR |= CPM_CPCCR_CE;
		REG_MSC_CLKRT = 0;
	} else {
		__cpm_select_msc_clk(sd);	/* select clock source from CPM */
		REG_CPM_CPCCR |= CPM_CPCCR_CE;
		clkrt = _mmc_calc_clkrt(sd, rate);
		REG_MSC_CLKRT = clkrt;
	}

//	printf("MMC: clock= %u Hz is_sd=%d clkrt=%d\n", rate, sd, clkrt);
//	printf("MMC: clock= %u Hz is_sd=%d\n", rate, sd);
}

void _mmc_irq_handler(unsigned int arg)
{
	__intc_mask_irq(IRQ_MSC);
#if MMC_UCOSII_EN
	OSSemPost(mmc_msc_irq_sem);
#endif
}


/*******************************************************************************************************************
** Name:	  void mmc_hardware_init()
** Function:      initialize the hardware condiction that access sd card
** Input:	  NULL
** Output:	  NULL
********************************************************************************************************************/
static int _mmc_hardware_inited = 0;
int _mmc_hardware_init(void)
{
	if(_mmc_hardware_inited) return MMC_NO_ERROR;
    _mmc_hardware_inited = 1;
    MMC_INIT_GPIO();	    /* init GPIO */
//    __cpm_start_msc();      //MSC0 clock enabled orignal, MSC1 need to enable 
#ifdef MMC_POWER_PIN
	MMC_POWER_ON();		    /* turn on power of card */
#endif
	MMC_RESET();		    /* reset mmc/sd controller */
	MMC_IRQ_MASK();		    /* mask all IRQs */
//    MMC_LOW_POWER();        /* Set low power mode */
	_mmc_stop_clock();	/* stop MMC/SD clock */

#if MMC_UCOSII_EN
	mmc_msc_irq_sem = OSSemCreate(0);
    if(mmc_msc_irq_sem == NULL)
    {
        MMC_DBG("MMC: create mmc_msc_irq_sem error\n");
        return MMC_NO_RESPONSE;
    }

	request_irq(IRQ_MSC, _mmc_irq_handler, 0);
#endif

#if MMC_DMA_ENABLE
#if MMC_LOW_LEVEL_DMA
    __cpm_start_dmac();
    __dmac_enable_module();
#elif MMC_UCOSII_EN
	mmc_dma_rtx_sem = OSSemCreate(0);
    if(mmc_dma_rtx_sem == NULL)
    {
        MMC_DBG("MMC: mmc_dma_rtx_sem error\n");
        return MMC_NO_RESPONSE;
    }
#endif
#endif

    return MMC_NO_ERROR;
}

