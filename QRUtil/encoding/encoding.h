// encoding.h

#ifndef INCLUDED_ENCODING_H_
#define INCLUDED_ENCODING_H_

namespace Encoding {

	/// Encoding type
	enum class EncodingType {
		/// Unknown encoding
		NONE = 0,
		/// UTF-16 LE/BE encoding
		UTF16,
		/// UTF-8 encoding
		UTF8,
		/// Shift_JIS/CP932 encoding
		SHIFTJIS,
		/// EUC-JP encoding
		EUCJP
	};

	/// Unicode registered symbols
	enum UnicodeSymbol {
		/// Unrecognized character (Geta character)
		UNICODE_BAD_SEQUENCE = 0xfffd
	};

	/** Transform a specified encoding into Unicode codepoint.
	 *
	 * @param dest Destination pointer for Unicode codepoint sequence or nullptr.
	 * @param dest_size Maximum length of dest (without L'\0').
	 * @param src Source text.
	 * @param src_size Maximum length of src (without L'\0').
	 * @param encoding Encoding of src.
	 * 
	 * @retval The length of the text which is actually decoded.
	 * (with invalid code (0xfffd), without L'\0')
	 * If dest is nullptr, this function only counts the necessary size of dest.
	 */
	unsigned int decode(short *dest, unsigned int dest_size, const unsigned char *src, unsigned int src_size, EncodingType encoding);

	/** Transform a specified encoding into Unicode codepoint.
	 *
	 * @param dest Destination pointer for encoded text or nullptr.
	 * @param dest_size Maximum length of dest (without L'\0').
	 * @param src Unicode codepoint sequence.
	 * @param src_size Maximum length of src (without L'\0').
	 * @param encoding Encoding of dest.
	 * 
	 * @retval The length of the text which is actually encoded.
	 * (without L'\0')
	 * If dest is nullptr, this function only counts the necessary size of dest.
	 */
	unsigned int encode(unsigned char *dest, unsigned int dest_size, const int *src, unsigned int src_size, EncodingType encoding);

	/** Guess the encoding type of text data.
	 *
	 * @param src Source text.
	 * @param src_size Maximum length of src (without '\0').
	 * 
	 * @retval An EncodingType value which is guessed from src.
	 */
	 EncodingType getEncoding(const unsigned char *src, unsigned int src_size);

} // namespace Encoding

#endif // INCLUDED_ENCODING_H_
