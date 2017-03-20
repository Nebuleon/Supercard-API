#ifndef __DS2_DS_MAIN_H__
#define __DS2_DS_MAIN_H__

#include <stdint.h>
#include <unistd.h>

#include "../jz4740.h"

#ifndef BIT
#  define BIT(n) (1UL << (n))
#endif

#define PA_PORT               0
#define PB_PORT               1
#define PC_PORT               2
#define PD_PORT               3
#define PE_PORT               4
#define PF_PORT               5
#define DATA_PORT       PA_PORT
#define INTERRUPT_PORT  PD_PORT

#define SA1     (1<<18)
#define SA2     (1<<19)
#define SA3     (1<<17)
#define SA4     (1<<23)

#define GPIO_ADDR_0    0                //0
#define GPIO_ADDR_1    SA1              //1
#define GPIO_ADDR_2    SA2              //2
#define GPIO_ADDR_3    (SA2 + SA1)      //3
#define GPIO_ADDR_4    SA3              //4
#define GPIO_ADDR_5    (SA3 + SA1)      //5
#define GPIO_ADDR_6    (SA3 + SA2)      //6
#define GPIO_ADDR_7    (SA3 + SA2 + SA1)//7
#define GPIO_ADDR_8    SA4              //8
#define GPIO_ADDR_9    (SA4 + SA1)      //9
#define GPIO_ADDR_10   (SA4 + SA2)      //10
#define GPIO_ADDR_11   (SA4 + SA2 + SA1)//11
#define GPIO_ADDR_12   (SA4 + SA3)      //12
#define GPIO_ADDR_13   (SA4 + SA3 + SA1)//13
#define GPIO_ADDR_14   (SA4 + SA3 + SA2)//14
#define GPIO_ADDR_15   (SA4 + SA3 + SA2 + SA1)//15

#define GPIO_ADDR_GROUP0    0
#define GPIO_ADDR_GROUP1    SA3
#define GPIO_ADDR_GROUP2    SA4
#define GPIO_ADDR_GROUP3    (SA4 + SA3)

#define SET_GPIO_ADDR(n)                \
do {                                    \
    REG_GPIO_PXDATS(2) = n;             \
    REG_GPIO_PXDATC(2) = n ^ (SA4 | SA3 | SA2 | SA1);\
} while(0)

#define SET_ADDR_GROUP(n)               \
do {                                    \
    usleep(1);                          \
    REG_GPIO_PXDATS(2) = n;             \
    REG_GPIO_PXDATC(2) = n ^ (SA4 | SA3);\
} while(0)

#define SET_ADDR_DEFT()                 \
    usleep(1);                          \
    SET_ADDR_GROUP(GPIO_ADDR_GROUP0)

#define GPIO_ADDR_MODE()                \
do {                                    \
    REG_GPIO_PXFUNS(2) = 0x0650ffff;    \
    REG_GPIO_PXSELC(2) = 0x0650ffff;    \
    REG_GPIO_PXPES(2) =  0x0650ffff;    \
    REG_GPIO_PXFUNC(2) = 0x008E0000;    \
    REG_GPIO_PXSELC(2) = 0x008E0000;    \
    REG_GPIO_PXPES(2) =  0x008E0000;    \
    REG_GPIO_PXDIRS(2) = 0x008E0000;    \
} while(0)

#define GPIO_ADDR_RECOVER()             \
do {                                    \
    REG_GPIO_PXFUNS(2) = 0x065Cffff;    \
    REG_GPIO_PXSELC(2) = 0x065Cffff;    \
    REG_GPIO_PXPES(2) =  0x065Cffff;    \
    REG_GPIO_PXFUNC(2) = 0x00820000;    \
    REG_GPIO_PXSELC(2) = 0x00820000;    \
    REG_GPIO_PXPES(2) =  0x00820000;    \
    REG_GPIO_PXDIRS(2) = 0x00820000;    \
    REG_GPIO_PXDATC(2) = GPIO_ADDR_15;  \
} while(0)

// #define GPIO_ADDR_REG32     (*((volatile uint32_t*)CPLD_BASE))
// #define GPIO_ADDR_REG16     (*((volatile uint16_t*)CPLD_BASE))

#define CPLD_FIFO_READ_NDSWCMD_GPIO     GPIO_ADDR_0      //0     --/4
#define CPLD_FIFO_WRITE_NDSRDATA_GPIO   GPIO_ADDR_0      //0     --0
#define CPLD_FIFO_READ_NDSWDATA_GPIO    GPIO_ADDR_3      //3
#define CPLD_CTR_GPIO                   GPIO_ADDR_1      //1
#define CPLD_STATE_GPIO                 GPIO_ADDR_2      //2
#define CPLD_WRITE_ADDR_CMP_GPIO        GPIO_ADDR_5      //5
#define CPLD_SPI_COUNT_GPIO             GPIO_ADDR_7      //7
#define CPLD_SPI_GPIO                   GPIO_ADDR_6      //6

#define CPLD_X0_GPIO                    GPIO_ADDR_8      //8
#define CPLD_X1_GPIO                    GPIO_ADDR_9      //9
#define CPLD_X4_GPIO                    GPIO_ADDR_10     //10
#define CPLD_Y0_GPIO                    GPIO_ADDR_11     //11
#define CPLD_Y1_GPIO                    GPIO_ADDR_12     //12
#define CPLD_Y2_GPIO                    GPIO_ADDR_13     //13
#define CPLD_Y3_GPIO                    GPIO_ADDR_14     //14
#define CPLD_Y4_GPIO                    GPIO_ADDR_15     //15


#define CPLD_BASE (0x14000000 | 0xa0000000)  /* Uses uncached memory access */
#define CPLD_STEP (1<<1)

// 32-bit FIFO to read commands sent by the Nintendo DS to the Supercard.
#define CPLD_FIFO_READ_NDSWCMD_BASE    CPLD_BASE
#define REG_CPLD_FIFO_READ_NDSWCMD    (*(volatile uint16_t*) CPLD_FIFO_READ_NDSWCMD_BASE)

// 32-bit FIFO to write data from the Supercard for the Nintendo DS to read.
#define CPLD_FIFO_WRITE_NDSRDATA_BASE  CPLD_BASE
#define REG_CPLD_FIFO_WRITE_NDSRDATA   (*(volatile uint16_t*) CPLD_FIFO_WRITE_NDSRDATA_BASE)

// 32-bit FIFO to read data written by the Nintendo DS to the Supercard.
// Appears to be unused. Use REG_CPLD_FIFO_READ_NDSWCMD above instead.
#define CPLD_FIFO_READ_NDSWDATA_BASE   (CPLD_BASE + CPLD_STEP * 3)
#define REG_CPLD_FIFO_READ_NDSWDATA    (*(volatile uint16_t*) CPLD_FIFO_READ_NDSWDATA_BASE)

// FPGA control register.
#define CPLD_CTR_BASE       (CPLD_BASE + CPLD_STEP * 1)
#define REG_CPLD_CTR        (*(volatile uint16_t*) CPLD_CTR_BASE)

#define CPLD_CTR_NDS_DIR               (1 << 0)

/* Enabled if BGR 555 input (or the result of setting CPLD_CTR_FIX_VIDEO_RGB_EN)
 * will get bit 15 of each half-word set to 1 in output to the FIFO. This is
 * used so that the Nintendo DS does not consider every pixel to be transparent
 * (bit 15 = 0), but allows a Supercard MIPS program to write 0 or 1 to bit 15
 * in its memory. */
#define CPLD_CTR_FIX_VIDEO_EN          (1 << 1)
#define CPLD_CTR_KEY2_CMD_EN           (1 << 2)
#define CPLD_CTR_KEY2_DATA_EN          (1 << 3)
#define CPLD_CTR_KEY2_INIT_EN          (1 << 4)

/* Enabled, along with CPLD_CTR_FPGA_MODE, to set the card write FIFO index
 * back to 0 for a new series of writes. */
#define CPLD_CTR_FIFO_CLEAR            (1 << 5)

/* Enabled if RGB 555 input will be transformed into BGR 555 output, reversing
 * the order of the red and blue components. */
#define CPLD_CTR_FIX_VIDEO_RGB_EN      (1 << 6)
#define CPLD_CTR_FPGA_MODE             (1 << 10)
#define CPLD_CTR_FIFO_DATA_CPU_WD_CLK  (1 << 8)
#define CPLD_CTR_RESET_FLAG            (1 << 12)

// FPGA FIFO state register.
#define CPLD_FIFO_STATE_BASE  (CPLD_BASE + CPLD_STEP * 2)
#define REG_CPLD_FIFO_STATE   (*(volatile uint16_t*) CPLD_FIFO_STATE_BASE)

#define CPLD_FIFO_STATE_CPU_READ_EMPTY           (1 << 0)
#define CPLD_FIFO_STATE_CPU_WRITE_FULL           (1 << 1)
#define CPLD_FIFO_STATE_CPU_DATAFIFO_READ_EMPTY  (1 << 2)

#define CPLD_FIFO_STATE_NDS_READ_EMPTY           (1 << 3)
#define CPLD_FIFO_STATE_NDS_WRITE_FULL           (1 << 4)
#define CPLD_FIFO_STATE_NDS_DATAFIFO_WRITE_FULL  (1 << 5)

#define CPLD_FIFO_STATE_READ_ERROR               (1 << 6)
#define CPLD_FIFO_STATE_WRITE_ERROR              (1 << 7)
#define CPLD_FIFO_STATE_NDS_CMD_COMPLETE         (1 << 8)
#define CPLD_FIFO_STATE_DATA_BEGIN               (1 << 9)

#define CPLD_FIFO_STATE_SPI_TIME_DISABLE         (1 << 12)
#define CPLD_FIFO_STATE_CPU_RTC_CLK_EN           (1 << 13)
#define CPLD_FIFO_STATE_HT_12M_DISABLE           (1 << 14)
#define CPLD_FIFO_STATE_NDS_IQE_OUT              (1 << 15)

#define __cpld_state_cpu_read_empty() (REG_CPLD_FIFO_STATE & CPLD_FIFO_STATE_CPU_READ_EMPTY)
#define __cpld_state_cpu_write_full() (REG_CPLD_FIFO_STATE & CPLD_FIFO_STATE_CPU_WRITE_FULL)
#define __cpld_state_fifo_read_error() (REG_CPLD_FIFO_STATE & CPLD_FIFO_STATE_READ_ERROR)
#define __cpld_state_fifo_write_error() (REG_CPLD_FIFO_STATE & CPLD_FIFO_STATE_WRITE_ERROR)
#define __cpld_state_nds_cmd_complete() (REG_CPLD_FIFO_STATE & CPLD_FIFO_STATE_NDS_CMD_COMPLETE)
#define __cpld_state_data_begin() (REG_CPLD_FIFO_STATE & CPLD_FIFO_STATE_DATA_BEGIN)

//fifo cmp (data,(write address - read address)) > =1 else =0  write/read
#define CPLD_WRITE_ADDR_CMP_BASE  (CPLD_BASE + CPLD_STEP * 5)
#define REG_CPLD_WRITE_ADDR_CMP   (*(volatile uint16_t*) CPLD_WRITE_ADDR_CMP_BASE)

#define CPLD_SPI_COUNT_BASE       (CPLD_BASE + CPLD_STEP * 4)
#define REG_CPLD_SPI_COUNT        (*(volatile uint16_t*) CPLD_SPI_COUNT_BASE)

//eeprom data write/read
#define CPLD_SPI_BASE             (CPLD_BASE + CPLD_STEP * 5)
#define REG_CPLD_SPI              (*(volatile uint16_t*) CPLD_SPI_BASE)

//read: spi Count ,write X0 data
#define CPLD_X0_BASE        (CPLD_BASE + CPLD_STEP * 6)
#define REG_CPLD_X0         (*(volatile uint16_t*) CPLD_X0_BASE)

#define CPLD_X1_BASE        (CPLD_BASE + CPLD_STEP * 7)
#define REG_CPLD_X1         (*(volatile uint16_t*) CPLD_X1_BASE)
#define CPLD_X2_BASE        (CPLD_BASE + CPLD_STEP * 8)
#define REG_CPLD_X2         (*(volatile uint16_t*) CPLD_X2_BASE)
#define CPLD_X3_BASE        (CPLD_BASE + CPLD_STEP * 9)
#define REG_CPLD_X3         (*(volatile uint16_t*) CPLD_X3_BASE)
#define CPLD_X4_BASE        (CPLD_BASE + CPLD_STEP * 10)
#define REG_CPLD_X4         (*(volatile uint16_t*) CPLD_X4_BASE)

#define CPLD_Y0_BASE        (CPLD_BASE + CPLD_STEP * 11)
#define REG_CPLD_Y0         (*(volatile uint16_t*) CPLD_Y0_BASE)
#define CPLD_Y1_BASE        (CPLD_BASE + CPLD_STEP * 12)
#define REG_CPLD_Y1         (*(volatile uint16_t*) CPLD_Y1_BASE)
#define CPLD_Y2_BASE        (CPLD_BASE + CPLD_STEP * 13)
#define REG_CPLD_Y2         (*(volatile uint16_t*) CPLD_Y2_BASE)
#define CPLD_Y3_BASE        (CPLD_BASE + CPLD_STEP * 14)
#define REG_CPLD_Y3         (*(volatile uint16_t*) CPLD_Y3_BASE)
#define CPLD_Y4_BASE        (CPLD_BASE + CPLD_STEP * 15)
#define REG_CPLD_Y4         (*(volatile uint16_t*) CPLD_Y4_BASE)

#define nds_cmd_iqe_pinx    11
#define fpga_cmd_port   (PD_PORT *32)

#define nds_data_iqe_pinx   12
#define fpga_data_port   (PD_PORT *32)

#define spi_cs_iqe_pinx     7
#define fpga_spics_port   (PD_PORT *32)

#define spi_data_iqe_pinx   6
#define fpga_spidata_port   (PD_PORT *32)

#define spi_clk_iqe_pinx     31
#define fpga_spiclk_port   (PD_PORT *32)

//read PIN Level 
#define	REG_GPIO_PXPINX(m,n)	(REG_GPIO_PXPIN(m) & BIT(n))  /* PIN X Level  */

#define REG_NDS_CMD_IQE   REG_GPIO_PXPINX(fpga_cmd_port/32, nds_cmd_iqe_pinx)
#define REG_NDS_DATA_IQE  REG_GPIO_PXPINX(fpga_data_port/32, nds_data_iqe_pinx)
#define REG_SPI_CS_IQE    REG_GPIO_PXPINX(fpga_spics_port/32, spi_cs_iqe_pinx)
#define REG_SPI_DATA_IQE  REG_GPIO_PXPINX(fpga_spidata_port/32, spi_data_iqe_pinx)

#define       NDS_CMD_IRQ		(IRQ_GPIO_0 + fpga_cmd_port + nds_cmd_iqe_pinx)
#define       NDS_DATA_IRQ		(IRQ_GPIO_0 + fpga_data_port + nds_data_iqe_pinx)

#define RESET_FPGA_FIFO() \
	do { \
		REG_CPLD_CTR = CPLD_CTR_FIFO_CLEAR | CPLD_CTR_FPGA_MODE; \
		usleep(200); \
		REG_CPLD_CTR = CPLD_CTR_FPGA_MODE; \
	} while (0)

#define RESET_FPGA_NDS_IQE() \
	do { \
		REG_CPLD_FIFO_STATE = 0; \
	} while (0)

#define key_port (PD_PORT * 32)
#define key_pinx 0
#define key_pin ( REG_GPIO_PXPINX(PD_PORT,key_pinx))


//{2'b0,cpu_w_len,reset_flag[3:0],{2{1'b0}},nds_iqe_out_reg,fpga_mode,enable_fix_video,enable_fix_video_rgb,cpld_state[9:0]};
// #define nds_fifo_read_full_bit  cpu_write_Full_bit
// #define nds_fifo_read_empty_bit cpu_read_Empty_bit
// #define nds_fifo_datafifo_read_empty cpu_datafifo_read_empty
// #define nds_fifo_read_error_bit  fifo_read_error_bit
// #define nds_fifo_write_error_bit  fifo_write_error_bit
#define nds_fifo_len_bit 19
#define nds_fifo_len_mask 0x3fe
#define nds_fifo_cpu_data_byte 2

#define nds_fifo_cmd_read_state  0xe0
#define nds_fifo_cmd_reset  0xe1
#define nds_fifo_cmd_read  0xe8
#define nds_fifo_cmd_write  0xe9

#define __gpio_as_disable_irq(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXIMS(p) = (1 << o);		\
	REG_GPIO_PXTRGC(p) = (1 << o);		\
	REG_GPIO_PXDIRC(p) = (1 << o);		\
	REG_GPIO_PXFUNC(p) = (1 << o);		\
	REG_GPIO_PXSELC(p) = (1 << o);		\
} while (0)

#define INIT_FPGA_PORT() \
	do { \
		REG_EMC_SMCR2 = (3 << EMC_SMCR_TAW_BIT) | (2 << EMC_SMCR_TBP_BIT) | (2 << EMC_SMCR_TAH_BIT) | (3 << EMC_SMCR_TAS_BIT) | EMC_SMCR_BW_16BIT; \
		REG_EMC_SACR2 = (0x14 << EMC_SACR_BASE_BIT) | (0xFC << EMC_SACR_MASK_BIT); \
	} while (0)

#define SEND_QUEUE_LEN 0xC8000
#define KEY_MASK (0x1f)

#define nds_load_set_cmd 0xc0

#define nds_load_block_cmd 0xc2
#define nds_load_key_cmd 0xc3
#define nds_load_exit_cmd 0xcf

#define flash_base_addr (0x08000000 | 0x80000000)

#endif /* !__DS2_DS_MAIN_H__ */
