// encode.cpp
#include "encoding.h"
#include "jis2unicode.h"

namespace {
	// UTF-16 LE/BE encoder
	unsigned int encode_utf16(unsigned char *dest, unsigned int dest_size, const int *src, unsigned int src_size)
	{
		// not implemented yet.
		return 0;
	}

	// UTF-8 encoder
	unsigned int encode_utf8(unsigned char *dest, unsigned int dest_size, const int *src, unsigned int src_size)
	{
		// not implemented yet.
		return 0;
	}

	// Shift_JIS encoder
	unsigned int encode_shiftjis(unsigned char *dest, unsigned int dest_size, const int *src, unsigned int src_size)
	{
		// not implemented yet.
		return 0;
	}

	// EUC-JP encoder
	unsigned int encode_eucjp(unsigned char *dest, unsigned int dest_size, const int *src, unsigned int src_size)
	{
		// not implemented yet.
		return 0;
	}

}

unsigned int Encoding::encode(unsigned char *dest, unsigned int dest_size, const int *src, unsigned int src_size, EncodingType encoding)
{
	// dispatching
	switch (encoding) {
	case UTF16: return ::encode_utf16(dest, dest_size, src, src_size);
	case UTF8: return ::encode_utf8(dest, dest_size, src, src_size);
	case SHIFTJIS: return ::encode_shiftjis(dest, dest_size, src, src_size);
	case EUCJP: return ::encode_eucjp(dest, dest_size, src, src_size);
	}

	// unknown encoding.
	return 0;
}