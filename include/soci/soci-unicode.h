#ifndef SOCI_UNICODE_H_INCLUDED
#define SOCI_UNICODE_H_INCLUDED

#include "soci/error.h"
#include <stdexcept>
#include <string>
#include <wchar.h>

// Include necessary headers based on the platform's capabilities for SIMD instructions
#if defined(SOCI_USE_SSE_4_2) || (defined(_MSC_VER) && defined(_M_X64))
#include <nmmintrin.h> // SSE4.2 intrinsics
#elif defined(SOCI_USE_NEON) || (defined(_M_ARM64) && defined(_MSC_VER))
#include <arm_neon.h>
#endif

// #undef SOCI_USE_NEON

// Define SOCI_WCHAR_T_IS_WIDE if wchar_t is wider than 16 bits (e.g., on Unix/Linux)
#if WCHAR_MAX > 0xFFFFu
#define SOCI_WCHAR_T_IS_WIDE
#endif

namespace soci
{
  namespace details
  {

    /**
     * Helper function to check if a UTF-8 sequence is valid.
     *
     * This function takes a byte sequence and its length as input and checks if the sequence is a valid UTF-8 encoded character.
     *
     * @param bytes Pointer to the byte sequence to be checked.
     * @param length Length of the byte sequence.
     * @return True if the sequence is a valid UTF-8 encoded character, false otherwise.
     */
    inline bool is_valid_utf8_sequence(const unsigned char *bytes, int length)
    {
      if (length == 1)
      {
        return (bytes[0] & 0x80) == 0;
      }
      else if (length == 2)
      {
        return (bytes[0] & 0xE0) == 0xC0 && (bytes[1] & 0xC0) == 0x80;
      }
      else if (length == 3)
      {
        return (bytes[0] & 0xF0) == 0xE0 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80;
      }
      else if (length == 4)
      {
        return (bytes[0] & 0xF8) == 0xF0 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80 && (bytes[3] & 0xC0) == 0x80;
      }
      return false;
    }

    // Check if a UTF-8 string is valid
    /**
     * This function checks if the given string is a valid UTF-8 encoded string.
     * It iterates over each byte in the string and checks if it is a valid start of a UTF-8 character.
     * If it is, it checks if the following bytes form a valid UTF-8 character sequence.
     * If the string is not a valid UTF-8 string, the function returns false.
     * If the string is a valid UTF-8 string, the function returns true.
     *
     * @param utf8 The string to check for valid UTF-8 encoding.
     * @return True if the string is a valid UTF-8 encoded string, false otherwise.
     */
    inline bool is_valid_utf8(const std::string &utf8)
    {
      const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
      std::size_t length = utf8.length();

      for (std::size_t i = 0; i < length;)
      {
        if ((bytes[i] & 0x80) == 0)
        {
          // ASCII character, one byte
          i += 1;
        }
        else if ((bytes[i] & 0xE0) == 0xC0)
        {
          // Two-byte character, check if the next byte is a valid continuation byte
          if (i + 1 >= length || !is_valid_utf8_sequence(bytes + i, 2))
            return false;
          i += 2;
        }
        else if ((bytes[i] & 0xF0) == 0xE0)
        {
          // Three-byte character, check if the next two bytes are valid continuation bytes
          if (i + 2 >= length || !is_valid_utf8_sequence(bytes + i, 3))
            return false;
          i += 3;
        }
        else if ((bytes[i] & 0xF8) == 0xF0)
        {
          // Four-byte character, check if the next three bytes are valid continuation bytes
          if (i + 3 >= length || !is_valid_utf8_sequence(bytes + i, 4))
            return false;
          i += 4;
        }
        else
        {
          // Invalid start byte
          return false;
        }
      }
      return true;
    }

    /**
     * Check if a given UTF-16 string is valid.
     *
     * A UTF-16 string is considered valid if it follows the UTF-16 encoding rules.
     * This means that all code units in the range 0xD800 to 0xDBFF must be followed
     * by a code unit in the range 0xDC00 to 0xDFFF. Conversely, code units in the
     * range 0xDC00 to 0xDFFF must not appear without a preceding code unit in the
     * range 0xD800 to 0xDBFF.
     *
     * @param utf16 The UTF-16 string to check.
     * @return True if the string is valid, false otherwise.
     */
    inline bool is_valid_utf16(const std::u16string &utf16)
    {
      const char16_t *chars = utf16.data();
      std::size_t length = utf16.length();

      for (std::size_t i = 0; i < length; ++i)
      {
        char16_t c = chars[i];
        if (c >= 0xD800 && c <= 0xDBFF)
        {
          if (i + 1 >= length)
            return false;
          char16_t next = chars[i + 1];
          if (next < 0xDC00 || next > 0xDFFF)
            return false;
          ++i;
        }
        else if (c >= 0xDC00 && c <= 0xDFFF)
        {
          return false;
        }
      }
      return true;
    }

    /**
     * Check if a UTF-32 string is valid.
     *
     * A UTF-32 string is considered valid if all its characters are within the
     * Unicode range (0x000000 to 0x10FFFF).
     *
     * @param utf32 The UTF-32 string to check.
     * @return True if the UTF-32 string is valid, false otherwise.
     */
    inline bool is_valid_utf32(const std::u32string &utf32)
    {
      const char32_t *chars = utf32.data();
      std::size_t length = utf32.length();

      for (std::size_t i = 0; i < length; ++i)
      {
        char32_t c = chars[i];
        if (c > 0x10FFFF)
          return false;
      }
      return true;
    }

    inline void utf8_to_utf16_common(const unsigned char *bytes, std::size_t length, std::u16string &utf16)
    {
      for (std::size_t i = 0; i < length;)
      {
        if ((bytes[i] & 0x80) == 0)
        {
          // ASCII character, one byte
          utf16.push_back(static_cast<char16_t>(bytes[i]));
          i += 1;
        }
        else if ((bytes[i] & 0xE0) == 0xC0)
        {
          // Two-byte character, check if the next byte is a valid continuation byte
          if (i + 1 >= length || !is_valid_utf8_sequence(bytes + i, 2))
            throw soci_error("Invalid UTF-8 sequence");
          utf16.push_back(static_cast<char16_t>(((bytes[i] & 0x1F) << 6) | (bytes[i + 1] & 0x3F)));
          i += 2;
        }
        else if ((bytes[i] & 0xF0) == 0xE0)
        {
          // Three-byte character, check if the next two bytes are valid continuation bytes
          if (i + 2 >= length || !is_valid_utf8_sequence(bytes + i, 3))
            throw soci_error("Invalid UTF-8 sequence");
          utf16.push_back(static_cast<char16_t>(((bytes[i] & 0x0F) << 12) | ((bytes[i + 1] & 0x3F) << 6) | (bytes[i + 2] & 0x3F)));
          i += 3;
        }
        else if ((bytes[i] & 0xF8) == 0xF0)
        {
          // Four-byte character, check if the next three bytes are valid continuation bytes
          if (i + 3 >= length || !is_valid_utf8_sequence(bytes + i, 4))
            throw soci_error("Invalid UTF-8 sequence");
          uint32_t codepoint = ((bytes[i] & 0x07) << 18) | ((bytes[i + 1] & 0x3F) << 12) | ((bytes[i + 2] & 0x3F) << 6) | (bytes[i + 3] & 0x3F);
          if (codepoint > 0x10FFFF)
            throw soci_error("Invalid UTF-8 codepoint");
          if (codepoint <= 0xFFFF)
          {
            utf16.push_back(static_cast<char16_t>(codepoint));
          }
          else
          {
            // Encode as a surrogate pair
            codepoint -= 0x10000;
            utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
            utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
          }
          i += 4;
        }
        else
        {
          throw soci_error("Invalid UTF-8 sequence");
        }
      }
    }

    // The following functions use SIMD instructions to optimize the conversion process.
    // If these instructions are not available, fallback functions are used instead.

#if defined(SOCI_USE_SSE_4_2)

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string using SSE4.2 intrinsics.
     *
     * This function takes a std::string containing a UTF-8 encoded string as input and returns a
     * std::u16string containing the equivalent UTF-16 encoded string. The conversion is performed
     * using SIMD instructions provided by the SSE4.2 instruction set, which can significantly
     * improve performance over traditional byte-by-byte conversion methods.
     *
     * @param utf8 A std::string containing a UTF-8 encoded string to be converted.
     * @return A std::u16string containing the equivalent UTF-16 encoded string.
     * @throws soci_error If an invalid UTF-8 sequence is encountered in the input string.
     */
        inline std::u16string utf8_to_utf16_sse42(const std::string &utf8)
    {
      std::u16string utf16;
      utf16.reserve(utf8.size());

      const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
      std::size_t length = utf8.length();

      for (std::size_t i = 0; i < length;)
      {
        if (length - i >= 16)
        {
          __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(bytes + i));
          __m128i mask = _mm_set1_epi8(0x80);
          __m128i result = _mm_cmpeq_epi8(_mm_and_si128(chunk, mask), _mm_setzero_si128());
          int bitfield = _mm_movemask_epi8(result);

          if (bitfield == 0xFFFF)
          {
            // All characters in the chunk are ASCII
            for (int j = 0; j < 16; ++j)
            {
              utf16.push_back(static_cast<char16_t>(bytes[i + j]));
            }
            i += 16;
          }
          else
          {
            // Handle non-ASCII characters with SSE4.2
            // ... (SSE4.2 specific code)
            // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
            utf8_to_utf16_common(bytes + i, length - i, utf16);
            break;
          }
        }
        else
        {
          utf8_to_utf16_common(bytes + i, length - i, utf16);
          break;
        }
      }

      return utf16;
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string using SSE4.2 intrinsics.
     *
     * This function leverages SSE4.2 intrinsics to efficiently convert a UTF-16 encoded string
     * to a UTF-8 encoded string. It processes the input string in chunks of 8 characters at a time.
     * If all characters in a chunk are ASCII (i.e., their values are less than 0x80), they are
     * directly appended to the output string. Otherwise, each character is processed individually
     * and converted to its corresponding UTF-8 representation.
     *
     * @param utf16 The input UTF-16 encoded string.
     * @return A UTF-8 encoded string.
     * @throws soci_error If an invalid UTF-16 sequence is encountered.
     */
    inline std::string utf16_to_utf8_sse42(const std::u16string &utf16)
    {
      std::string utf8;
      utf8.reserve(utf16.size() * 3);

      const char16_t *src = utf16.data();
      const char16_t *end = src + utf16.size();

      while (src < end)
      {
        size_t remaining = end - src;
        if (remaining >= 8)
        {
          // Load a chunk of 8 UTF-16 characters
          __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
          // Mask to identify ASCII characters (0x7F)
          __m128i mask = _mm_set1_epi16(0x7F);
          // Compare and create a result mask
          __m128i result = _mm_cmpeq_epi16(_mm_and_si128(chunk, mask), chunk);

          // Bitfield from the comparison result
          int bitfield = _mm_movemask_epi8(result) & 0xFFFF; // Only lower 16 bits for 8 char16_t
          if (bitfield == 0xFFFF)
          {
            // All characters in the chunk are ASCII
            for (int i = 0; i < 8; ++i)
            {
              utf8.push_back(static_cast<char>(src[i]));
            }
            src += 8;
            continue;
          }
        }

        // Process remaining characters
        while (src < end)
        {
          char16_t c = *src++;

          if (c < 0x80) // 1-byte sequence (ASCII)
          {
            utf8.push_back(static_cast<char>(c));
          }
          else if (c < 0x800) // 2-byte sequence
          {
            utf8.push_back(static_cast<char>(0xC0 | ((c >> 6) & 0x1F)));
            utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
          }
          else if (c >= 0xD800 && c <= 0xDBFF) // Surrogate pair
          {
            if (src >= end)
              throw soci_error("Invalid UTF-16 sequence (truncated surrogate pair)");

            char16_t c2 = *src++;
            if (c2 < 0xDC00 || c2 > 0xDFFF)
              throw soci_error("Invalid UTF-16 sequence (invalid surrogate pair)");

            uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
            utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
            utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
          }
          else // 3-byte sequence
          {
            utf8.push_back(static_cast<char>(0xE0 | ((c >> 12) & 0x0F)));
            utf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
          }
        }
      }

      return utf8;
    }

    /**
     * Converts a UTF-16 encoded string to a UTF-32 encoded string using SSE4.2 intrinsics.
     *
     * This function takes a UTF-16 encoded string as input and returns a UTF-32 encoded string as output.
     * It uses SSE4.2 intrinsics to optimize the conversion process. The function first checks if the input
     * string contains any surrogate pairs. If it does, the function converts each surrogate pair to a single
     * UTF-32 code point. If the input string does not contain any surrogate pairs, the function simply
     * reinterprets the input string as a UTF-32 string.
     *
     * @param utf16 The input UTF-16 encoded string to convert.
     * @return The output UTF-32 encoded string.
     * @throw soci_error If the input string contains an invalid UTF-16 sequence.
     */
    inline std::u32string utf16_to_utf32_sse42(const std::u16string &utf16)
    {
      std::u32string utf32;
      utf32.reserve(utf16.size());

      const char16_t *src = utf16.data();
      const char16_t *end = src + utf16.size();

      // Constants for surrogate detection
      const __m128i highSurrogateMask = _mm_set1_epi16(static_cast<int16_t>(0xFC00));
      const __m128i highSurrogateStart = _mm_set1_epi16(static_cast<int16_t>(0xD800));

      while (src < end)
      {
        size_t remaining = end - src;

        if (remaining >= 8)
        {
          // Load a chunk of 8 UTF-16 characters
          __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
          __m128i highSurrogate = _mm_and_si128(chunk, highSurrogateMask);
          __m128i isHighSurrogate = _mm_cmpeq_epi16(highSurrogate, highSurrogateStart);

          uint32_t bitfield = _mm_movemask_epi8(isHighSurrogate);

          if (bitfield == 0)
          {
            // No surrogates in the chunk, convert directly
            for (int i = 0; i < 8; ++i)
            {
              utf32.push_back(static_cast<char32_t>(src[i]));
            }
            src += 8;
            continue;
          }
        }

        // Process remaining characters or handle surrogates
        while (src < end)
        {
          char16_t c = *src++;

          if (c >= 0xD800 && c <= 0xDBFF) // High surrogate
          {
            if (src >= end)
              throw soci_error("Invalid UTF-16 sequence (truncated surrogate pair)");

            char16_t c2 = *src++;
            if (c2 < 0xDC00 || c2 > 0xDFFF)
              throw soci_error("Invalid UTF-16 sequence (invalid surrogate pair)");

            uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
            utf32.push_back(codepoint);
          }
          else if (c >= 0xDC00 && c <= 0xDFFF) // Low surrogate without preceding high surrogate
          {
            throw soci_error("Invalid UTF-16 sequence (lone low surrogate)");
          }
          else // Basic Multilingual Plane (BMP)
          {
            utf32.push_back(static_cast<char32_t>(c));
          }
        }
      }

      return utf32;
    }

    /**
     * Converts a UTF-32 encoded string to a UTF-16 encoded string using SSE4.2 intrinsics.
     *
     * This function takes a UTF-32 encoded string as input and converts it to a UTF-16 encoded string.
     * It uses SSE4.2 intrinsics to optimize the conversion process. The function checks if any of the
     * UTF-32 code points require surrogate pairs in UTF-16 encoding. If they do, the function converts
     * these code points to surrogate pairs. Otherwise, it directly converts the code points to UTF-16.
     *
     * @param utf32 The input UTF-32 encoded string.
     * @return The output UTF-16 encoded string.
     * @throw soci_error If the input string contains an invalid UTF-32 code point.
     */
    inline std::u16string utf32_to_utf16_sse42(const std::u32string &utf32)
    {
      std::u16string utf16;
      utf16.reserve(utf32.size() * 2);

      const char32_t *src = utf32.data();
      const char32_t *end = src + utf32.size();

      while (src < end)
      {
        size_t remaining = end - src;

        if (remaining >= 4) // Only process in chunks if enough data remains
        {
          // Validate next 4 UTF-32 codepoints
          for (int i = 0; i < 4; ++i)
          {
            char32_t codepoint = src[i];
            if (codepoint > 0x10FFFF)
            {
              throw soci_error("Invalid UTF-32 code point");
            }
          }

          // Load 4 UTF-32 codepoints
          __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
          __m128i cmp = _mm_cmpgt_epi32(chunk, _mm_set1_epi32(0xFFFF));

          // Determine if any codepoints are > 0xFFFF
          uint32_t bitfield = _mm_movemask_ps(_mm_castsi128_ps(cmp));

          if (bitfield == 0)
          {
            // No characters > 0xFFFF in the chunk
            // Directly convert to UTF-16 (all BMP characters)
            for (int i = 0; i < 4; ++i)
            {
              utf16.push_back(static_cast<char16_t>(src[i]));
            }
            src += 4;
          }
          else
          {
            // Characters > 0xFFFF present in the chunk
            for (int i = 0; i < 4; ++i)
            {
              char32_t codepoint = src[i];

              if (codepoint <= 0xFFFF)
              {
                utf16.push_back(static_cast<char16_t>(codepoint));
              }
              else
              {
                // Encode as surrogate pair
                if (codepoint <= 0x10FFFF)
                {
                  codepoint -= 0x10000;
                  utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
                  utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
                }
                else
                {
                  throw soci_error("Invalid UTF-32 code point");
                }
              }
            }
            src += 4;
          }
        }
        else // Process remaining characters individually
        {
          // This handles the trailing characters that don't form a full chunk
          while (src < end)
          {
            char32_t codepoint = *src++;

            if (codepoint <= 0xFFFF)
            {
              utf16.push_back(static_cast<char16_t>(codepoint));
            }
            else if (codepoint <= 0x10FFFF)
            {
              // Encode as surrogate pair
              codepoint -= 0x10000;
              utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
              utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
            }
            else
            {
              throw soci_error("Invalid UTF-32 code point");
            }
          }
        }
      }

      return utf16;
    }

    /**
     * Converts a UTF-32 encoded string to a UTF-8 encoded string using SSE4.2 intrinsics.
     *
     * This function takes a UTF-32 encoded string as input and converts it to a UTF-8 encoded string.
     * It uses SSE4.2 intrinsics to optimize the conversion process. The function processes the input
     * string in chunks and converts each chunk to its corresponding UTF-8 representation.
     *
     * @param utf32 The input UTF-32 encoded string.
     * @return The output UTF-8 encoded string.
     * @throw soci_error If the input string contains an invalid UTF-32 code point.
     */
    inline std::string utf32_to_utf8_sse42(const std::u32string &utf32)
    {
      std::string utf8;
      utf8.reserve(utf32.size() * 4);

      const uint32_t *src = reinterpret_cast<const uint32_t *>(utf32.data());
      const uint32_t *end = src + utf32.size();

      while (src < end)
      {
        size_t remaining = end - src;
        if (remaining >= 4)
        {
          // Load 4 UTF-32 code points
          __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));

          // Compare if all are ASCII (less than 0x80)
          __m128i thres = _mm_set1_epi32(0x80);
          __m128i mask = _mm_cmpgt_epi32(chunk, thres);
          int maskVal = _mm_movemask_ps(_mm_castsi128_ps(mask));

          if (maskVal == 0)
          {
            // All characters are ASCII, directly convert
            for (int i = 0; i < 4; ++i)
            {
              utf8.push_back(static_cast<char>(src[i]));
            }
            src += 4;
            continue;
          }
        }

        // Handle non-ASCII code points or remaining code points
        for (int i = 0; i < 4 && src < end; ++i, ++src)
        {
          uint32_t codepoint = *src;

          // Validate the code point
          if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
          {
            throw soci::soci_error("Invalid UTF-32 code point");
          }

          if (codepoint < 0x80)
          {
            // 1-byte sequence (ASCII)
            utf8.push_back(static_cast<char>(codepoint));
          }
          else if (codepoint < 0x800)
          {
            // 2-byte sequence
            utf8.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
            utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
          }
          else if (codepoint < 0x10000)
          {
            // 3-byte sequence
            utf8.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
            utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
          }
          else if (codepoint <= 0x10FFFF)
          {
            // 4-byte sequence
            utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
            utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
          }
        }
      }

      return utf8;
    }

    /**
     * Converts a UTF-8 encoded string to a UTF-32 encoded string using SSE4.2 intrinsics.
     *
     * This function takes a UTF-8 encoded string as input and converts it to a UTF-32 encoded string.
     * It uses SSE4.2 intrinsics to optimize the conversion process. The function processes the input
     * string in chunks and converts each chunk to its corresponding UTF-32 representation.
     *
     * @param utf8 The input UTF-8 encoded string.
     * @return The output UTF-32 encoded string.
     * @throw soci_error If the input string contains an invalid UTF-8 sequence.
     */
    inline std::u32string utf8_to_utf32_sse42(const std::string &utf8)
    {
      std::u32string utf32;
      utf32.reserve(utf8.size()); // Best case utf8.size() = utf32.size() when all characters are ASCII

      const char *src = utf8.data();
      const char *end = src + utf8.size();

      while (src < end)
      {
        size_t remaining = end - src;
        if (remaining >= 16)
        {
          // Load 16 bytes of data
          __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));

          // Mask to select ASCII range (0-127)
          __m128i mask = _mm_set1_epi8(static_cast<char>(0x80)); // 0x80 = 1000 0000 in binary
          __m128i test = _mm_cmpeq_epi8(_mm_and_si128(data, mask), _mm_setzero_si128());

          // Create a bit mask of 16 elements, ASCII characters have a zero upper bit
          int maskValue = _mm_movemask_epi8(test); // Extract the most significant bit of each byte

          if (maskValue == 0xFFFF)
          {
            // All characters are ASCII
            for (int i = 0; i < 16; ++i, ++src)
            {
              utf32.push_back(static_cast<char32_t>(*src));
            }
            continue;
          }
        }

        // Handle more complex sequences one byte at a time
        while (src < end)
        {
          unsigned char c1 = static_cast<unsigned char>(*src);
          if (c1 < 0x80)
          {
            // 1-byte sequence (ASCII)
            utf32.push_back(c1);
            src++;
          }
          else if ((c1 & 0xE0) == 0xC0)
          {
            // 2-byte sequence (110xxxxx 10xxxxxx)
            if (src + 1 >= end)
              throw soci_error("Invalid UTF-8 sequence (truncated 2-byte sequence)");

            unsigned char c2 = static_cast<unsigned char>(*(src + 1));
            if ((c2 & 0xC0) != 0x80)
              throw soci_error("Invalid UTF-8 sequence (invalid continuation byte in 2-byte sequence)");

            utf32.push_back(((c1 & 0x1F) << 6) | (c2 & 0x3F));
            src += 2;
          }
          else if ((c1 & 0xF0) == 0xE0)
          {
            // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
            if (src + 2 >= end)
              throw soci_error("Invalid UTF-8 sequence (truncated 3-byte sequence)");

            unsigned char c2 = static_cast<unsigned char>(*(src + 1));
            unsigned char c3 = static_cast<unsigned char>(*(src + 2));
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
              throw soci_error("Invalid UTF-8 sequence (invalid continuation bytes in 3-byte sequence)");

            utf32.push_back(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F));
            src += 3;
          }
          else if ((c1 & 0xF8) == 0xF0)
          {
            // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            if (src + 3 >= end)
              throw soci_error("Invalid UTF-8 sequence (truncated 4-byte sequence)");

            unsigned char c2 = static_cast<unsigned char>(*(src + 1));
            unsigned char c3 = static_cast<unsigned char>(*(src + 2));
            unsigned char c4 = static_cast<unsigned char>(*(src + 3));
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
              throw soci_error("Invalid UTF-8 sequence (invalid continuation bytes in 4-byte sequence)");

            utf32.push_back(((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F));
            src += 4;
          }
          else
          {
            throw soci_error("Invalid UTF-8 sequence");
          }

          // If next sequence possibly starts an ASCII run, break to check with SIMD first.
          if (src < end && static_cast<unsigned char>(*src) < 0x80)
            break;
        }
      }

      return utf32;
    }

#elif defined(SOCI_USE_NEON)

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string using NEON intrinsics.
     *
     * This function takes a UTF-8 encoded string as input and converts it to a UTF-16 encoded string.
     * It uses NEON intrinsics for efficient processing of the UTF-8 string. If the input UTF-8 string
     * is not well-formed, the function throws an exception with an appropriate error message.
     *
     * @param utf8 The input UTF-8 encoded string.
     * @return std::u16string The output UTF-16 encoded string.
     * @throws soci_error If the input UTF-8 string is not well-formed.
     */
    inline std::u16string utf8_to_utf16_neon(const std::string &utf8)
    {
      std::u16string utf16;
      utf16.reserve(utf8.size());

      const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
      std::size_t length = utf8.length();

      for (std::size_t i = 0; i < length;)
      {
        if (length - i >= 16)
        {
          uint8x16_t chunk = vld1q_u8(bytes + i);
          uint8x16_t mask = vdupq_n_u8(0x80);
          uint8x16_t result = vceqq_u8(vandq_u8(chunk, mask), vdupq_n_u8(0));
          uint64_t bitfield_lo = vgetq_lane_u64(vreinterpretq_u64_u8(result), 0);
          uint64_t bitfield_hi = vgetq_lane_u64(vreinterpretq_u64_u8(result), 1);
          uint64_t bitfield = bitfield_lo | (bitfield_hi << 8);

          if (bitfield == 0xFFFFFFFFFFFFFFFF)
          {
            // All characters in the chunk are ASCII
            for (int j = 0; j < 16; ++j)
            {
              utf16.push_back(static_cast<char16_t>(bytes[i + j]));
            }
            i += 16;
          }
          else
          {
            // Handle non-ASCII characters with NEON
            // ... (NEON specific code)
            // For simplicity, let's assume we handle the non-ASCII part here and then call the common function
            utf8_to_utf16_common(bytes + i, length - i, utf16);
            break;
          }
        }
        else
        {
          utf8_to_utf16_common(bytes + i, length - i, utf16);
          break;
        }
      }

      return utf16;
    }
    
    
    
    
    // inline std::u16string utf8_to_utf16_neon(const std::string &utf8)
    // {
    //   std::u16string utf16;
    //   utf16.reserve(utf8.size());

    //   const char *src = utf8.data();
    //   const char *end = src + utf8.size();

    //   while (src < end)
    //   {
    //     size_t bytes_remaining = end - src;
    //     size_t chunk_size = bytes_remaining < 16 ? bytes_remaining : 16;

    //     uint8x16_t chunk = vld1q_u8(reinterpret_cast<const uint8_t *>(src));
    //     uint8x16_t mask = vdupq_n_u8(0x80);
    //     uint8x16_t result = vceqq_u8(vandq_u8(chunk, mask), vdupq_n_u8(0));

    //     uint64_t bitfield_lo = vgetq_lane_u64(vreinterpretq_u64_u8(result), 0);
    //     uint64_t bitfield_hi = vgetq_lane_u64(vreinterpretq_u64_u8(result), 1);
    //     uint64_t bitfield = bitfield_lo | (bitfield_hi << 8);

    //     if (bitfield == 0xFFFFFFFFFFFFFFFF)
    //     {
    //       for (size_t i = 0; i < chunk_size; ++i)
    //       {
    //         utf16.push_back(static_cast<char16_t>(src[i]));
    //       }
    //       src += chunk_size;
    //     }
    //     else
    //     {
    //       size_t i = 0;
    //       while (i < chunk_size)
    //       {
    //         unsigned char c1 = static_cast<unsigned char>(src[i]);

    //         if (c1 < 0x80)
    //         {
    //           utf16.push_back(static_cast<char16_t>(c1));
    //           ++i;
    //         }
    //         else if ((c1 & 0xE0) == 0xC0)
    //         {
    //           if (i + 1 >= chunk_size)
    //             break;
    //           unsigned char c2 = static_cast<unsigned char>(src[i + 1]);
    //           if ((c2 & 0xC0) != 0x80)
    //             throw soci_error("Invalid UTF-8 sequence");

    //           utf16.push_back(static_cast<char16_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F)));
    //           i += 2;
    //         }
    //         else if ((c1 & 0xF0) == 0xE0)
    //         {
    //           if (i + 2 >= chunk_size)
    //             break;
    //           unsigned char c2 = static_cast<unsigned char>(src[i + 1]);
    //           unsigned char c3 = static_cast<unsigned char>(src[i + 2]);
    //           if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80))
    //             throw soci_error("Invalid UTF-8 sequence");

    //           utf16.push_back(static_cast<char16_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)));
    //           i += 3;
    //         }
    //         else if ((c1 & 0xF8) == 0xF0)
    //         {
    //           if (i + 3 >= chunk_size)
    //             break;
    //           unsigned char c2 = static_cast<unsigned char>(src[i + 1]);
    //           unsigned char c3 = static_cast<unsigned char>(src[i + 2]);
    //           unsigned char c4 = static_cast<unsigned char>(src[i + 3]);
    //           if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80) || ((c4 & 0xC0) != 0x80))
    //             throw soci_error("Invalid UTF-8 sequence");

    //           uint32_t codepoint = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
    //           if (codepoint <= 0xFFFF)
    //           {
    //             utf16.push_back(static_cast<char16_t>(codepoint));
    //           }
    //           else
    //           {
    //             codepoint -= 0x10000;
    //             utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
    //             utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
    //           }
    //           i += 4;
    //         }
    //         else
    //         {
    //           throw soci_error("Invalid UTF-8 sequence");
    //         }
    //       }
    //       src += i;
    //     }
    //   }

    //   return utf16;
    // }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string using NEON intrinsics.
     *
     * This function takes a UTF-16 encoded string as input and converts it to a UTF-8 encoded string.
     * It uses NEON intrinsics to optimize the conversion process for ARM architectures.
     *
     * @param utf16 The input UTF-16 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throw soci_error If the input UTF-16 string is invalid.
     */
    inline std::string utf16_to_utf8_neon(const std::u16string &utf16)
    {
      std::string utf8;
      utf8.reserve(utf16.size() * 3); // Reserve space for the UTF-8 string to avoid reallocations

      const char16_t *src = utf16.data();
      const char16_t *end = src + utf16.size();

      while (src < end)
      {
        size_t remaining = end - src;
        size_t chunk_size = remaining < 8 ? remaining : 8;

        // Load up to 8 UTF-16 characters into a NEON register
        uint16x8_t chunk = vld1q_u16(reinterpret_cast<const uint16_t *>(src));

        // Create a mask to identify ASCII characters
        uint16x8_t ascii_mask = vcleq_u16(chunk, vdupq_n_u16(0x7F));

        // Check if all characters in the chunk are ASCII
        uint64_t ascii_bitfield = vgetq_lane_u64(vreinterpretq_u64_u16(ascii_mask), 0);
        if (ascii_bitfield == 0xFFFFFFFFFFFFFFFF)
        {
          // Handle up to 8 ASCII characters
          for (size_t i = 0; i < chunk_size; i++)
          {
            utf8.push_back(static_cast<char>(src[i]));
          }
          src += chunk_size;
        }
        else
        {
          // Handle non-ASCII characters with NEON and scalar fallbacks as needed.
          for (size_t i = 0; i < chunk_size; i++)
          {
            char16_t c = src[i];

            if (c < 0x80)
            {
              utf8.push_back(static_cast<char>(c));
            }
            else if (c < 0x800)
            {
              utf8.push_back(static_cast<char>(0xC0 | ((c >> 6) & 0x1F)));
              utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
            }
            else if ((c >= 0xD800) && (c <= 0xDBFF))
            {
              // Handle UTF-16 surrogate pairs
              if (i + 1 >= chunk_size || src + i + 1 >= end)
                throw soci::soci_error("Invalid UTF-16 sequence");

              char16_t c2 = src[i + 1];
              if ((c2 < 0xDC00) || (c2 > 0xDFFF))
                throw soci::soci_error("Invalid UTF-16 sequence");

              uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
              utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
              utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
              utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
              utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
              i++; // Skip the next character as it is part of the surrogate pair
            }
            else
            {
              utf8.push_back(static_cast<char>(0xE0 | ((c >> 12) & 0x0F)));
              utf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
              utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
            }
          }
          src += chunk_size;
        }
      }

      return utf8;
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-32 encoded string.
     *
     * This function uses NEON intrinsics for ARMv8 processors to optimize the conversion process.
     * It checks if the input UTF-16 string is valid before performing the conversion.
     * If the input string is not valid UTF-16, it throws a soci_error exception.
     *
     * @param utf16 The input UTF-16 encoded string to be converted.
     * @return std::u32string The converted UTF-32 encoded string.
     * @throws soci_error If the input UTF-16 string is not valid.
     */
    inline std::u32string utf16_to_utf32_neon(const std::u16string &utf16)
    {
      // Check if the input UTF-16 string is valid
      if (!is_valid_utf16(utf16))
      {
        throw soci_error("Invalid UTF-16 string");
      }

      std::u32string utf32;
      utf32.reserve(utf16.size());

      const char16_t *src = utf16.data();
      const char16_t *end = src + utf16.size();

      while (src < end)
      {
        // Load 8 UTF-16 characters into a NEON register
        uint16x8_t chunk = vld1q_u16(reinterpret_cast<const uint16_t *>(src));

        // Check for high surrogates (values between 0xD800 and 0xDBFF)
        uint16x8_t highSurrogate = vandq_u16(chunk, vdupq_n_u16(0xFC00));
        uint16x8_t isSurrogate = vceqq_u16(highSurrogate, vdupq_n_u16(0xD800));

        // Convert the NEON register to a 64-bit integer for easier bitwise operations
        uint64_t bitfield = vgetq_lane_u64(vreinterpretq_u64_u16(isSurrogate), 0);

        if (bitfield == 0)
        {
          // No surrogates in the chunk, so we can directly convert the UTF-16 characters to UTF-32
          for (int i = 0; i < 8 && src < end; ++i)
          {
            utf32.push_back(static_cast<char32_t>(*src));
            src++;
          }
        }
        else
        {
          // Surrogates present in the chunk, so we need to handle them separately
          while (src < end)
          {
            char16_t c = *src++;

            if ((c >= 0xD800) && (c <= 0xDBFF))
            {
              // This is a high surrogate, so we need to combine it with a low surrogate to form a UTF-32 character
              if (src >= end)
                throw soci_error("Invalid UTF-16 sequence");

              char16_t c2 = *src++;
              if ((c2 < 0xDC00) || (c2 > 0xDFFF))
                throw soci_error("Invalid UTF-16 sequence");

              // Combine the high and low surrogates to form a UTF-32 character
              uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
              utf32.push_back(codepoint);
            }
            else
            {
              // This is not a surrogate, so we can directly convert it to UTF-32
              utf32.push_back(static_cast<char32_t>(c));
            }
          }
        }
      }

      return utf32;
    }

    /**
     * Converts a UTF-32 encoded string to a UTF-16 encoded string using NEON intrinsics.
     *
     * This function takes a UTF-32 encoded string as input and converts it to a UTF-16 encoded string.
     * It uses the NEON intrinsics for ARM architecture to optimize the conversion process.
     *
     * @param utf32 The input UTF-32 encoded string.
     * @return The converted UTF-16 encoded string.
     * @throws soci_error If the input UTF-32 string contains invalid code points.
     */
    inline std::u16string utf32_to_utf16_neon(const std::u32string &utf32)
    {
      // Check if the input UTF-32 string contains only valid code points
      if (!is_valid_utf32(utf32))
      {
        throw soci_error("Invalid UTF-32 string");
      }

      std::u16string utf16;
      utf16.reserve(utf32.size() * 2); // Maximum possible size (all surrogate pairs)

      const uint32_t *src = reinterpret_cast<const uint32_t *>(utf32.data());
      const uint32_t *end = src + utf32.size();

      while (src < end)
      {
        size_t remaining = end - src;
        size_t chunk_size = (remaining < 4 ? remaining : 4);

        // Load up to 4 UTF-32 code points into a NEON register
        uint32x4_t chunk = vld1q_u32(src);

        // Compare each element with 0xFFFF using a greater than operation
        uint32x4_t mask = vdupq_n_u32(0xFFFF);
        uint32x4_t results = vcgtq_u32(chunk, mask);

        // Combine the results to determine if any element is greater than 0xFFFF
        uint64x2_t packed = vreinterpretq_u64_u32(results);
        uint64_t combined = vgetq_lane_u64(packed, 0) | vgetq_lane_u64(packed, 1);

        if (combined == 0)
        { // No characters > 0xFFFF
          // Directly pack and store characters
          uint32_t temp[4];
          vst1q_u32(temp, chunk);
          for (size_t i = 0; i < chunk_size; ++i)
          {
            utf16.push_back(static_cast<char16_t>(temp[i]));
          }
          src += chunk_size;
        }
        else
        {
          // Process characters individually due to potential surrogate pairs
          uint32_t temp[4];
          vst1q_u32(temp, chunk);
          for (size_t i = 0; i < chunk_size; ++i, ++src)
          {
            uint32_t codepoint = temp[i];
            if (codepoint <= 0xFFFF)
            {
              // BMP character
              utf16.push_back(static_cast<char16_t>(codepoint));
            }
            else if (codepoint <= 0x10FFFF)
            {
              // Surrogate pair needed
              codepoint -= 0x10000;
              utf16.push_back(static_cast<char16_t>((codepoint >> 10) | 0xD800));
              utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) | 0xDC00));
            }
            else
            {
              throw soci_error("Invalid UTF-32 code point");
            }
          }
        }
      }
      return utf16;
    }

    /**
     * Converts a UTF-8 encoded string to a UTF-32 encoded string using NEON intrinsics.
     *
     * @param utf8 The input UTF-8 encoded string.
     * @return The converted UTF-32 encoded string.
     * @throws soci_error If the input UTF-8 string is invalid or contains an invalid UTF-8 sequence.
     */
    inline std::u32string utf8_to_utf32_neon(const std::string &utf8)
    {
      // Check if the input UTF-8 string is valid
      if (!is_valid_utf8(utf8))
      {
        throw soci_error("Invalid UTF-8 string");
      }

      // Initialize an empty UTF-32 string
      std::u32string utf32;
      utf32.reserve(utf8.size()); // Reserve space to minimize reallocations

      const char *src = utf8.data();
      const char *end = src + utf8.size();

      while (src < end)
      {
        size_t remaining = end - src;
        size_t chunk_size = remaining < 16 ? remaining : 16;

        // Load up to 16 bytes into a NEON register
        uint8x16_t chunk = vld1q_u8(reinterpret_cast<const uint8_t *>(src));

        // Create a mask to check for ASCII characters
        uint8x16_t ascii_mask = vceqq_u8(vandq_u8(chunk, vdupq_n_u8(0x80)), vdupq_n_u8(0x00));
        uint64_t mask_low = vgetq_lane_u64(vreinterpretq_u64_u8(ascii_mask), 0);
        uint64_t mask_high = vgetq_lane_u64(vreinterpretq_u64_u8(ascii_mask), 1);
        uint64_t bitfield = mask_low | (mask_high << 8);

        // If all characters in the chunk are ASCII, convert them directly to UTF-32
        if ((bitfield & 0xFFFFFFFFFFFFFFFF) == 0xFFFFFFFFFFFFFFFF)
        {
          for (size_t i = 0; i < chunk_size; ++i)
          {
            utf32.push_back(static_cast<char32_t>(src[i]));
          }
          src += chunk_size;
        }
        else
        {
          while (src < end)
          {
            unsigned char c1 = static_cast<unsigned char>(*src++);

            if (c1 < 0x80)
            {
              utf32.push_back(static_cast<char32_t>(c1));
            }
            else if ((c1 & 0xE0) == 0xC0)
            {
              // Two-byte UTF-8 character
              if (src >= end)
                throw soci_error("Invalid UTF-8 sequence");

              unsigned char c2 = static_cast<unsigned char>(*src++);
              if ((c2 & 0xC0) != 0x80)
                throw soci_error("Invalid UTF-8 sequence");

              utf32.push_back(static_cast<char32_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F)));
            }
            else if ((c1 & 0xF0) == 0xE0)
            {
              // Three-byte UTF-8 character
              if (src + 1 >= end)
                throw soci_error("Invalid UTF-8 sequence");

              unsigned char c2 = static_cast<unsigned char>(*src++);
              unsigned char c3 = static_cast<unsigned char>(*src++);
              if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80))
                throw soci_error("Invalid UTF-8 sequence");

              utf32.push_back(static_cast<char32_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)));
            }
            else if ((c1 & 0xF8) == 0xF0)
            {
              // Four-byte UTF-8 character
              if (src + 2 >= end)
                throw soci_error("Invalid UTF-8 sequence");

              unsigned char c2 = static_cast<unsigned char>(*src++);
              unsigned char c3 = static_cast<unsigned char>(*src++);
              unsigned char c4 = static_cast<unsigned char>(*src++);
              if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80) || ((c4 & 0xC0) != 0x80))
                throw soci_error("Invalid UTF-8 sequence");

              utf32.push_back(static_cast<char32_t>(((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)));
            }
            else
            {
              throw soci_error("Invalid UTF-8 sequence");
            }
          }
        }
      }

      return utf32;
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-8 encoded string using NEON intrinsics.
     *
     * This function takes a UTF-32 encoded string as input and converts it to a UTF-8 encoded string.
     * It uses NEON intrinsics for efficient processing of UTF-32 data.
     *
     * @param utf32 The input UTF-32 encoded string.
     * @return std::string The output UTF-8 encoded string.
     * @throw soci_error If the input UTF-32 string contains invalid code points.
     */
    inline std::string utf32_to_utf8_neon(const std::u32string &utf32)
    {
      // Initialize the output UTF-8 string
      std::string utf8;
      // Reserve enough space for the output string to avoid reallocations
      utf8.reserve(utf32.size() * 4);

      // Get pointers to the start and end of the input UTF-32 string
      auto *src = reinterpret_cast<const uint32_t *>(utf32.data());
      auto *end = src + utf32.size();

      // Process the input UTF-32 string in chunks of 4 code points
      while (src < end)
      {
        // Load 4 UTF-32 code points into a NEON register
        uint32x4_t chunk = vld1q_u32(src);
        // Set a threshold value to determine if a code point is ASCII or not
        uint32_t thres = 0x80;

        // Compare each code point in the chunk with the threshold value
        uint32x4_t mask = vcgtq_u32(chunk, vdupq_n_u32(thres));

        // Combine the results of the comparison into a single 64-bit value
        uint64x2_t result = vreinterpretq_u64_u32(mask);
        uint64_t combined = vgetq_lane_u64(result, 0) | vgetq_lane_u64(result, 1);

        // If all code points in the chunk are ASCII, process them en-bloc
        if (combined == 0)
        {
          for (int i = 0; i < 4 && src < end; ++i)
          {
            uint32_t codepoint = *src;
            if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
            {
              throw soci::soci_error("Invalid UTF-32 code point");
            }
            utf8.push_back(static_cast<char>(codepoint));
            src++;
          }
        }
        // Otherwise, process each code point individually
        else
        {
          for (int i = 0; i < 4 && src < end; ++i, ++src)
          {
            uint32_t codepoint = *src;
            if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
            {
              throw soci::soci_error("Invalid UTF-32 code point");
            }
            // Convert the code point to UTF-8 and append it to the output string
            if (codepoint < 0x80)
            {
              utf8.push_back(static_cast<char>(codepoint));
            }
            else if (codepoint < 0x800)
            {
              utf8.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
              utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
            else if (codepoint < 0x10000)
            {
              utf8.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
              utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
              utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
            else if (codepoint <= 0x10FFFF)
            {
              utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
              utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
              utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
              utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
          }
        }
      }

      // Return the output UTF-8 string
      return utf8;
    }

// #else
#endif

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
     *
     * This function iterates through the input string and converts each UTF-8 sequence into
     * its corresponding UTF-16 code unit(s).
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    inline std::u16string utf8_to_utf16_fallback(const std::string &utf8)
    {
      std::u16string utf16;
      utf16.reserve(utf8.size());

      const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
      std::size_t length = utf8.length();

      utf8_to_utf16_common(bytes, length, utf16);

      return utf16;
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
     *
     * This function iterates through the input string and converts each UTF-16 code unit into
     * its corresponding UTF-8 sequence(s).
     * If the input string contains invalid UTF-16 encoding, a soci_error exception is thrown.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    inline std::string utf16_to_utf8_fallback(const std::u16string &utf16)
    {
      // If valid UTF-16 validation is required before conversion:
      if (!is_valid_utf16(utf16))
      {
        throw soci_error("Invalid UTF-16 string");
      }

      std::string utf8;
      utf8.reserve(utf16.size() * 3);

      for (std::size_t i = 0; i < utf16.size();)
      {
        char16_t c = utf16[i++];

        if (c < 0x80)
        {
          // 1-byte sequence (ASCII)
          utf8.push_back(static_cast<char>(c));
        }
        else if (c < 0x800)
        {
          // 2-byte sequence
          utf8.push_back(static_cast<char>(0xC0 | ((c >> 6) & 0x1F)));
          utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
        else if (c >= 0xD800 && c <= 0xDBFF)
        {
          // Surrogate pair
          if (i >= utf16.size())
          {
            throw soci_error("Invalid UTF-16 sequence (truncated surrogate pair)");
          }

          char16_t c2 = utf16[i++];
          if (c2 < 0xDC00 || c2 > 0xDFFF)
          {
            throw soci_error("Invalid UTF-16 sequence (invalid surrogate pair)");
          }

          uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
          utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (c >= 0xDC00 && c <= 0xDFFF)
        {
          // Lone lower surrogate, not valid by itself
          throw soci_error("Invalid UTF-16 sequence (lone low surrogate)");
        }
        else
        {
          // 3-byte sequence
          utf8.push_back(static_cast<char>(0xE0 | ((c >> 12) & 0x0F)));
          utf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
      }

      return utf8;
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-32 encoded string.
     *
     * This function iterates through the input string and converts each UTF-16 code unit into
     * its corresponding UTF-32 code point(s).
     * If the input string contains invalid UTF-16 encoding, a soci_error exception is thrown.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    inline std::u32string utf16_to_utf32_fallback(const std::u16string &utf16)
    {
      // Validate UTF-16 string if necessary
      if (!is_valid_utf16(utf16))
      {
        throw soci_error("Invalid UTF-16 string");
      }

      std::u32string utf32;
      utf32.reserve(utf16.size());

      for (std::size_t i = 0; i < utf16.size();)
      {
        char16_t c = utf16[i++];

        if (c >= 0xD800 && c <= 0xDBFF)
        {
          // High surrogate, must be followed by a low surrogate
          if (i >= utf16.size())
            throw soci_error("Invalid UTF-16 sequence (truncated surrogate pair)");

          char16_t c2 = utf16[i++];
          if (c2 < 0xDC00 || c2 > 0xDFFF)
            throw soci_error("Invalid UTF-16 sequence (invalid surrogate pair)");

          uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
          utf32.push_back(codepoint);
        }
        else if (c >= 0xDC00 && c <= 0xDFFF)
        {
          // Low surrogate without preceding high surrogate
          throw soci_error("Invalid UTF-16 sequence (lone low surrogate)");
        }
        else
        {
          // Valid BMP character
          utf32.push_back(static_cast<char32_t>(c));
        }
      }

      return utf32;
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-16 encoded string.
     *
     * This function iterates through the input string and converts each UTF-32 code point into
     * its corresponding UTF-16 code unit(s).
     * If the input string contains invalid UTF-32 code points, a soci_error exception is thrown.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    inline std::u16string utf32_to_utf16_fallback(const std::u32string &utf32)
    {
      // Validate UTF-32 string if necessary
      if (!is_valid_utf32(utf32))
      {
        throw soci_error("Invalid UTF-32 string");
      }

      std::u16string utf16;
      utf16.reserve(utf32.size() * 2); // Maximum possible size (all surrogate pairs)

      for (char32_t codepoint : utf32)
      {
        if (codepoint <= 0xFFFF)
        {
          // BMP character
          utf16.push_back(static_cast<char16_t>(codepoint));
        }
        else if (codepoint <= 0x10FFFF)
        {
          // Convert to surrogate pair
          codepoint -= 0x10000;
          utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
          utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
        }
        else
        {
          // Invalid Unicode range
          throw soci_error("Invalid UTF-32 code point: out of Unicode range");
        }
      }

      return utf16;
    }

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-32 encoded string.
     *
     * This function iterates through the input string and converts each UTF-8 sequence into
     * its corresponding UTF-32 code point(s).
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    inline std::u32string utf8_to_utf32_fallback(const std::string &utf8)
    {
      // Validate UTF-8 string if necessary
      if (!is_valid_utf8(utf8))
      {
        throw soci_error("Invalid UTF-8 string");
      }

      std::u32string utf32;
      utf32.reserve(utf8.size());

      for (std::size_t i = 0; i < utf8.size();)
      {
        unsigned char c1 = static_cast<unsigned char>(utf8[i++]);

        if (c1 < 0x80)
        {
          // 1-byte sequence (ASCII)
          utf32.push_back(static_cast<char32_t>(c1));
        }
        else if ((c1 & 0xE0) == 0xC0)
        {
          // 2-byte sequence
          if (i >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence (truncated 2-byte sequence)");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          if ((c2 & 0xC0) != 0x80)
            throw soci_error("Invalid UTF-8 sequence (invalid continuation byte in 2-byte sequence)");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F)));
        }
        else if ((c1 & 0xF0) == 0xE0)
        {
          // 3-byte sequence
          if (i + 1 >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence (truncated 3-byte sequence)");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c3 = static_cast<unsigned char>(utf8[i++]);
          if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
            throw soci_error("Invalid UTF-8 sequence (invalid continuation bytes in 3-byte sequence)");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)));
        }
        else if ((c1 & 0xF8) == 0xF0)
        {
          // 4-byte sequence
          if (i + 2 >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence (truncated 4-byte sequence)");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c3 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c4 = static_cast<unsigned char>(utf8[i++]);

          if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
            throw soci_error("Invalid UTF-8 sequence (invalid continuation bytes in 4-byte sequence)");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)));
        }
        else
        {
          // Invalid UTF-8 start byte
          throw soci_error("Invalid UTF-8 sequence (invalid start byte)");
        }
      }

      return utf32;
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-8 encoded string.
     *
     * This function iterates through the input string and converts each UTF-32 code point into
     * its corresponding UTF-8 sequence(s).
     * If the input string contains invalid UTF-32 code points, a soci_error exception is thrown.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    inline std::string utf32_to_utf8_fallback(const std::u32string &utf32)
    {
      // Validate the UTF-32 string if necessary
      if (!is_valid_utf32(utf32))
      {
        throw soci_error("Invalid UTF-32 string");
      }

      std::string utf8;
      utf8.reserve(utf32.size() * 4); // Reserve enough space to avoid reallocations

      for (char32_t codepoint : utf32)
      {
        if (codepoint < 0x80)
        {
          // 1-byte sequence (ASCII)
          utf8.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint < 0x800)
        {
          // 2-byte sequence
          utf8.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint < 0x10000)
        {
          // 3-byte sequence (BMP characters)
          utf8.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint <= 0x10FFFF)
        {
          // 4-byte sequence (Supplementary characters)
          utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else
        {
          // Invalid code point
          throw soci_error("Invalid UTF-32 code point: out of Unicode range");
        }
      }

      return utf8;
    }

    // #endif

    /* Wrappers for the conversion functions that select the appropriate implementation based on
    platform capabilities */

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    inline std::u16string utf8_to_utf16(const std::string &utf8)
    {
#if defined(SOCI_USE_SSE_4_2)
      return utf8_to_utf16_sse42(utf8);
#elif defined(SOCI_USE_NEON) // && (0)
      return utf8_to_utf16_neon(utf8);
#else
      return utf8_to_utf16_fallback(utf8);
#endif
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-16 encoding, a soci_error exception is thrown.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    inline std::string utf16_to_utf8(const std::u16string &utf16)
    {
#if defined(SOCI_USE_SSE_4_2)
      return utf16_to_utf8_sse42(utf16);
#elif defined(SOCI_USE_NEON) //&& (0)
      return utf16_to_utf8_neon(utf16);
#else
      return utf16_to_utf8_fallback(utf16);
#endif
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-32 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-16 encoding, a soci_error exception is thrown.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    inline std::u32string utf16_to_utf32(const std::u16string &utf16)
    {
#if defined(SOCI_USE_SSE_4_2)
      return utf16_to_utf32_sse42(utf16);
#elif defined(SOCI_USE_NEON)
      return utf16_to_utf32_neon(utf16);
#else
      return utf16_to_utf32_fallback(utf16);
#endif
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-16 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-32 code points, a soci_error exception is thrown.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    inline std::u16string utf32_to_utf16(const std::u32string &utf32)
    {
#if defined(SOCI_USE_SSE_4_2)
      return utf32_to_utf16_sse42(utf32);
#elif defined(SOCI_USE_NEON)
      return utf32_to_utf16_neon(utf32);
#else
      return utf32_to_utf16_fallback(utf32);
#endif
    }

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-32 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    inline std::u32string utf8_to_utf32(const std::string &utf8)
    {
#if defined(SOCI_USE_SSE_4_2)
      return utf8_to_utf32_sse42(utf8);
#elif defined(SOCI_USE_NEON)
      return utf8_to_utf32_neon(utf8);
#else
      return utf8_to_utf32_fallback(utf8);
#endif
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-8 encoded string.
     *
     * This function uses SIMD instructions if available, otherwise it falls back to a standard
     * implementation.
     * If the input string contains invalid UTF-32 code points, a soci_error exception is thrown.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    inline std::string utf32_to_utf8(const std::u32string &utf32)
    {
#if defined(SOCI_USE_SSE_4_2)
      return utf32_to_utf8_sse42(utf32);
#elif defined(SOCI_USE_NEON)
      return utf32_to_utf8_neon(utf32);
#else
      return utf32_to_utf8_fallback(utf32);
#endif
    }

    /**
     * @brief Converts a UTF-8 encoded string to a wide string (wstring).
     *
     * This function uses the platform's native wide character encoding. On Windows, this is UTF-16,
     * while on Unix/Linux and other platforms, it is UTF-32 or UTF-8 depending on the system
     * configuration.
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::wstring The wide string.
     */
    inline std::wstring utf8_to_wide(const std::string &utf8)
    {
#if defined(SOCI_WCHAR_T_IS_WIDE) // Windows
      // Convert UTF-8 to UTF-32 first and then to wstring (UTF-32 on Unix/Linux)
      std::u32string utf32 = utf8_to_utf32(utf8);
      return std::wstring(utf32.begin(), utf32.end());
#else  // Unix/Linux and others
      std::u16string utf16 = utf8_to_utf16(utf8);
      return std::wstring(utf16.begin(), utf16.end());
#endif // SOCI_WCHAR_T_IS_WIDE
    }

    /**
     * @brief Converts a wide string (wstring) to a UTF-8 encoded string.
     *
     * This function uses the platform's native wide character encoding. On Windows, this is UTF-16,
     * while on Unix/Linux and other platforms, it is UTF-32 or UTF-8 depending on the system
     * configuration.
     * If the input string contains invalid wide characters, a soci_error exception is thrown.
     *
     * @param wide The wide string.
     * @return std::string The UTF-8 encoded string.
     */
    inline std::string wide_to_utf8(const std::wstring &wide)
    {
#if defined(SOCI_WCHAR_T_IS_WIDE) // Windows
      // Convert wstring (UTF-32) to utf8
      std::u32string utf32(wide.begin(), wide.end());
      return utf32_to_utf8(utf32);
#else  // Unix/Linux and others
      std::u16string utf16(wide.begin(), wide.end());
      return utf16_to_utf8(utf16);
#endif // SOCI_WCHAR_T_IS_WIDE
    }

  } // namespace details

} // namespace soci

#endif // SOCI_UNICODE_H_INCLUDED
