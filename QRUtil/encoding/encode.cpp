// encode.cpp
#include "encoding.h"
#include "jis2unicode.h"

namespace {
	// UTF-16 LE/BE encoder
	unsigned short encode_utf16(unsigned char *dest, unsigned short dest_size, const short *src, unsigned short src_size)
	{
		// not implemented yet.
		return 0;
	}

	// UTF-8 encoder
	unsigned short encode_utf8(unsigned char *dest, unsigned short dest_size, const short *src, unsigned short src_size)
	{
		// not implemented yet.
		return 0;
	}

	// Shift_JIS encoder
	unsigned short encode_shiftjis(unsigned char *dest, unsigned short dest_size, const short *src, unsigned short src_size)
	{
		// not implemented yet.
		return 0;
	}

	// EUC-JP encoder
	unsigned short encode_eucjp(unsigned char *dest, unsigned short dest_size, const short *src, unsigned short src_size)
	{
		// not implemented yet.
		return 0;
	}

}

unsigned short Encoding::encode(unsigned char *dest, unsigned short dest_size, const short *src, unsigned short src_size, EncodingType encoding)
{
	// dispatching
	switch (encoding) {
	case EncodingType::UTF16: return ::encode_utf16(dest, dest_size, src, src_size);
	case EncodingType::UTF8: return ::encode_utf8(dest, dest_size, src, src_size);
	case EncodingType::SHIFTJIS: return ::encode_shiftjis(dest, dest_size, src, src_size);
	case EncodingType::EUCJP: return ::encode_eucjp(dest, dest_size, src, src_size);
	}

	// unknown encoding.
	return 0;
}