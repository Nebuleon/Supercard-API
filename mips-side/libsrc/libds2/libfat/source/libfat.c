/*
	libfat.c
	Simple functionality for startup, mounting and unmounting of FAT-based devices.

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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "common.h"
#include "partition.h"
#include "fatfile.h"
#include "fatdir.h"
#include "lock.h"
#include "mem_allocate.h"
#include "disc.h"

bool fatMount (const char* name, const DISC_INTERFACE* interface, sec_t startSector, uint32_t cacheSize, uint32_t SectorsPerPage) {
	if(!name || strlen(name) > 8 || !interface)
		return false;

	if(!interface->startup())
		return false;

	if(!interface->isInserted())
		return false;

	// Initialize the file system
	single_partition = _FAT_partition_constructor (interface, cacheSize, SectorsPerPage, startSector);
	if (!single_partition) {
		return false;
	}

	return true;
}

bool fatMountSimple (const char* name, const DISC_INTERFACE* interface) {
	return fatMount (name, interface, 0, DEFAULT_CACHE_PAGES, DEFAULT_SECTORS_PAGE);
}

void fatUnmount (const char* name) {
	if(!name)
		return;

	_FAT_partition_destructor (single_partition);
	single_partition = NULL;
}

bool fatInit (uint32_t cacheSize, bool setAsDefaultDevice) {
	int i;
	int defaultDevice = -1;
	const DISC_INTERFACE *disc;

	for (i = 0;
		_FAT_disc_interfaces[i].name != NULL && _FAT_disc_interfaces[i].getInterface != NULL;
		i++)
	{
		disc = _FAT_disc_interfaces[i].getInterface();
		if (fatMount (_FAT_disc_interfaces[i].name, disc, 0, cacheSize, DEFAULT_SECTORS_PAGE)) {
			// The first device to successfully mount is set as the default
			if (defaultDevice < 0) {
				defaultDevice = i;
			}
		}
	}

	if (defaultDevice < 0) {
		// None of our devices mounted
		return false;
	}

	if (setAsDefaultDevice) {
		char filePath[PATH_MAX];
		strcpy (filePath, _FAT_disc_interfaces[defaultDevice].name);
		strcat (filePath, ":/");
		_FAT_chdir (filePath);
	}

	return true;
}

bool fatInitDefault (void) {
	return fatInit (DEFAULT_CACHE_PAGES, true);
}

void fatGetVolumeLabel (const char* name, char *label) {
	if(!name || !label)
		return;

	if(!_FAT_directory_getVolumeLabel(single_partition, label)) { 
		strncpy(label,single_partition->label,11);
		label[11]='\0';
	}
	if(!strncmp(label, "NO NAME", 7)) label[0]='\0';
}
