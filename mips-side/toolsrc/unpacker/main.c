#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>

/* Set by start.S, which is in turn written by the packer in the first four
 * instructions of the unpacker */
uint32_t packed_size __attribute__((section(".noinit")));
uint32_t unpacked_size __attribute__((section(".noinit")));

/* Symbol provided by the linker, at the end of all data sections */
extern Bytef packed_data[];

int main(int argc, char** argv)
{
	z_stream stream;
	int ret = EXIT_SUCCESS, z_result;

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = packed_size;
	stream.next_in = packed_data;
	stream.avail_out = unpacked_size;
	stream.next_out = (Bytef*) 0x80002000;

	z_result = inflateInit2(&stream, 0 /* window size from zlib header */);
	if (z_result != Z_OK) {
		ret = EXIT_FAILURE;
		goto end;
	}

	z_result = inflate(&stream, Z_FINISH);
	if (z_result != Z_STREAM_END) {
		ret = EXIT_FAILURE;
		goto end;
	}

end:
	inflateEnd(&stream);
	return ret;
}
