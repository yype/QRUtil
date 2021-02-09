// jis2unicode.h
#ifndef INCLUDED_JIS_2_UNICODE_H_
#define INCLUDED_JIS_2_UNICODE_H_

namespace Encoding {
	/** JIS X 0201 to Unicode translating table
	 * 
	 * Untranslatable character is translated into U+FFFD character.
	 * USAGE:
	 *   unicode = jisx0201_2_unicode[jis];
	 *     unicode: unicode character
	 *     jis    : JIS X 0201 character
	*/
	extern const int jisx0201_2_unicode[];

	/** JIS X 0213 to Unicode translating table
	 * 
	 * Untranslatable character is translated into U+FFFD character.
	 * USAGE:
	 * 	 unicode = jisx0213_2_unicode[(plane-1)*94*94 + (row-1)*94 + (col-1)];
	 * 	   unicode: unicode character
	 *     plane  : JIS X 0213 "kaku" [1:2]
	 *     row    : JIS X 0213 "ku" [1:94]
	 *     col    : JIS X 0213 "ten" [1:94]
	*/
	extern const int jisx0213_2_unicode[];
}

#endif // INCLUDED_JIS_2_UNICODE_H_
