// decode.cpp
#include "encoding.h"
#include "jis2unicode.h"

namespace {
	// UTF-16 LE/BE decoder
	unsigned int decode_utf16(short *dest, unsigned int dest_size, const unsigned char *src, unsigned int src_size)
	{
		int high = 1, low = 0; // for endian
		unsigned char b1, b2;
		int code1, code2;
		unsigned int len = 0;

		if (src_size < 2) return 0;

		// recognize BOM
		b1 = src[0], b2 = src[1];
		if (b1 == 0xff && b2 == 0xfe) {
			// little endian
			src += 2, src_size -= 2;
		} else if (b1 == 0xfe && b2 == 0xff) {
			// big endian
			src += 2, src_size -= 2;
			high = 0, low = 1;
		}

		if (!dest) {
			// counting
			for (unsigned int i = 0; i < src_size - 1; i += 2, ++len) {
				code1 = ((int)src[i + high] << 8) | (int)src[i + low];
				// end of text
				if (code1 == 0x0000) break;
				// surrogate pair
				if ((code1 & 0xfc00) == 0xd800 && i < src_size - 3) {
					code2 = ((int)src[i + 2 + high] << 8) | (int)src[i + 2 + low];
					if ((code2 & 0xfc00) == 0xdc00) i += 2;
				}
			}
			return len;
		}

		// decoding
		for (unsigned int i = 0; i < src_size - 1 && len < dest_size; i += 2, ++len) {
			code1 = ((int)src[i + high] << 8) | (int)src[i + low];
			// end of text
			if (code1 == 0x0000) break;
			// surrogate pair
			if ((code1 & 0xfc00) == 0xd800 && i < src_size - 3) {
				code2 = ((int)src[i + 2 + high] << 8) | (int)src[i + 2 + low];
				if ((code2 & 0xfc00) == 0xdc00) {
					dest[len] = (((code1 & 0x0003ff) << 10) | (code2 & 0x0003ff)) + 0x010000;
					i += 2;
				}
				else dest[len] = Encoding::UNICODE_BAD_SEQUENCE;
			}
			else dest[len] = code1;
		}
		return len;
	}

	// UTF-8 decoder
	unsigned int decode_utf8(short *dest, unsigned int dest_size, const unsigned char *src, unsigned int src_size)
	{
		unsigned char b1, b2;
		unsigned int len = 0;

		// recognize BOM
		if (src_size > 3)
			if (src[0] == 0xef && src[1] == 0xbb && src[2] == 0xbf)
				src += 3, src_size -= 3;

		if (!dest) {
			// counting
			for (unsigned int i = 0; i < src_size; ++i, ++len) {
				b1 = src[i];
				// end of text
				if (b1 == 0x00) break;
				// 1 byte sequence (ASCII compatible)/other errors
				if (b1 < 0xc2 || 0xfd < b1) continue;
				// 2~6 bytes sequence
				unsigned char sup = 0xdf;
				for (int bytes = 2; bytes <= 6; ++bytes) {
					if (b1 <= sup && i <= src_size - bytes) {
						bool p = true;
						for (int n = 1; n < bytes; ++n) {
							b2 = src[i + n];
							if (b2 < 0x80 || 0xbf < b2) {
								p = false;
								break;
							}
						}
						if (p) i += bytes - 1;
						break;
					}
					sup = (sup >> 1) | 0x80;
				}
			}
			return len;
		}

		// decoding
		for (unsigned int i = 0; i < src_size && len < dest_size; ++i, ++len) {
			b1 = src[i];
			// end of text
			if (b1 == 0x00) break;
			// 1 byte sequence (ASCII compatible)
			if (b1 <= 0x7f) {
				dest[len] = (int)b1;
				continue;
			}
			// other errors
			else if (b1 < 0xc2 || 0xfd < b1) {
				dest[len] = Encoding::UNICODE_BAD_SEQUENCE;
				continue;
			}
			// 2~6 bytes sequence
			unsigned char sup = 0xdf, mask = 0x1f;
			for (int bytes = 2; bytes <= 6; ++bytes) {
				if (b1 <= sup && i <= src_size - bytes) {
					dest[len] = b1 & mask;
					bool p = true;
					for (int n = 1; n < bytes; ++n) {
						b2 = src[i + n];
						if (b2 < 0x80 || 0xbf < b2) {
							dest[len] = Encoding::UNICODE_BAD_SEQUENCE;
							p = false;
							break;
						}
						dest[len] = dest[len] << 6 | (b2 & 0x3f);
					}
					if (p) i += bytes - 1;
					break;
				}
				sup = (sup >> 1) | 0x80;
				mask >>= 1;
			}
		}
		return len;
	}

	// Shift_JIS decoder
	unsigned int decode_shiftjis(short *dest, unsigned int dest_size, const unsigned char *src, unsigned int src_size)
	{
		const int KU_SIZE = 94;
		const int offset[] = {
			-1,  0*KU_SIZE,  2*KU_SIZE,  4*KU_SIZE,  6*KU_SIZE,  8*KU_SIZE, 10*KU_SIZE, 12*KU_SIZE,
			14*KU_SIZE, 16*KU_SIZE, 18*KU_SIZE, 20*KU_SIZE, 22*KU_SIZE, 24*KU_SIZE, 26*KU_SIZE, 28*KU_SIZE,
			30*KU_SIZE, 32*KU_SIZE, 34*KU_SIZE, 36*KU_SIZE, 38*KU_SIZE, 40*KU_SIZE, 42*KU_SIZE, 44*KU_SIZE,
			46*KU_SIZE, 48*KU_SIZE, 50*KU_SIZE, 52*KU_SIZE, 54*KU_SIZE, 56*KU_SIZE, 58*KU_SIZE, 60*KU_SIZE,
			-1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1,
			62*KU_SIZE, 64*KU_SIZE, 66*KU_SIZE, 68*KU_SIZE, 70*KU_SIZE, 72*KU_SIZE, 74*KU_SIZE, 76*KU_SIZE,
			78*KU_SIZE, 80*KU_SIZE, 82*KU_SIZE, 84*KU_SIZE, 86*KU_SIZE, 88*KU_SIZE, 90*KU_SIZE, 92*KU_SIZE,
			(94+ 0)*KU_SIZE, (94+ 2)*KU_SIZE, (94+ 4)*KU_SIZE, (94+12)*KU_SIZE, (94+14)*KU_SIZE, (94+78)*KU_SIZE, (94+80)*KU_SIZE, (94+82)*KU_SIZE,
			(94+84)*KU_SIZE, (94+86)*KU_SIZE, (94+88)*KU_SIZE, (94+90)*KU_SIZE, (94+92)*KU_SIZE, -1, -1, -1
		};

		unsigned char b1, b2;
		int codepoint;
		unsigned int len = 0;

		if (!dest) {
			// counting
			for (unsigned int i = 0; i < src_size; ++i, ++len) {
				b1  = src[i];
				// end of text
				if (b1 == 0x00) break;
				// 2 bytes sequence
				if (Encoding::jisx0201_2_unicode[b1] == Encoding::UNICODE_BAD_SEQUENCE) {
					if (++i >= src_size) break;
					b2 = src[i];
					// correct sequence
					if (offset[b1 - 0x80] != -1 && b2 != 0x7f && 0x40 <= b2 && b2 <= 0xfc) continue;
					// bad sequence
					--i;
				}
			}
			return len;
		}

		// decoding
		for (unsigned int i = 0; i < src_size && len < dest_size; ++i, ++len) {
			b1  = src[i];
			// end of text
			if (b1 == 0x00) break;
			// 1 byte sequence (ASCII & JIS X 0201)
			if (Encoding::jisx0201_2_unicode[b1] != Encoding::UNICODE_BAD_SEQUENCE)
				dest[len] = Encoding::jisx0201_2_unicode[b1];
			// 2 bytes sequence (JIS X 0208)
			else {
				if (++i >= src_size) break;
				b2 = src[i];
				// correct sequence
				if (offset[b1 - 0x80] != -1 && b2 != 0x7f && 0x40 <= b2 && b2 <= 0xfc) {
					codepoint = offset[b1 - 0x80] + (b2 < 0x80 ? b2 : b2 - 1) - 0x40;
					// jis x 0213 shifting
					if (b1 >= 0xf0) {
						if (b1 == 0xf0 && b2 >= 0x80) codepoint += 6*KU_SIZE;
						else if (b1 == 0xf2 && b2 >= 0x80) codepoint += 6*KU_SIZE;
						else if (b1 == 0xf4 && b2 >= 0x80) codepoint += 62*KU_SIZE;
					}
					dest[len] = Encoding::jisx0213_2_unicode[codepoint];
				}
				// bad sequence
				else {
					dest[len] = Encoding::UNICODE_BAD_SEQUENCE;
					--i;
				}
			}
		}
		return len;
	}

	// EUC-JP decoder
	unsigned int decode_eucjp(short *dest, unsigned int dest_size, const unsigned char *src, unsigned int src_size)
	{
		unsigned char b1, b2, b3;
		unsigned int len = 0;

		if (!dest) {
			// counting
			for (unsigned int i = 0; i < src_size; ++i, ++len) {
				b1 = src[i];
				// end of text
				if (b1 == 0x00) break;
				// 3 bytes sequence (JIS X 0213 plane 2)
				if (b1 == 0x8f) {
					if ((i += 2) >= src_size) break;
					b2 = src[i - 1], b3 = src[i];
					if (0xa1 <= b2 && b2 <= 0xfe && 0xa1 <= b3 && b3 <= 0xfe) continue;
					// bad sequence
					i -= 2;
				}
				// 2 bytes sequence (JIS X 0201 kana/JIS X 0213 plane 1)
				else if (b1 >= 0x80) {
					if (++i >= src_size) break;
					b2 = src[i];
					if (b1 == 0x8e || (0xa1 <= b1 && b1 <= 0xfe && 0xa1 <= b2 && b2 <= 0xfe)) continue;
					// bad sequence
					--i;
				}
			}
			return len;
		}

		// decoding
		for (unsigned int i = 0; i < src_size && len < dest_size; ++i, ++len) {
			b1 = src[i];
			// end of text
			if (b1 == 0x00) break;
			// 1 byte sequence
			if (b1 <= 0x7f) dest[len] = (int)b1;
			// 3 bytes sequence (JIS X 0213 plane 2)
			else if (b1 == 0x8f) {
				if ((i += 2) >= src_size) break;
				b2 = src[i - 1], b3 = src[i];
				if (0xa1 <= b2 && b2 <= 0xfe && 0xa1 <= b3 && b3 <= 0xfe)
					dest[len] = Encoding::jisx0213_2_unicode[(b2 - 0xa1 + 94) * 94 + (b3 - 0xa1)];
				// bad sequence
				else {
					dest[len] = Encoding::UNICODE_BAD_SEQUENCE;
					i -= 2;
				}
			}
			// 2 bytes sequence
			else /* b1 >= 0x80 */ {
				if (++i >= src_size) break;
				b2 = src[i];
				// JIS X 0201 kana
				if (b1 == 0x8e) dest[len] = Encoding::jisx0201_2_unicode[b2];
				// JIS X 0213 plane 1
				else if (0xa1 <= b1 && b1 <= 0xfe && 0xa1 <= b2 && b2 <= 0xfe)
					dest[len] = Encoding::jisx0213_2_unicode[(b1 - 0xa1) * 94 + (b2 - 0xa1)];
				// bad sequence
				else {
					dest[len] = Encoding::UNICODE_BAD_SEQUENCE;
					--i;
				}
			}
		}
		return len;
	}
}

unsigned int Encoding::decode(short *dest, unsigned int dest_size, const unsigned char *src, unsigned int src_size, EncodingType encoding)
{
	// auto encoding judgement
	if (encoding == NONE) encoding = getEncoding(src, src_size);

	// dispatching
	switch (encoding) {
	case UTF16: return ::decode_utf16(dest, dest_size, src, src_size);
	case UTF8: return ::decode_utf8(dest, dest_size, src, src_size);
	case SHIFTJIS: return ::decode_shiftjis(dest, dest_size, src, src_size);
	case EUCJP: return ::decode_eucjp(dest, dest_size, src, src_size);
	}

	// unknown encoding.
	return 0;
}
