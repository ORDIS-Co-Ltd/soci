#ifndef SOCI_UNICODE_H_INCLUDED
#define SOCI_UNICODE_H_INCLUDED

#include "soci/error.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <wchar.h>

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
     * @brief Check if a given UTF-32 string is valid.
     *
     * This function checks whether all code points in the input
     * UTF-32 string are within the Unicode range (0x0 to 0x10FFFF) and
     * do not fall into the surrogate pair range (0xD800 to 0xDFFF).
     *
     * @param utf32 The input UTF-32 string.
     * @return True if the input string is valid, false otherwise.
     */
    inline bool is_valid_utf32(const std::u32string &utf32)
    {
      const char32_t *chars = utf32.data();
      std::size_t length = utf32.length();

      for (std::size_t i = 0; i < length; ++i)
      {
        char32_t c = chars[i];

        // Check if the code point is within the Unicode range
        if (c > 0x10FFFF)
        {
          return false;
        }

        // Surrogate pairs are not valid in UTF-32
        if (c >= 0xD800 && c <= 0xDFFF)
        {
          return false;
        }
      }

      return true;
    }

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
     *
     * This function iterates over the input UTF-8 encoded string and converts each character to its corresponding UTF-16 representation.
     * It handles ASCII characters, two-byte, three-byte, and four-byte UTF-8 sequences. For four-byte sequences that represent codepoints
     * greater than U+10FFFF, it throws an exception. For codepoints greater than U+FFFF, it encodes them as a surrogate pair.
     *
     * @param bytes The input UTF-8 encoded string.
     * @param length The length of the input string.
     * @param utf16 The output UTF-16 encoded string.
     * @throw soci_error If the input string contains an invalid UTF-8 sequence or an invalid UTF-8 codepoint.
     */
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

    /**
     * @brief Convert a UTF-16 encoded string to UTF-8.
     *
     * This function takes a UTF-16 encoded string and its length, and appends the UTF-8
     * representation of the string to the provided std::string. It handles UTF-16 surrogate
     * pairs correctly.
     *
     * @param chars The UTF-16 encoded string to convert.
     * @param length The length of the UTF-16 encoded string.
     * @param utf8 The std::string to which the UTF-8 representation of the input string
     *             will be appended.
     * @throw soci_error If the input UTF-16 string is not valid.
     */
    inline void utf16_to_utf8_common(const char16_t *chars, std::size_t length, std::string &utf8)
    {
      for (std::size_t i = 0; i < length; ++i)
      {
        char16_t c = chars[i];

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
        else if ((c >= 0xD800) && (c <= 0xDBFF))
        {
          // Handle UTF-16 surrogate pairs
          if (i + 1 >= length)
            throw soci_error("Invalid UTF-16 sequence");

          char16_t c2 = chars[i + 1];
          if ((c2 < 0xDC00) || (c2 > 0xDFFF))
            throw soci_error("Invalid UTF-16 sequence");

          uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
          utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
          ++i; // Skip the next character as it is part of the surrogate pair
        }
        else if ((c >= 0xDC00) && (c <= 0xDFFF))
        {
          // Lone low surrogate, not valid by itself
          throw soci_error("Invalid UTF-16 sequence");
        }
        else
        {
          // 3-byte sequence
          utf8.push_back(static_cast<char>(0xE0 | ((c >> 12) & 0x0F)));
          utf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
      }
    }

    /**
     * @brief Converts a UTF-16 string to a UTF-32 string.
     *
     * This function iterates over the input UTF-16 string and converts each character to UTF-32.
     * It handles surrogate pairs correctly. If the input string contains invalid UTF-16 sequences,
     * such as a lone low surrogate or a truncated surrogate pair, the function throws a soci_error exception.
     *
     * @param chars Pointer to the first character of the input UTF-16 string.
     * @param length Length of the input UTF-16 string.
     * @param utf32 Reference to the output UTF-32 string.
     * @throw soci_error If the input string contains invalid UTF-16 sequences.
     */
    inline void utf16_to_utf32_common(const char16_t *chars, std::size_t length, std::u32string &utf32)
    {
      for (std::size_t i = 0; i < length; ++i)
      {
        char16_t c = chars[i];

        if (c >= 0xD800 && c <= 0xDBFF)
        {
          // High surrogate, must be followed by a low surrogate
          if (i + 1 >= length)
            throw soci_error("Invalid UTF-16 sequence (truncated surrogate pair)");

          char16_t c2 = chars[i + 1];
          if (c2 < 0xDC00 || c2 > 0xDFFF)
            throw soci_error("Invalid UTF-16 sequence (invalid surrogate pair)");

          uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
          utf32.push_back(codepoint);
          ++i; // Skip the next character as it is part of the surrogate pair
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
    }

    /**
     * @brief Converts a sequence of UTF-32 encoded code points into a UTF-16 encoded string.
     *
     * This function iterates over each code point in the input sequence, checks if it's within the Basic Multilingual Plane (BMP) or not, and encodes it accordingly. If the code point is within the BMP, it's directly cast to a char16_t and appended to the output string. If it's outside the BMP but within the valid Unicode range, it's encoded as a surrogate pair and appended to the output string. If the code point is out of the valid Unicode range, an exception is thrown.
     *
     * @param chars A pointer to the first element in the sequence of UTF-32 encoded code points.
     * @param length The number of elements in the input sequence.
     * @param utf16 A reference to a std::u16string object where the output will be stored.
     * @throws soci_error If an invalid UTF-32 code point is encountered (out of Unicode range).
     */
    inline void utf32_to_utf16_common(const char32_t *chars, std::size_t length, std::u16string &utf16)
    {
      for (std::size_t i = 0; i < length; ++i)
      {
        char32_t codepoint = chars[i];

        if (codepoint <= 0xFFFF)
        {
          // BMP character
          utf16.push_back(static_cast<char16_t>(codepoint));
        }
        else if (codepoint <= 0x10FFFF)
        {
          // Encode as a surrogate pair
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
    }

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-32 encoded string.
     *
     * This function takes a byte array representing a UTF-8 encoded string and its length,
     * then converts it into a UTF-32 encoded string. The conversion is done by iterating over the bytes
     * of the input string and checking their leading bits to determine how many bytes are in the current character.
     * Depending on this, the function extracts the code points from the bytes and appends them to the output string.
     * If an invalid UTF-8 sequence is encountered, a soci_error exception is thrown with an appropriate error message.
     *
     * @param bytes A pointer to the first byte of the input UTF-8 encoded string.
     * @param length The number of bytes in the input string.
     * @param utf32 A reference to a std::u32string object where the converted UTF-32 encoded string will be stored.
     */
    inline void utf8_to_utf32_common(const unsigned char *bytes, std::size_t length, std::u32string &utf32)
    {
      for (std::size_t i = 0; i < length;)
      {
        unsigned char c1 = bytes[i];

        if (c1 < 0x80)
        {
          // 1-byte sequence (ASCII)
          utf32.push_back(static_cast<char32_t>(c1));
          ++i;
        }
        else if ((c1 & 0xE0) == 0xC0)
        {
          // 2-byte sequence
          if (i + 1 >= length)
            throw soci_error("Invalid UTF-8 sequence (truncated 2-byte sequence)");

          unsigned char c2 = bytes[i + 1];
          if ((c2 & 0xC0) != 0x80)
            throw soci_error("Invalid UTF-8 sequence (invalid continuation byte in 2-byte sequence)");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F)));
          i += 2;
        }
        else if ((c1 & 0xF0) == 0xE0)
        {
          // 3-byte sequence
          if (i + 2 >= length)
            throw soci_error("Invalid UTF-8 sequence (truncated 3-byte sequence)");

          unsigned char c2 = bytes[i + 1];
          unsigned char c3 = bytes[i + 2];
          if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
            throw soci_error("Invalid UTF-8 sequence (invalid continuation bytes in 3-byte sequence)");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)));
          i += 3;
        }
        else if ((c1 & 0xF8) == 0xF0)
        {
          // 4-byte sequence
          if (i + 3 >= length)
            throw soci_error("Invalid UTF-8 sequence (truncated 4-byte sequence)");

          unsigned char c2 = bytes[i + 1];
          unsigned char c3 = bytes[i + 2];
          unsigned char c4 = bytes[i + 3];
          if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
            throw soci_error("Invalid UTF-8 sequence (invalid continuation bytes in 4-byte sequence)");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)));
          i += 4;
        }
        else
        {
          throw soci_error("Invalid UTF-8 sequence (invalid start byte)");
        }
      }
    }

    /**
     * @brief Converts a sequence of UTF-32 encoded characters into a UTF-8 encoded string.
     *
     * This function iterates over the input sequence of UTF-32 encoded characters and appends their
     * corresponding UTF-8 representation to the output string. The length of the input sequence is
     * specified by the 'length' parameter.
     *
     * @param chars Pointer to the first character in the input sequence.
     * @param length Number of characters in the input sequence.
     * @param utf8 Reference to the output string where the UTF-8 encoded representation will be appended.
     * @throw soci_error If an invalid UTF-32 code point is encountered.
     */
    inline void utf32_to_utf8_common(const char32_t *chars, std::size_t length, std::string &utf8)
    {
      for (std::size_t i = 0; i < length; ++i)
      {
        char32_t codepoint = chars[i];

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
        else
        {
          throw soci_error("Invalid UTF-32 code point");
        }
      }
    }

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
    inline std::u16string utf8_to_utf16(const std::string &utf8)
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
    inline std::string utf16_to_utf8(const std::u16string &utf16)
    {
      std::string utf8;
      utf8.reserve(utf16.size() * 3);

      const char16_t *chars = utf16.data();
      std::size_t length = utf16.length();

      utf16_to_utf8_common(chars, length, utf8);

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
    inline std::u32string utf16_to_utf32(const std::u16string &utf16)
    {
      std::u32string utf32;
      utf32.reserve(utf16.size());

      const char16_t *chars = utf16.data();
      std::size_t length = utf16.length();

      utf16_to_utf32_common(chars, length, utf32);

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
    inline std::u16string utf32_to_utf16(const std::u32string &utf32)
    {
      std::u16string utf16;
      utf16.reserve(utf32.size() * 2);

      const char32_t *chars = utf32.data();
      std::size_t length = utf32.length();

      utf32_to_utf16_common(chars, length, utf16);

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
    inline std::u32string utf8_to_utf32(const std::string &utf8)
    {
      std::u32string utf32;
      utf32.reserve(utf8.size());

      const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
      std::size_t length = utf8.length();

      utf8_to_utf32_common(bytes, length, utf32);

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
    inline std::string utf32_to_utf8(const std::u32string &utf32)
    {
      if (!is_valid_utf32(utf32))
      {
        throw soci_error("Invalid UTF-32 string");
      }

      std::string utf8;
      utf8.reserve(utf32.size() * 4);

      const char32_t *chars = utf32.data();
      std::size_t length = utf32.length();

      utf32_to_utf8_common(chars, length, utf8);

      return utf8;
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
