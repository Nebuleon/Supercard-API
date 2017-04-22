#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

const char UNPACKER_NAME[] = "unpacker.dat";

/* These are the boundaries of the packed executable as it exists in Supercard
 * RAM before the inflate() call in the unpacker. */
#define PACKED_ADDR_START   UINT32_C(0x81000000)
#define PACKED_ADDR_END     UINT32_C(0x81FF0000)

/* These are the boundaries of the unpacked executable as it will exist in
 * Supercard RAM after the inflate() call in the unpacker. */
#define UNPACKED_ADDR_START UINT32_C(0x80002000)
#define UNPACKED_ADDR_END   UINT32_C(0x81000000)

int main(int argc, char** argv)
{
	int ret = EXIT_SUCCESS, z_result;
	z_stream stream;
	char *program_path, *unpacker_path;
	size_t unpacked_size, packed_size, unpacker_size;
	FILE *infile, *outfile, *unpacker;
	uint8_t *unpacked_buf, *packed_buf, *unpacker_buf;
	size_t read, total_read, written, total_written;

	if (argc < 3) {
		fprintf(stderr, "usage: packer INFILE OUTFILE\n\nINFILE is a file containing a concatenation of the .text (code), .rodata (read-only data) and .data sections of your MIPS plugin. Those files will often have the extension .dat and be created by 'objcopy -x -O binary'.\n\nOUTFILE will be a similar kind of file after packing.\n");
		ret = EXIT_FAILURE;
		goto end;
	}

	program_path = malloc(strlen(argv[0]) + 1);
	strcpy(program_path, argv[0]);

	/* Grab the path of the executable, without its file name */
	char* program_slash = strrchr(program_path, '/');
	if (program_slash) {
		unpacker_path = malloc((program_slash - program_path) + sizeof(UNPACKER_NAME) + 2);
		memcpy(unpacker_path, program_path, (program_slash - program_path) + 1);
		/* Put the name of the MIPS unpacker after the last slash */
		strcpy(unpacker_path + (program_slash - program_path) + 1, UNPACKER_NAME);
	} else {
		fprintf(stderr, "packer fatal error: could not determine the path containing the executable\n");
		ret = EXIT_FAILURE;
		goto with_program_path;
	}

	/* Open the three required files:
	 * a) The original MIPS executable (INFILE) */
	infile = fopen(argv[1], "rb");
	if (infile == NULL) {
		fprintf(stderr, "packer fatal error: could not open input file %s: %s\n", argv[1], strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unpacker_path;
	}

	/* b) The packed MIPS executable (OUTFILE) */
	outfile = fopen(argv[2], "wb");
	if (outfile == NULL) {
		fprintf(stderr, "packer fatal error: could not open output file %s: %s\n", argv[2], strerror(errno));
		ret = EXIT_FAILURE;
		goto with_infile;
	}

	/* c) The MIPS unpacker (UNPACKER_NAME) */
	unpacker = fopen(unpacker_path, "rb");
	if (unpacker == NULL) {
		fprintf(stderr, "packer fatal error: could not open auxiliary file %s: %s\n", unpacker_path, strerror(errno));
		ret = EXIT_FAILURE;
		goto with_outfile;
	}

	/* Read the entire unpacked executable */
	if (fseek(infile, 0, SEEK_END) == -1) {
		fprintf(stderr, "packer fatal error: could not seek in input file %s: %s\n", argv[1], strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unpacker;
	}

	unpacked_size = ftell(infile);
	if (unpacked_size > UNPACKED_ADDR_END - UNPACKED_ADDR_START) {
		fprintf(stderr, "packer fatal error: input file %s too large for the Supercard DSTwo RAM (%ld > %" PRIu32 ")\n", argv[1], unpacked_size, UNPACKED_ADDR_END - UNPACKED_ADDR_START);
		ret = EXIT_FAILURE;
		goto with_unpacker;
	}

	if (fseek(infile, 0, SEEK_SET) == -1) {
		fprintf(stderr, "packer fatal error: could not seek in input file %s: %s\n", argv[1], strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unpacker;
	}

	unpacked_buf = malloc(unpacked_size);
	if (!unpacked_buf) {
		fprintf(stderr, "packer fatal error: failed to allocate %ld bytes of memory for input data\n", unpacked_size);
		ret = EXIT_FAILURE;
		goto with_unpacker;
	}

	total_read = 0;
	while (total_read < unpacked_size) {
		read = fread(unpacked_buf + total_read, 1, unpacked_size - total_read, infile);
		if (feof(infile)) {
			fprintf(stderr, "packer fatal error: end-of-file reached while reading input file %s with %ld more bytes expected\n", argv[1], unpacked_size - total_read);
			ret = EXIT_FAILURE;
			goto with_unpacked_buf;
		} else if (ferror(infile)) {
			fprintf(stderr, "packer fatal error: failed to read from input file %s: %s\n", argv[1], strerror(errno));
			ret = EXIT_FAILURE;
			goto with_unpacked_buf;
		}
		total_read += read;
	}

	/* Read the entire unpacker */
	if (fseek(unpacker, 0, SEEK_END) == -1) {
		fprintf(stderr, "packer fatal error: could not seek in auxiliary file %s: %s\n", unpacker_path, strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unpacked_buf;
	}

	unpacker_size = ftell(unpacker);

	if (fseek(unpacker, 0, SEEK_SET) == -1) {
		fprintf(stderr, "packer fatal error: could not seek in auxiliary file %s: %s\n", unpacker_path, strerror(errno));
		ret = EXIT_FAILURE;
		goto with_unpacked_buf;
	}

	unpacker_buf = malloc(unpacker_size);
	if (!unpacker_buf) {
		fprintf(stderr, "packer fatal error: failed to allocate %zu bytes of memory for auxiliary data\n", unpacker_size);
		ret = EXIT_FAILURE;
		goto with_unpacked_buf;
	}

	total_read = 0;
	while (total_read < unpacker_size) {
		read = fread(unpacker_buf + total_read, 1, unpacker_size - total_read, unpacker);
		if (feof(unpacker)) {
			fprintf(stderr, "packer fatal error: end-of-file reached while reading auxiliary file %s with %ld more bytes expected\n", unpacker_path, unpacker_size - total_read);
			ret = EXIT_FAILURE;
			goto with_unpacker_buf;
		} else if (ferror(unpacker)) {
			fprintf(stderr, "packer fatal error: failed to read from input file %s: %s\n", unpacker_path, strerror(errno));
			ret = EXIT_FAILURE;
			goto with_unpacker_buf;
		}
		total_read += read;
	}

	/* Prepare for compression */
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	z_result = deflateInit2(&stream, 9, Z_DEFLATED,
		MAX_WBITS /* window size */,
		9 /* memory usage level */,
		Z_DEFAULT_STRATEGY);
	if (z_result != Z_OK) {
		fprintf(stderr, "packer fatal error: failed to allocate memory for zlib\n");
		ret = EXIT_FAILURE;
		goto with_unpacker_buf;
	}

	packed_size = deflateBound(&stream, (uLong) unpacked_size);

	packed_buf = malloc(packed_size);
	if (!packed_buf) {
		fprintf(stderr, "packer fatal error: failed to allocate %lu bytes of memory for output data\n", packed_size);
		ret = EXIT_FAILURE;
		goto with_zlib_stream;
	}

	/* Compress */
	stream.next_in = unpacked_buf;
	stream.avail_in = unpacked_size;
	stream.next_out = packed_buf;
	stream.avail_out = packed_size;
	if ((z_result = deflate(&stream, Z_FINISH)) != Z_STREAM_END) {
		fprintf(stderr, "packer fatal error: deflate error %d\n", z_result);
		ret = EXIT_FAILURE;
		goto with_packed_buf;
	}
	packed_size -= stream.avail_out;

	if (unpacker_size + packed_size > PACKED_ADDR_END - PACKED_ADDR_START) {
		fprintf(stderr, "packer fatal error: output file %s and unpacker too large for the Supercard DSTwo RAM (%ld > %" PRIu32 ")\n", argv[1], unpacker_size + packed_size, PACKED_ADDR_END - PACKED_ADDR_START);
		ret = EXIT_FAILURE;
		goto with_packed_buf;
	}

	/* The first two instructions of the unpacker are LUI s4, 0 and
	 * ORI s4, s4, 0. They will contain the unpacked size. LUI gets
	 * the high 16 bits in its lowest 16 bits, little-endian; ORI
	 * gets the low 16 bits in its lowest 16 bits. */
	unpacker_buf[0]  /* LUI lowest byte */ = (unpacked_size >> 16) & 0xFF;
	unpacker_buf[1]  /* LUI highest byte */ = (unpacked_size >> 24) & 0xFF;
	unpacker_buf[4]  /* ORI lowest byte */ = unpacked_size & 0xFF;
	unpacker_buf[5]  /* ORI highest byte */ = (unpacked_size >> 8) & 0xFF;

	/* The next two instructions are LUI s5, 0 and ORI s5, s5, 0. They will
	 * contain the packed size. */
	unpacker_buf[8]  /* LUI lowest byte */ = (packed_size >> 16) & 0xFF;
	unpacker_buf[9]  /* LUI highest byte */ = (packed_size >> 24) & 0xFF;
	unpacker_buf[12] /* ORI lowest byte */ = packed_size & 0xFF;
	unpacker_buf[13] /* ORI highest byte */ = (packed_size >> 8) & 0xFF;

	/* Write the unpacker */
	total_written = 0;
	while (total_written < unpacker_size) {
		written = fwrite(unpacker_buf + total_written, 1, unpacker_size - total_written, outfile);
		if (ferror(outfile)) {
			fprintf(stderr, "packer fatal error: failed to write to output file %s: %s\n", argv[2], strerror(errno));
			ret = EXIT_FAILURE;
			goto with_packed_buf;
		}
		total_written += written;
	}

	if (ferror(infile)) {
		fprintf(stderr, "packer fatal error: failed to read from auxiliary file %s: %s\n", unpacker_path, strerror(errno));
		ret = EXIT_FAILURE;
		goto with_packed_buf;
	}

	/* And write the packed data */
	total_written = 0;
	while (total_written < packed_size) {
		written = fwrite(packed_buf + total_written, 1, packed_size - total_written, outfile);
		if (ferror(outfile)) {
			fprintf(stderr, "packer fatal error: failed to write to output file %s: %s\n", argv[2], strerror(errno));
			ret = EXIT_FAILURE;
			goto with_packed_buf;
		}
		total_written += written;
	}

with_packed_buf:
	free(packed_buf);
with_zlib_stream:
	deflateEnd(&stream);
with_unpacker_buf:
	free(unpacker_buf);
with_unpacked_buf:
	free(unpacked_buf);
with_unpacker:
	fclose(unpacker);
with_outfile:
	fclose(outfile);
with_infile:
	fclose(infile);
with_unpacker_path:
	free(unpacker_path);
with_program_path:
	free(program_path);

end:
	return ret;
}
