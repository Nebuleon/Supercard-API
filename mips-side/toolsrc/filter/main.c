/*
 * This file is part of the MIPS filterer for the Supercard DSTwo.
 *
 * Copyright 2017 Nebuleon Fumika <nebuleon.fumika@gmail.com>
 *
 * It is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with it.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filter.h"

const char UNFILTERER_NAME[] = "unfilter.dat";

/* These are the boundaries of the filtered executable as it exists in
 * Supercard RAM before the unfilterer's code runs. */
#define FILTERED_ADDR_START   UINT32_C(0x81000000)
#define FILTERED_ADDR_END     UINT32_C(0x81FF0000)

/* These are the boundaries of the unfiltered executable as it will exist in
 * Supercard RAM after the unfilterer's code runs. */
#define UNFILTERED_ADDR_START UINT32_C(0x80002000)
#define UNFILTERED_ADDR_END   UINT32_C(0x81000000)

int main(int argc, char** argv)
{
	int ret = EXIT_SUCCESS;
	char *program_path, *unfilterer_path;
	size_t unfiltered_size, filtered_size, unfilterer_size;
	uint32_t instructions;
	FILE *infile, *outfile, *unfilterer;
	uint8_t *unfiltered_buf, *unfilterer_buf;
	size_t read, total_read, written, total_written;

	if (argc < 3) {
		fprintf(stderr, "usage: filter INFILE OUTFILE\n\nINFILE is a file containing a concatenation of the .text (code), .rodata (read-only data) and .data sections of your MIPS plugin. Those files will often have the extension .dat and be created by 'objcopy -x -O binary'.\n\nfilter works better if the .text section is first in the file.\n\nOUTFILE will be a similar kind of file after filtering.\n");
		ret = EXIT_FAILURE;
		goto end;
	}

	program_path = malloc(strlen(argv[0]) + 1);
	strcpy(program_path, argv[0]);

	/* Grab the path of the executable, without its file name */
	char* program_slash = strrchr(program_path, '/');
	if (program_slash) {
		unfilterer_path = malloc((program_slash - program_path) + sizeof(UNFILTERER_NAME) + 2);
		memcpy(unfilterer_path, program_path, (program_slash - program_path) + 1);
		/* Put the name of the MIPS unfilterer after the last slash */
		strcpy(unfilterer_path + (program_slash - program_path) + 1, UNFILTERER_NAME);
	} else {
		fprintf(stderr, "filter fatal error: could not determine the path containing the executable\n");
		ret = EXIT_FAILURE;
		goto with_program_path;
	}

	/* Open the three required files:
	 * a) The original MIPS executable (INFILE) */
	infile = fopen(argv[1], "rb");
	if (infile == NULL) {
		fprintf(stderr, "filter fatal error: could not open input file %s: %s\n", argv[1], strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unfilterer_path;
	}

	/* b) The filtered MIPS executable (OUTFILE) */
	outfile = fopen(argv[2], "wb");
	if (outfile == NULL) {
		fprintf(stderr, "filter fatal error: could not open output file %s: %s\n", argv[2], strerror(errno));
		ret = EXIT_FAILURE;
		goto with_infile;
	}

	/* c) The MIPS unfilterer (UNFILTERER_NAME) */
	unfilterer = fopen(unfilterer_path, "rb");
	if (unfilterer == NULL) {
		fprintf(stderr, "filter fatal error: could not open auxiliary file %s: %s\n", unfilterer_path, strerror(errno));
		ret = EXIT_FAILURE;
		goto with_outfile;
	}

	/* Read the entire unfiltered executable */
	if (fseek(infile, 0, SEEK_END) == -1) {
		fprintf(stderr, "filter fatal error: could not seek in input file %s: %s\n", argv[1], strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unfilterer;
	}

	unfiltered_size = ftell(infile);
	if (unfiltered_size > UNFILTERED_ADDR_END - UNFILTERED_ADDR_START) {
		fprintf(stderr, "filter fatal error: input file %s too large for the Supercard DSTwo RAM (%ld > %" PRIu32 ")\n", argv[1], unfiltered_size, UNFILTERED_ADDR_END - UNFILTERED_ADDR_START);
		ret = EXIT_FAILURE;
		goto with_unfilterer;
	}

	if (fseek(infile, 0, SEEK_SET) == -1) {
		fprintf(stderr, "filter fatal error: could not seek in input file %s: %s\n", argv[1], strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unfilterer;
	}

	unfiltered_buf = malloc(unfiltered_size);
	if (!unfiltered_buf) {
		fprintf(stderr, "filter fatal error: failed to allocate %ld bytes of memory for input data\n", unfiltered_size);
		ret = EXIT_FAILURE;
		goto with_unfilterer;
	}

	total_read = 0;
	while (total_read < unfiltered_size) {
		read = fread(unfiltered_buf + total_read, 1, unfiltered_size - total_read, infile);
		if (feof(infile)) {
			fprintf(stderr, "filter fatal error: end-of-file reached while reading input file %s with %ld more bytes expected\n", argv[1], unfiltered_size - total_read);
			ret = EXIT_FAILURE;
			goto with_unfiltered_buf;
		} else if (ferror(infile)) {
			fprintf(stderr, "filter fatal error: failed to read from input file %s: %s\n", argv[1], strerror(errno));
			ret = EXIT_FAILURE;
			goto with_unfiltered_buf;
		}
		total_read += read;
	}

	/* Read the entire unfilterer */
	if (fseek(unfilterer, 0, SEEK_END) == -1) {
		fprintf(stderr, "filter fatal error: could not seek in auxiliary file %s: %s\n", unfilterer_path, strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unfiltered_buf;
	}

	unfilterer_size = ftell(unfilterer);

	if (fseek(unfilterer, 0, SEEK_SET) == -1) {
		fprintf(stderr, "filter fatal error: could not seek in auxiliary file %s: %s\n", unfilterer_path, strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unfiltered_buf;
	}

	unfilterer_buf = malloc(unfilterer_size);
	if (!unfilterer_buf) {
		fprintf(stderr, "filter fatal error: failed to allocate %zu bytes of memory for auxiliary data\n", unfilterer_size);
		ret = EXIT_FAILURE;
		goto with_unfiltered_buf;
	}

	total_read = 0;
	while (total_read < unfilterer_size) {
		read = fread(unfilterer_buf + total_read, 1, unfilterer_size - total_read, unfilterer);
		if (feof(unfilterer)) {
			fprintf(stderr, "filter fatal error: end-of-file reached while reading auxiliary file %s with %ld more bytes expected\n", unfilterer_path, unfilterer_size - total_read);
			ret = EXIT_FAILURE;
			goto with_unfilterer_buf;
		} else if (ferror(unfilterer)) {
			fprintf(stderr, "filter fatal error: failed to read from input file %s: %s\n", unfilterer_path, strerror(errno));
			ret = EXIT_FAILURE;
			goto with_unfilterer_buf;
		}
		total_read += read;
	}

	/* Filter */
	instructions = make_streams(unfiltered_buf, unfiltered_size / 4);
	filtered_size = get_filtered_size() + unfiltered_size - instructions * 4;

	if (unfilterer_size + filtered_size > FILTERED_ADDR_END - FILTERED_ADDR_START) {
		fprintf(stderr, "filter fatal error: output file %s and unfilterer too large for the Supercard DSTwo RAM (%ld > %" PRIu32 ")\n", argv[1], unfilterer_size + filtered_size, FILTERED_ADDR_END - FILTERED_ADDR_START);
		ret = EXIT_FAILURE;
		goto with_streams;
	}

	/* The first two instructions of the unfilterer are LUI s4, 0 and
	 * ORI s4, s4, 0. They will contain the filtered size. LUI gets
	 * the high 16 bits in its lowest 16 bits, little-endian; ORI
	 * gets the low 16 bits in its lowest 16 bits. */
	unfilterer_buf[0]  /* LUI lowest byte */ = (filtered_size >> 16) & 0xFF;
	unfilterer_buf[1]  /* LUI highest byte */ = (filtered_size >> 24) & 0xFF;
	unfilterer_buf[4]  /* ORI lowest byte */ = filtered_size & 0xFF;
	unfilterer_buf[5]  /* ORI highest byte */ = (filtered_size >> 8) & 0xFF;

	/* The next two instructions are LUI s5, 0 and ORI s5, s5, 0. They will
	 * contain the number of instructions. */
	unfilterer_buf[8]  /* LUI lowest byte */ = (instructions >> 16) & 0xFF;
	unfilterer_buf[9]  /* LUI highest byte */ = (instructions >> 24) & 0xFF;
	unfilterer_buf[12] /* ORI lowest byte */ = instructions & 0xFF;
	unfilterer_buf[13] /* ORI highest byte */ = (instructions >> 8) & 0xFF;

	/* Write the unfilterer */
	total_written = 0;
	while (total_written < unfilterer_size) {
		written = fwrite(unfilterer_buf + total_written, 1, unfilterer_size - total_written, outfile);
		if (ferror(outfile)) {
			fprintf(stderr, "filter fatal error: failed to write to output file %s: %s\n", argv[2], strerror(errno));
			ret = EXIT_FAILURE;
			goto with_streams;
		}
		total_written += written;
	}

	if (ferror(infile)) {
		fprintf(stderr, "filter fatal error: failed to read from auxiliary file %s: %s\n", unfilterer_path, strerror(errno));
		ret = EXIT_FAILURE;
		goto with_streams;
	}

	/* Write the filtered data */
	if (!write_streams(outfile)) {
		ret = EXIT_FAILURE;
		goto with_streams;
	}

	/* And write the data segment */
	total_written = 0;
	while (total_written < unfiltered_size - instructions * 4) {
		written = fwrite(unfiltered_buf + instructions * 4 + total_written, 1,
			unfiltered_size - instructions * 4 - total_written, outfile);
		if (ferror(outfile)) {
			fprintf(stderr, "filter fatal error: failed to write to output file %s: %s\n", argv[2], strerror(errno));
			ret = EXIT_FAILURE;
			goto with_streams;
		}
		total_written += written;
	}

with_streams:
	free_streams();
with_unfilterer_buf:
	free(unfilterer_buf);
with_unfiltered_buf:
	free(unfiltered_buf);
with_unfilterer:
	fclose(unfilterer);
with_outfile:
	fclose(outfile);
with_infile:
	fclose(infile);
with_unfilterer_path:
	free(unfilterer_path);
with_program_path:
	free(program_path);

end:
	return ret;
}
