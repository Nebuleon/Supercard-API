#include "disc_io.h"
#include "ds2_msc_api.h"

#include <stdint.h>

/* initialize MMC/SD card */
static inline bool _MMC_StartUp(void)
{
	return MMC_Initialize() == MMC_NO_ERROR;
}

/* read multi blocks from MMC/SD card */
/* read a single block from MMC/SD card */
static inline bool _MMC_ReadSectors(uint32_t sector, uint32_t numSectors, void* buffer)
{
	int flag;

	if(numSectors > 1)
		flag= MMC_ReadMultiBlock(sector, numSectors, (unsigned char*)buffer);
	else
		flag= MMC_ReadBlock(sector, (unsigned char*)buffer);
	return (flag==MMC_NO_ERROR);
}

/* write multi blocks from MMC/SD card */
/* write a single block from MMC/SD card */
static inline bool _MMC_WriteSectors(uint32_t sector, uint32_t numSectors, const void* buffer)
{
	int flag;

	if(numSectors > 1)
		flag= MMC_WriteMultiBlock(sector, numSectors, (unsigned char*)buffer);
	else
		flag= MMC_WriteBlock(sector, (unsigned char*)buffer);

	return (flag==MMC_NO_ERROR);
}

static inline bool _MMC_ClearStatus(void)
{
	return true;
}

static inline bool _MMC_ShutDown(void)
{
	return true;
}

static inline bool _MMC_IsInserted(void)
{
	return true;
}

const DISC_INTERFACE __io_scds2 = {
	DEVICE_TYPE_SCDS2_MICROSD,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
	(FN_MEDIUM_STARTUP)&_MMC_StartUp,
	(FN_MEDIUM_ISINSERTED)&_MMC_IsInserted,
	(FN_MEDIUM_READSECTORS)&_MMC_ReadSectors,
	(FN_MEDIUM_WRITESECTORS)&_MMC_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)&_MMC_ClearStatus,
	(FN_MEDIUM_SHUTDOWN)&_MMC_ShutDown
};
