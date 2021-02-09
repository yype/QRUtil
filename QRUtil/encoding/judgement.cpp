// judgement.cpp
#include "encoding.h"

Encoding::EncodingType Encoding::getEncoding(const unsigned char *src, unsigned int src_size)
{
	/* basic idea:
	 *   UTF-16LE/UTF-16BE/UTF-8 with BOM
	 *     recognizing BOM.
	 *   UTF-16LE/UTF-16BE without BOM
	 *     find UTF-16 encoded ASCII.
	 *   UTF-8 without BOM, Shift_JIS, EUC-JP
	 *     Calculate the "similarity value" and choose the largest one.
	 */
	int utf8 = 0, sjis = 0, eucjp = 0;
	unsigned char b1, b2, b3;

	// check UTF-16 BOM
	if (src_size >= 2) {
		// UTF-16 LE
		if (src[0] == 0xff && src[1] == 0xfe) return UTF16;
		// UTF-16 BE
		if (src[0] == 0xfe && src[1] == 0xff) return UTF16;
	}
	// check UTF-8 BOM
	if (src_size >= 3) {
		if (src[0] == 0xef && src[1] == 0xbb && src[2] == 0xbf) return UTF8;
	}

	// find UTF-16 encoded ASCII
	for (unsigned int i = 0; i < src_size - 1; i += 2) {
		if (src[i] == 0x00 || src[i + 1] == 0x00) return UTF16;
	}

	// calculate UTF-8 similarity
	for (unsigned int i = 0; i < src_size; ++i) {
		b1 = src[i];
		// 1 byte sequence
		if (b1 <= 0x7f) {
			++utf8;
			continue;
		}
		// other errors
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
				if (p) utf8 += bytes, i += bytes - 1;
				break;
			}
			sup = (sup >> 1) | 0x80;
		}
	}

	// calculate Shift_JIS similarity
	for (unsigned int i = 0; i < src_size; ++i) {
		b1 = src[i];
		// 1 byte sequence
		if (b1 <= 0x7f || (0xa1 <= b1 && b1 <= 0xdf)) ++sjis;
		// 2 bytes sequence
		else if (((0x81 <= b1 && b1 <= 0x9f) || (0xe0 <= b1 && b1 <= 0xfc)) && i < src_size - 1) {
			b2 = src[i + 1];
			if (b2 != 0x7f && 0x40 <= b2 && b2 <= 0xfc) sjis += 2, ++i;
		}
	}

	// calculate EUC-JP similarity
	for (unsigned int i = 0; i < src_size; ++i) {
		b1 = src[i];
		// 1 byte sequence
		if (b1 <= 0x7f) ++eucjp;
		// 3 bytes sequence (JIS X 0213 plane 2)
		else if (b1 == 0x8f && i < src_size - 2) {
			b2 = src[i + 1], b3 = src[i + 2];
			if (0xa1 <= b2 && b2 <= 0xfe && 0xa1 <= b3 && b3 <= 0xfe) eucjp += 3, i += 2;
		}
		// 2 bytes sequence
		else if (i < src_size - 1) {
			b2 = src[i + 1];
			// JIS X 0201 kana/JIS X 0213 plane 1
			if (b1 == 0x8e || (0xa1 <= b1 && b1 <= 0xfe && 0xa1 <= b2 && b2 <= 0xfe)) eucjp += 2, ++i;
		}
	}

	if (utf8 >= sjis && utf8 >= eucjp) return UTF8;
	if (sjis >= eucjp) return SHIFTJIS;
	return EUCJP;
}
