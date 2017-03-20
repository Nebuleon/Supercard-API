/*
 fatdir.c

 Functions used by the newlib disc stubs to interface with
 this library

 Copyright (c) 2006 Michael "Chishm" Chisholm

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include "fatdir.h"

#include "cache.h"
#include "file_allocation_table.h"
#include "partition.h"
#include "directory.h"
#include "bit_ops.h"
#include "filetime.h"
#include "lock.h"


int _FAT_stat (const char* restrict path, struct stat* restrict st) {
	DIR_ENTRY dirEntry;

	// Get the partition this file is on
	if (single_partition == NULL) {
		errno = ENODEV;
		return -1;
	}

	// Move the path pointer to the start of the actual path
	if (strchr (path, ':') != NULL) {
		path = strchr (path, ':') + 1;
	}
	if (strchr (path, ':') != NULL) {
		errno = EINVAL;
		return -1;
	}

	_FAT_lock(&single_partition->lock);

	// Search for the file on the disc
	if (!_FAT_directory_entryFromPath (single_partition, &dirEntry, path, NULL)) {
		_FAT_unlock(&single_partition->lock);
		errno = ENOENT;
		return -1;
	}

	// Fill in the stat struct
	_FAT_directory_entryStat (single_partition, &dirEntry, st);

	_FAT_unlock(&single_partition->lock);
	return 0;
}

int _FAT_link (const char *existing, const char *newLink) {
	errno = ENOTSUP;
	return -1;
}

int _FAT_unlink (const char *path) {
	DIR_ENTRY dirEntry;
	DIR_ENTRY dirContents;
	uint32_t cluster;
	bool nextEntry;
	bool errorOccured = false;

	// Get the partition this directory is on
	if (single_partition == NULL) {
		errno = ENODEV;
		return -1;
	}

	// Make sure we aren't trying to write to a read-only disc
	if (single_partition->readOnly) {
		errno = EROFS;
		return -1;
	}

	// Move the path pointer to the start of the actual path
	if (strchr (path, ':') != NULL) {
		path = strchr (path, ':') + 1;
	}
	if (strchr (path, ':') != NULL) {
		errno = EINVAL;
		return -1;
	}

	_FAT_lock(&single_partition->lock);

	// Search for the file on the disc
	if (!_FAT_directory_entryFromPath (single_partition, &dirEntry, path, NULL)) {
		_FAT_unlock(&single_partition->lock);
		errno = ENOENT;
		return -1;
	}

	cluster = _FAT_directory_entryGetCluster (single_partition, dirEntry.entryData);


	// If this is a directory, make sure it is empty
	if (_FAT_directory_isDirectory (&dirEntry)) {
		nextEntry = _FAT_directory_getFirstEntry (single_partition, &dirContents, cluster);

		while (nextEntry) {
			if (!_FAT_directory_isDot (&dirContents)) {
				// The directory had something in it that isn't a reference to itself or it's parent
				_FAT_unlock(&single_partition->lock);
				errno = ENOTEMPTY; /* [Neb] previously EPERM */
				return -1;
			}
			nextEntry = _FAT_directory_getNextEntry (single_partition, &dirContents);
		}
	}

	if (_FAT_fat_isValidCluster(single_partition, cluster)) {
		// Remove the cluster chain for this file
		if (!_FAT_fat_clearLinks (single_partition, cluster)) {
			errno = EIO;
			errorOccured = true;
		}
	}

	// Remove the directory entry for this file
	if (!_FAT_directory_removeEntry (single_partition, &dirEntry)) {
		errno = EIO;
		errorOccured = true;
	}

	// Flush any sectors in the disc cache
	if (!_FAT_cache_flush(single_partition->cache)) {
		errno = EIO;
		errorOccured = true;
	}

	_FAT_unlock(&single_partition->lock);
	if (errorOccured) {
		return -1;
	} else {
		return 0;
	}
}

int _FAT_chdir (const char *path) {
	// Get the partition this directory is on
	if (single_partition == NULL) {
		errno = ENODEV;
		return -1;
	}

	// Move the path pointer to the start of the actual path
	if (strchr (path, ':') != NULL) {
		path = strchr (path, ':') + 1;
	}
	if (strchr (path, ':') != NULL) {
		errno = EINVAL;
		return -1;
	}

	_FAT_lock(&single_partition->lock);

	// Try changing directory
	if (_FAT_directory_chdir (single_partition, path)) {
		// Successful
		_FAT_unlock(&single_partition->lock);
		return 0;
	} else {
		// Failed
		_FAT_unlock(&single_partition->lock);
		errno = ENOTDIR;
		return -1;
	}
}

int _FAT_rename (const char *oldName, const char *newName) {
	DIR_ENTRY oldDirEntry;
	DIR_ENTRY newDirEntry;
	const char *pathEnd;
	uint32_t dirCluster;

	// Get the partition this directory is on
	if (single_partition == NULL) {
		errno = ENODEV;
		return -1;
	}

	_FAT_lock(&single_partition->lock);

	// Make sure we aren't trying to write to a read-only disc
	if (single_partition->readOnly) {
		_FAT_unlock(&single_partition->lock);
		errno = EROFS;
		return -1;
	}

	// Move the path pointer to the start of the actual path
	if (strchr (oldName, ':') != NULL) {
		oldName = strchr (oldName, ':') + 1;
	}
	if (strchr (oldName, ':') != NULL) {
		_FAT_unlock(&single_partition->lock);
		errno = EINVAL;
		return -1;
	}
	if (strchr (newName, ':') != NULL) {
		newName = strchr (newName, ':') + 1;
	}
	if (strchr (newName, ':') != NULL) {
		_FAT_unlock(&single_partition->lock);
		errno = EINVAL;
		return -1;
	}

	// Search for the file on the disc
	if (!_FAT_directory_entryFromPath (single_partition, &oldDirEntry, oldName, NULL)) {
		_FAT_unlock(&single_partition->lock);
		errno = ENOENT;
		return -1;
	}

	// Make sure there is no existing file / directory with the new name
	if (_FAT_directory_entryFromPath (single_partition, &newDirEntry, newName, NULL)) {
		_FAT_unlock(&single_partition->lock);
		errno = EEXIST;
		return -1;
	}

	// Create the new file entry
	// Get the directory it has to go in
	pathEnd = strrchr (newName, DIR_SEPARATOR);
	if (pathEnd == NULL) {
		// No path was specified
		dirCluster = single_partition->cwdCluster;
		pathEnd = newName;
	} else {
		// Path was specified -- get the right dirCluster
		// Recycling newDirEntry, since it needs to be recreated anyway
		if (!_FAT_directory_entryFromPath (single_partition, &newDirEntry, newName, pathEnd) ||
			!_FAT_directory_isDirectory(&newDirEntry)) {
			_FAT_unlock(&single_partition->lock);
			errno = ENOTDIR;
			return -1;
		}
		dirCluster = _FAT_directory_entryGetCluster (single_partition, newDirEntry.entryData);
		// Move the pathEnd past the last DIR_SEPARATOR
		pathEnd += 1;
	}

	// Copy the entry data
	memcpy (&newDirEntry, &oldDirEntry, sizeof(DIR_ENTRY));

	// Set the new name
	strncpy (newDirEntry.filename, pathEnd, MAX_FILENAME_LENGTH - 1);

	// Write the new entry
	if (!_FAT_directory_addEntry (single_partition, &newDirEntry, dirCluster)) {
		_FAT_unlock(&single_partition->lock);
		errno = ENOSPC;
		return -1;
	}

	// Remove the old entry
	if (!_FAT_directory_removeEntry (single_partition, &oldDirEntry)) {
		_FAT_unlock(&single_partition->lock);
		errno = EIO;
		return -1;
	}

	// Flush any sectors in the disc cache
	if (!_FAT_cache_flush (single_partition->cache)) {
		_FAT_unlock(&single_partition->lock);
		errno = EIO;
		return -1;
	}

	_FAT_unlock(&single_partition->lock);
	return 0;
}

int _FAT_mkdir (const char *path, int mode) {
	bool fileExists;
	DIR_ENTRY dirEntry;
	const char* pathEnd;
	uint32_t parentCluster, dirCluster;
	uint8_t newEntryData[DIR_ENTRY_DATA_SIZE];

	if (single_partition == NULL) {
		errno = ENODEV;
		return -1;
	}

	// Move the path pointer to the start of the actual path
	if (strchr (path, ':') != NULL) {
		path = strchr (path, ':') + 1;
	}
	if (strchr (path, ':') != NULL) {
		errno = EINVAL;
		return -1;
	}

	_FAT_lock(&single_partition->lock);

	// Search for the file/directory on the disc
	fileExists = _FAT_directory_entryFromPath (single_partition, &dirEntry, path, NULL);

	// Make sure it doesn't exist
	if (fileExists) {
		_FAT_unlock(&single_partition->lock);
		errno = EEXIST;
		return -1;
	}

	if (single_partition->readOnly) {
		// We can't write to a read-only partition
		_FAT_unlock(&single_partition->lock);
		errno = EROFS;
		return -1;
	}

	// Get the directory it has to go in
	pathEnd = strrchr (path, DIR_SEPARATOR);
	if (pathEnd == NULL) {
		// No path was specified
		parentCluster = single_partition->cwdCluster;
		pathEnd = path;
	} else {
		// Path was specified -- get the right parentCluster
		// Recycling dirEntry, since it needs to be recreated anyway
		if (!_FAT_directory_entryFromPath (single_partition, &dirEntry, path, pathEnd) ||
			!_FAT_directory_isDirectory(&dirEntry)) {
			_FAT_unlock(&single_partition->lock);
			errno = ENOTDIR;
			return -1;
		}
		parentCluster = _FAT_directory_entryGetCluster (single_partition, dirEntry.entryData);
		// Move the pathEnd past the last DIR_SEPARATOR
		pathEnd += 1;
	}
	// Create the entry data
	strncpy (dirEntry.filename, pathEnd, MAX_FILENAME_LENGTH - 1);
	memset (dirEntry.entryData, 0, DIR_ENTRY_DATA_SIZE);

	// Set the creation time and date
	dirEntry.entryData[DIR_ENTRY_cTime_ms] = 0;
	u16_to_u8array (dirEntry.entryData, DIR_ENTRY_cTime, _FAT_filetime_getTimeFromRTC());
	u16_to_u8array (dirEntry.entryData, DIR_ENTRY_cDate, _FAT_filetime_getDateFromRTC());
	u16_to_u8array (dirEntry.entryData, DIR_ENTRY_mTime, _FAT_filetime_getTimeFromRTC());
	u16_to_u8array (dirEntry.entryData, DIR_ENTRY_mDate, _FAT_filetime_getDateFromRTC());
	u16_to_u8array (dirEntry.entryData, DIR_ENTRY_aDate, _FAT_filetime_getDateFromRTC());

	// Set the directory attribute
	dirEntry.entryData[DIR_ENTRY_attributes] = ATTRIB_DIR;

	// Get a cluster for the new directory
	dirCluster = _FAT_fat_linkFreeClusterCleared (single_partition, CLUSTER_FREE);
	if (!_FAT_fat_isValidCluster(single_partition, dirCluster)) {
		// No space left on disc for the cluster
		_FAT_unlock(&single_partition->lock);
		errno = ENOSPC;
		return -1;
	}
	u16_to_u8array (dirEntry.entryData, DIR_ENTRY_cluster, dirCluster);
	u16_to_u8array (dirEntry.entryData, DIR_ENTRY_clusterHigh, dirCluster >> 16);

	// Write the new directory's entry to it's parent
	if (!_FAT_directory_addEntry (single_partition, &dirEntry, parentCluster)) {
		_FAT_unlock(&single_partition->lock);
		errno = ENOSPC;
		return -1;
	}

	// Create the dot entry within the directory
	memset (newEntryData, 0, DIR_ENTRY_DATA_SIZE);
	memset (newEntryData, ' ', 11);
	newEntryData[DIR_ENTRY_name] = '.';
	newEntryData[DIR_ENTRY_attributes] = ATTRIB_DIR;
	u16_to_u8array (newEntryData, DIR_ENTRY_cluster, dirCluster);
	u16_to_u8array (newEntryData, DIR_ENTRY_clusterHigh, dirCluster >> 16);

	// Write it to the directory, erasing that sector in the process
	_FAT_cache_eraseWritePartialSector ( single_partition->cache, newEntryData,
		_FAT_fat_clusterToSector (single_partition, dirCluster), 0, DIR_ENTRY_DATA_SIZE);


	// Create the double dot entry within the directory

	// if ParentDir == Rootdir then ".."" always link to Cluster 0
	if(parentCluster == single_partition->rootDirCluster)
		parentCluster = FAT16_ROOT_DIR_CLUSTER;

	newEntryData[DIR_ENTRY_name + 1] = '.';
	u16_to_u8array (newEntryData, DIR_ENTRY_cluster, parentCluster);
	u16_to_u8array (newEntryData, DIR_ENTRY_clusterHigh, parentCluster >> 16);

	// Write it to the directory
	_FAT_cache_writePartialSector ( single_partition->cache, newEntryData,
		_FAT_fat_clusterToSector (single_partition, dirCluster), DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);

	// Flush any sectors in the disc cache
	if (!_FAT_cache_flush(single_partition->cache)) {
		_FAT_unlock(&single_partition->lock);
		errno = EIO;
		return -1;
	}

	_FAT_unlock(&single_partition->lock);
	return 0;
}

int _FAT_statvfs (const char* restrict path, struct statvfs* restrict buf)
{
	unsigned int freeClusterCount;

	// Get the partition of the requested path
	if (single_partition == NULL) {
		errno = ENODEV;
		return -1;
	}

	_FAT_lock(&single_partition->lock);

	if(memcmp(&buf->f_flag, "SCAN", 4) == 0)
	{
		//Special command was given to sync the numberFreeCluster
		_FAT_partition_createFSinfo(single_partition);
	}

	if(single_partition->filesysType == FS_FAT32)
		freeClusterCount = single_partition->fat.numberFreeCluster;
	else
		freeClusterCount = _FAT_fat_freeClusterCount (single_partition);

	// FAT clusters = POSIX blocks
	buf->f_bsize = single_partition->bytesPerCluster;		// File system block size.
	buf->f_frsize = single_partition->bytesPerCluster;	// Fundamental file system block size.

	buf->f_blocks	= single_partition->fat.lastCluster - CLUSTER_FIRST + 1; // Total number of blocks on file system in units of f_frsize.
	buf->f_bfree = freeClusterCount;	// Total number of free blocks.
	buf->f_bavail	= freeClusterCount;	// Number of free blocks available to non-privileged process.

	// Treat requests for info on inodes as clusters
	buf->f_files = single_partition->fat.lastCluster - CLUSTER_FIRST + 1;	// Total number of file serial numbers.
	buf->f_ffree = freeClusterCount;	// Total number of free file serial numbers.
	buf->f_favail = freeClusterCount;	// Number of file serial numbers available to non-privileged process.

	// File system ID. 32bit ioType value
	buf->f_fsid = _FAT_disc_hostType(single_partition->disc);

	// Bit mask of f_flag values.
	buf->f_flag = ST_NOSUID /* No support for ST_ISUID and ST_ISGID file mode bits */
		| (single_partition->readOnly ? ST_RDONLY /* Read only file system */ : 0 ) ;
	// Maximum filename length.
	buf->f_namemax = MAX_FILENAME_LENGTH;

	_FAT_unlock(&single_partition->lock);
	return 0;
}

DIR_ITER* _FAT_diropen (DIR_ITER *dirState, const char *path) {
	DIR_ENTRY dirEntry;
	DIR_STATE_STRUCT* state = (DIR_STATE_STRUCT*) (dirState->dirStruct);
	bool fileExists;

	state->partition = single_partition;
	if (state->partition == NULL) {
		errno = ENODEV;
		return NULL;
	}

	// Move the path pointer to the start of the actual path
	if (strchr (path, ':') != NULL) {
		path = strchr (path, ':') + 1;
	}
	if (strchr (path, ':') != NULL) {
		errno = EINVAL;
		return NULL;
	}

	_FAT_lock(&state->partition->lock);

	// Get the start cluster of the directory
	fileExists = _FAT_directory_entryFromPath (state->partition, &dirEntry, path, NULL);

	if (!fileExists) {
		_FAT_unlock(&state->partition->lock);
		errno = ENOENT;
		return NULL;
	}

	// Make sure it is a directory
	if (! _FAT_directory_isDirectory (&dirEntry)) {
		_FAT_unlock(&state->partition->lock);
		errno = ENOTDIR;
		return NULL;
	}

	// Save the start cluster for use when resetting the directory data
	state->startCluster = _FAT_directory_entryGetCluster (state->partition, dirEntry.entryData);

	// Get the first entry for use with a call to dirnext
	state->validEntry =
		_FAT_directory_getFirstEntry (state->partition, &(state->currentEntry), state->startCluster);

	// We are now using this entry
	state->inUse = true;
	_FAT_unlock(&state->partition->lock);
	return (DIR_ITER*) state;
}

int _FAT_dirreset (DIR_ITER *dirState) {
	DIR_STATE_STRUCT* state = (DIR_STATE_STRUCT*) (dirState->dirStruct);

	_FAT_lock(&state->partition->lock);

	// Make sure we are still using this entry
	if (!state->inUse) {
		_FAT_unlock(&state->partition->lock);
		errno = EBADF;
		return -1;
	}

	// Get the first entry for use with a call to dirnext
	state->validEntry =
		_FAT_directory_getFirstEntry (state->partition, &(state->currentEntry), state->startCluster);

	_FAT_unlock(&state->partition->lock);
	return 0;
}

int _FAT_dirnext (DIR_ITER *dirState, char *filename, struct stat *filestat) {
	DIR_STATE_STRUCT* state = (DIR_STATE_STRUCT*) (dirState->dirStruct);

	_FAT_lock(&state->partition->lock);

	// Make sure we are still using this entry
	if (!state->inUse) {
		_FAT_unlock(&state->partition->lock);
		errno = EBADF;
		return -1;
	}

	// Make sure there is another file to report on
	if (! state->validEntry) {
		_FAT_unlock(&state->partition->lock);
		return -1;
	}

	// Get the filename
	strncpy (filename, state->currentEntry.filename, MAX_FILENAME_LENGTH);
	// Get the stats, if requested
	if (filestat != NULL) {
		_FAT_directory_entryStat (state->partition, &(state->currentEntry), filestat);
	}

	// Look for the next entry for use next time
	state->validEntry =
		_FAT_directory_getNextEntry (state->partition, &(state->currentEntry));

	_FAT_unlock(&state->partition->lock);
	return 0;
}

int _FAT_dirclose (DIR_ITER *dirState) {
	DIR_STATE_STRUCT* state = (DIR_STATE_STRUCT*) (dirState->dirStruct);

	// We are no longer using this entry
	_FAT_lock(&state->partition->lock);
	state->inUse = false;
	_FAT_unlock(&state->partition->lock);

	return 0;
}
