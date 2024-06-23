//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/odbc/soci-odbc.h"
#include "common-tests.h"
#include <iostream>
#include <string>
#include <ctime>
#include <cmath>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_odbc();

// MS SQL-specific tests
TEST_CASE("MS SQL long string", "[odbc][mssql][long]")
{
    soci::session sql(backEnd, connectString);

    struct long_text_table_creator : public table_creator_base
    {
        explicit long_text_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            // Notice that 4000 is the maximal length of an nvarchar() column,
            // at least when using FreeTDS ODBC driver.
            sql << "create table soci_test ("
                        "long_text nvarchar(max) null, "
                        "fixed_text nvarchar(4000) null"
                    ")";
        }
    } long_text_table_creator(sql);

    // Build a string at least 8000 characters long to test that it survives
    // the round trip unscathed.
    std::ostringstream os;
    for ( int n = 0; n < 1000; ++n )
    {
        os << "Line #" << n << "\n";
    }

    std::string const str_in = os.str();
    CHECK_NOTHROW((
        sql << "insert into soci_test(long_text) values(:str)", use(str_in)
    ));

    std::string str_out;
    sql << "select long_text from soci_test", into(str_out);

    // Don't just compare the strings because the error message in case they
    // differ is completely unreadable due to their size, so give a better
    // error in the common failure case.
    if (str_out.length() != str_in.length())
    {
        FAIL("Read back string of length " << str_out.length() <<
             " instead of expected " << str_in.length());
    }
    else
    {
        CHECK(str_out == str_in);
    }

    // The long string should be truncated when inserting it into a fixed size
    // column.
    CHECK_THROWS_AS(
        (sql << "insert into soci_test(fixed_text) values(:str)", use(str_in)),
        soci_error
    );
}

TEST_CASE("MS SQL wide string", "[odbc][mssql][wstring]")
{
    soci::session sql(backEnd, connectString);

    struct wide_text_table_creator : public table_creator_base
    {
        explicit wide_text_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                "wide_text nvarchar(40) null"
                ")";
        }
    } wide_text_table_creator(sql);

    std::wstring const str_in = L"Hello, SOCI!";
    std::string const str_in_utf8 = "Hello, SOCI!";

    sql << "insert into soci_test(wide_text) values(:str)", use(str_in);

    std::wstring str_out;
    sql << "select wide_text from soci_test", into(str_out);
    
    std::string str_out_utf8;
    sql << "select wide_text from soci_test", into(str_out_utf8);

    CHECK(str_out == str_in);
    
    CHECK(str_out_utf8 == str_in_utf8);
    

}

TEST_CASE("MS SQL wide string vector", "[odbc][mssql][vector][wstring]")
{
    soci::session sql(backEnd, connectString);

    struct wide_text_table_creator : public table_creator_base
    {
        explicit wide_text_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                "wide_text nvarchar(40) null"
                ")";
        }
    } wide_text_table_creator(sql);

    std::vector<std::wstring> const str_in = {
        L"Hello, SOCI!",
        L"Hello, World!",
        L"Hello, Universe!",
        L"Hello, Galaxy!"
    };

    sql << "insert into soci_test(wide_text) values(:str)", use(str_in);

    std::vector<std::wstring> str_out(4);

    sql << "select wide_text from soci_test", into(str_out);


    CHECK(str_out.size() == str_in.size());
    for (std::size_t i = 0; i != str_in.size(); ++i)
    {
        CHECK(str_out[i] == str_in[i]);
    }
    
}

TEST_CASE("MS SQL wide char", "[odbc][mssql][wchar]")
{
    soci::session sql(backEnd, connectString);

    struct wide_char_table_creator : public table_creator_base
    {
        explicit wide_char_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                "wide_char nchar(2) null"
                ")";
        }
    } wide_char_table_creator(sql);

    wchar_t const ch_in = L'X';

    sql << "insert into soci_test(wide_char) values(:str)", use(ch_in);

    wchar_t ch_out;
    sql << "select wide_char from soci_test", into(ch_out);

    CHECK(ch_out == ch_in);
}

TEST_CASE("MS SQL wchar vector", "[odbc][mssql][vector][wchar]")
{
    soci::session sql(backEnd, connectString);

    struct wide_char_table_creator : public table_creator_base
    {
        explicit wide_char_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                "wide_char nchar(2) null"
                ")";
        }
    } wide_char_table_creator(sql);

    std::vector<wchar_t> const ch_in = {
        L'A',
        L'B',
        L'C',
        L'D'
    };

    sql << "insert into soci_test(wide_char) values(:str)", use(ch_in);

    std::vector<wchar_t> ch_out(4);

    sql << "select wide_char from soci_test", into(ch_out);

    CHECK(ch_out.size() == ch_in.size());
    for (std::size_t i = 0; i != ch_in.size(); ++i)
    {
        CHECK(ch_out[i] == ch_in[i]);
    }
}

// TODO: See if we can get this to work on Windows. The tests pass on Linux/MacOS.
// It seems that on Linux the MS SQL ODBC driver does implicitly convert
// between UTF-8 and UTF-16, but on Windows it doesn't.
// For standard_into_type_backend it's possible to describe the column and
// implicit conversion was therefore possible. But for the vector_into_type_backend
// it that didn't work, as the call to describe_column() failed.

//TEST_CASE("MS SQL string stream implicit unicode conversion", "[odbc][mssql][string][stream][utf8-utf16-conversion]")
//{
//    soci::session sql(backEnd, connectString);
//
//    struct wide_text_table_creator : public table_creator_base
//    {
//        explicit wide_text_table_creator(soci::session& sql)
//            : table_creator_base(sql)
//        {
//            sql << "create table soci_test ("
//                "wide_text nvarchar(40) null"
//                ")";
//        }
//    } wide_text_table_creator(sql);
//
//    //std::string const str_in = u8"สวัสดี!";
//    std::string const str_in = "\xe0\xb8\xaa\xe0\xb8\xa7\xe0\xb8\xb1\xe0\xb8\xaa\xe0\xb8\x94\xe0\xb8\xb5!";
//    
//    sql << "insert into soci_test(wide_text) values(N'" << str_in << "')";
//
//    std::string str_out;
//    sql << "select wide_text from soci_test", into(str_out);
//
//    std::wstring wstr_out;
//    sql << "select wide_text from soci_test", into(wstr_out);
//
//    CHECK(str_out == str_in);
//    
//#if defined(SOCI_WCHAR_T_IS_WIDE) // Unices
//    CHECK(wstr_out == L"\U00000E2A\U00000E27\U00000E31\U00000E2A\U00000E14\U00000E35\U00000021");
//#else // Windows
//    CHECK(wstr_out == L"\u0E2A\u0E27\u0E31\u0E2A\u0E14\u0E35\u0021");
//#endif
//
//}
//
//TEST_CASE("MS SQL wide string stream implicit unicode conversion", "[odbc][mssql][wstring][stream][utf8-utf16-conversion]")
//{
//    soci::session sql(backEnd, connectString);
//
//    struct wide_text_table_creator : public table_creator_base
//    {
//        explicit wide_text_table_creator(soci::session& sql)
//            : table_creator_base(sql)
//        {
//            sql << "create table soci_test ("
//                "wide_text nvarchar(40) null"
//                ")";
//        }
//    } wide_text_table_creator(sql);
//
//    //std::string const str_in = u8"สวัสดี!";
//    std::wstring const wstr_in = L"\u0E2A\u0E27\u0E31\u0E2A\u0E14\u0E35\u0021";
//
//    sql << "insert into soci_test(wide_text) values(N'" << wstr_in << "')";
//
//    std::string str_out;
//    sql << "select wide_text from soci_test", into(str_out);
//
//    std::wstring wstr_out;
//    sql << "select wide_text from soci_test", into(wstr_out);
//
//    CHECK(str_out == "\xe0\xb8\xaa\xe0\xb8\xa7\xe0\xb8\xb1\xe0\xb8\xaa\xe0\xb8\x94\xe0\xb8\xb5!");
//    CHECK(wstr_out == wstr_in);
//
//}

TEST_CASE("UTF-8 validation tests", "[unicode]") {
    using namespace soci::details;

    // Valid UTF-8 strings
    REQUIRE(is_valid_utf8("Hello, world!")); // valid ASCII
    REQUIRE(is_valid_utf8("")); // Empty string
    REQUIRE(is_valid_utf8(u8"Здравствуй, мир!")); // valid UTF-8
    REQUIRE(is_valid_utf8(u8"こんにちは世界")); // valid UTF-8
    REQUIRE(is_valid_utf8(u8"😀😁😂🤣😃😄😅😆")); // valid UTF-8 with emojis

    // Invalid UTF-8 strings
    REQUIRE(!is_valid_utf8("\x80")); // Invalid single byte
    REQUIRE(!is_valid_utf8("\xC3\x28")); // Invalid two-byte character
    REQUIRE(!is_valid_utf8("\xE2\x82")); // Truncated three-byte character
    REQUIRE(!is_valid_utf8("\xF0\x90\x28")); // Truncated four-byte character
    REQUIRE(!is_valid_utf8("\xF0\x90\x8D\x80\x80")); // Extra byte in four-byte character
}

TEST_CASE("UTF-16 validation tests", "[unicode]") {
    using namespace soci::details;

    // Valid UTF-16 strings
    REQUIRE(is_valid_utf16(u"Hello, world!")); // valid ASCII
    REQUIRE(is_valid_utf16(u"Здравствуй, мир!")); // valid Cyrillic
    REQUIRE(is_valid_utf16(u"こんにちは世界")); // valid Japanese
    REQUIRE(is_valid_utf16(u"😀😁😂🤣😃😄😅😆")); // valid emojis

    // Invalid UTF-16 strings
    std::u16string invalid_utf16 = u"";
    invalid_utf16 += 0xD800; // lone high surrogate
    REQUIRE(!is_valid_utf16(invalid_utf16)); // Invalid UTF-16

    std::u16string valid_utf16 = u"";
    valid_utf16 += 0xD800;
    valid_utf16 += 0xDC00; // valid surrogate pair
    REQUIRE(is_valid_utf16(valid_utf16));

    invalid_utf16 = u"";
    invalid_utf16 += 0xDC00; // lone low surrogate
    REQUIRE(!is_valid_utf16(invalid_utf16)); // Invalid UTF-16

    invalid_utf16 = u"";
    invalid_utf16 += 0xD800;
    invalid_utf16 += 0xD800; // two high surrogates in a row
    REQUIRE(!is_valid_utf16(invalid_utf16)); // Invalid UTF-16

    invalid_utf16 = u"";
    invalid_utf16 += 0xDC00;
    invalid_utf16 += 0xDC00; // two low surrogates in a row
    REQUIRE(!is_valid_utf16(invalid_utf16)); // Invalid UTF-16
}

TEST_CASE("UTF-32 validation tests", "[unicode]") {
    using namespace soci::details;

    // Valid UTF-32 strings
    REQUIRE(is_valid_utf32(U"Hello, world!")); // valid ASCII
    REQUIRE(is_valid_utf32(U"Здравствуй, мир!")); // valid Cyrillic
    REQUIRE(is_valid_utf32(U"こんにちは世界")); // valid Japanese
    REQUIRE(is_valid_utf32(U"😀😁😂🤣😃😄😅😆")); // valid emojis

    // Invalid UTF-32 strings
    REQUIRE(!is_valid_utf32(U"\x110000")); // Invalid UTF-32 code point
    REQUIRE(!is_valid_utf32(U"\x1FFFFF")); // Invalid range
    REQUIRE(!is_valid_utf32(U"\xFFFFFFFF")); // Invalid range
}

TEST_CASE("UTF-16 to UTF-32 conversion tests", "[unicode]") {
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf16_to_utf32(u"Hello, world!") == U"Hello, world!");
    REQUIRE(utf16_to_utf32(u"こんにちは世界") == U"こんにちは世界");
    REQUIRE(utf16_to_utf32(u"😀😁😂🤣😃😄😅😆") == U"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u16string utf16;
    utf16.push_back(char16_t(0xD83D)); // high surrogate
    utf16.push_back(char16_t(0xDE00)); // low surrogate
    REQUIRE(utf16_to_utf32(utf16) == U"\U0001F600"); // 😀

    // Invalid conversion (should throw an exception)
    std::u16string invalid_utf16;
    invalid_utf16.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf16_to_utf32(invalid_utf16), soci::soci_error);
}

TEST_CASE("UTF-32 to UTF-16 conversion tests", "[unicode]") {
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf32_to_utf16(U"Hello, world!") == u"Hello, world!");
    REQUIRE(utf32_to_utf16(U"こんにちは世界") == u"こんにちは世界");
    REQUIRE(utf32_to_utf16(U"😀😁😂🤣😃😄😅😆") == u"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u32string utf32 = U"\U0001F600"; // 😀
    std::u16string expected_utf16;
    expected_utf16.push_back(0xD83D); // high surrogate
    expected_utf16.push_back(0xDE00); // low surrogate
    REQUIRE(utf32_to_utf16(utf32) == expected_utf16);

    // Invalid conversion (should throw an exception)
    std::u32string invalid_utf32 = U"\x110000"; // Invalid code point
    REQUIRE_THROWS_AS(utf32_to_utf16(invalid_utf32), soci::soci_error);
}

TEST_CASE("UTF-8 to UTF-16 conversion tests", "[unicode]") {
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf8_to_utf16(u8"Hello, world!") == u"Hello, world!");
    REQUIRE(utf8_to_utf16(u8"こんにちは世界") == u"こんにちは世界");
    REQUIRE(utf8_to_utf16(u8"😀😁😂🤣😃😄😅😆") == u"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    std::u16string expected_utf16 = u"\xD83D\xDE00";
    REQUIRE(utf8_to_utf16_fallback(utf8) == expected_utf16);

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_utf16(invalid_utf8), soci::soci_error);
}

TEST_CASE("UTF-16 to UTF-8 conversion tests", "[unicode]") {
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf16_to_utf8(u"Hello, world!") == u8"Hello, world!");
    REQUIRE(utf16_to_utf8(u"こんにちは世界") == u8"こんにちは世界");
    REQUIRE(utf16_to_utf8(u"😀😁😂🤣😃😄😅😆") == u8"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u16string utf16;
    utf16.push_back(0xD83D); // high surrogate
    utf16.push_back(0xDE00); // low surrogate
    REQUIRE(utf16_to_utf8(utf16) == "\xF0\x9F\x98\x80"); // 😀 
  
    // Invalid conversion (should throw an exception)
    std::u16string invalid_utf16;
    invalid_utf16.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf16_to_utf8(invalid_utf16), soci::soci_error);
}

TEST_CASE("UTF-8 to UTF-32 conversion tests", "[unicode]") {
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf8_to_utf32(u8"Hello, world!") == U"Hello, world!");
    REQUIRE(utf8_to_utf32(u8"こんにちは世界") == U"こんにちは世界");
    REQUIRE(utf8_to_utf32(u8"😀😁😂🤣😃😄😅😆") == U"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    REQUIRE(utf8_to_utf32(utf8) == U"\U0001F600");

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_utf32(invalid_utf8), soci::soci_error);
}

TEST_CASE("UTF-32 to UTF-8 conversion tests", "[unicode]") {
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf32_to_utf8(U"Hello, world!") == u8"Hello, world!");
    REQUIRE(utf32_to_utf8(U"こんにちは世界") == u8"こんにちは世界");
    REQUIRE(utf32_to_utf8(U"😀😁😂🤣😃😄😅😆") == u8"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u32string utf32 = U"\U0001F600"; // 😀
    REQUIRE(utf32_to_utf8(utf32) == "\xF0\x9F\x98\x80");

    // Invalid conversion (should throw an exception)
    std::u32string invalid_utf32 = U"\x110000"; // Invalid code point
    REQUIRE_THROWS_AS(utf32_to_utf8(invalid_utf32), soci::soci_error);
    
    // Invalid conversion (should throw an exception)
    std::u32string invalid_wide;
    invalid_wide.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf32_to_utf8(invalid_wide), soci::soci_error);
    
}

TEST_CASE("UTF-8 to wide string conversion tests", "[unicode]") {
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf8_to_wide(u8"Hello, world!") == L"Hello, world!");
    REQUIRE(utf8_to_wide(u8"こんにちは世界") == L"こんにちは世界");
    REQUIRE(utf8_to_wide(u8"😀😁😂🤣😃😄😅😆") == L"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    std::wstring expected_wide = L"\U0001F600";
    REQUIRE(utf8_to_wide(utf8) == expected_wide);

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_wide(invalid_utf8), soci::soci_error);
}

TEST_CASE("Wide string to UTF-8 conversion tests", "[unicode]") {
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(wide_to_utf8(L"Hello, world!") == u8"Hello, world!");
    REQUIRE(wide_to_utf8(L"こんにちは世界") == u8"こんにちは世界");
    REQUIRE(wide_to_utf8(L"😀😁😂🤣😃😄😅😆") == u8"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::wstring wide = L"\U0001F600"; // 😀
    REQUIRE(wide_to_utf8(wide) == "\xF0\x9F\x98\x80");

    // Invalid conversion (should throw an exception)
    std::wstring invalid_wide;
    invalid_wide.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(wide_to_utf8(invalid_wide), soci::soci_error);
}

TEST_CASE("UTF8->UTF16 Conversion Speed Test (scalar) (ASCII)", "[unicode]")
{
    std::string utf8(1000000, 'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result1 = details::utf8_to_utf16_fallback(utf8); // Non-intrinsic
        
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF8->UTF16 Scalar ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF8->UTF16 Conversion Speed Test (scalar) (non-ASCII)", "[unicode]")
{
    std::string utf8;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
      utf8.append("世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result1 = details::utf8_to_utf16_fallback(utf8); // Non-intrinsic
        
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF8->UTF16 Scalar Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF8->UTF16 Conversion Speed Test (optimized) (ASCII)", "[unicode]")
{
    std::string utf8(1000000, 'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result1 = details::utf8_to_utf16(utf8); // Non-intrinsic
        
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF8->UTF16 Optimized ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF8->UTF16 Conversion Speed Test (optimized) (non-ASCII)", "[unicode]")
{
    std::string utf8;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
      utf8.append("世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result1 = details::utf8_to_utf16(utf8); // Non-intrinsic
        
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF8->UTF16 Optimized Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

// UTF16->UTF8 Conversion Speed Test

TEST_CASE("UTF16->UTF8 Conversion Speed Test (scalar) (ASCII)", "[unicode]")
{
    std::u16string utf16(1000000, u'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::string result1 = details::utf16_to_utf8_fallback(utf16); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF16->UTF8 Scalar ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF16->UTF8 Conversion Speed Test (scalar) (non-ASCII)", "[unicode]")
{
    std::u16string utf16;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf16.append(u"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::string result1 = details::utf16_to_utf8_fallback(utf16); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF16->UTF8 Scalar Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF16->UTF8 Conversion Speed Test (optimized) (ASCII)", "[unicode]")
{
    std::u16string utf16(1000000, u'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::string result1 = details::utf16_to_utf8(utf16); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF16->UTF8 Optimized ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF16->UTF8 Conversion Speed Test (optimized) (non-ASCII)", "[unicode]")
{
    std::u16string utf16;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf16.append(u"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::string result1 = details::utf16_to_utf8(utf16); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF16->UTF8 Optimized Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

// UTF32->UTF16 Conversion Speed Test

TEST_CASE("UTF32->UTF16 Conversion Speed Test (scalar) (ASCII)", "[unicode]")
{
    std::u32string utf32(1000000, U'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result1 = details::utf32_to_utf16_fallback(utf32); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF16 Scalar ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF16 Conversion Speed Test (scalar) (non-ASCII)", "[unicode]")
{
    std::u32string utf32;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf32.append(U"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result1 = details::utf32_to_utf16_fallback(utf32); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF16 Scalar Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF16 Conversion Speed Test (optimized) (ASCII)", "[unicode]")
{
    std::u32string utf32(1000000, U'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result1 = details::utf32_to_utf16(utf32); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF16 Optimized ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF16 Conversion Speed Test (optimized) (non-ASCII)", "[unicode]")
{
    std::u32string utf32;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf32.append(U"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result1 = details::utf32_to_utf16(utf32); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF16 Optimized Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF16 Conversion Speed Test (NEON) (ASCII)", "[unicode]")
{
    std::u32string utf32(1000000, U'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result = details::utf32_to_utf16_neon(utf32);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF16 NEON ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF16 Conversion Speed Test (NEON) (non-ASCII)", "[unicode]")
{
    std::u32string utf32;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf32.append(U"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u16string result = details::utf32_to_utf16_neon(utf32);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF16 NEON Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF16 Conversion (NEON) (mixed characters)", "[unicode]")
{
    std::u32string utf32 = U"Hello, 世界! 🌍";
    std::u16string expected = u"Hello, 世界! 🌍";
    std::u16string result = details::utf32_to_utf16_neon(utf32);
    REQUIRE(result == expected);
}

TEST_CASE("UTF32->UTF16 Conversion (NEON) (8-15 characters)", "[unicode]")
{
    std::u32string utf32 = U"ABCDEFGHIJK";
    std::u16string expected = u"ABCDEFGHIJK";
    std::u16string result = details::utf32_to_utf16_neon(utf32);
    REQUIRE(result == expected);
}

TEST_CASE("UTF32->UTF16 Conversion (NEON) (4-7 characters)", "[unicode]")
{
    std::u32string utf32 = U"ABCD";
    std::u16string expected = u"ABCD";
    std::u16string result = details::utf32_to_utf16_neon(utf32);
    REQUIRE(result == expected);
}

TEST_CASE("UTF32->UTF16 Conversion (NEON) (1-3 characters)", "[unicode]")
{
    std::u32string utf32 = U"ABC";
    std::u16string expected = u"ABC";
    std::u16string result = details::utf32_to_utf16_neon(utf32);
    REQUIRE(result == expected);
}

TEST_CASE("UTF32->UTF16 Conversion (NEON) (empty string)", "[unicode]")
{
    std::u32string utf32;
    std::u16string expected;
    std::u16string result = details::utf32_to_utf16_neon(utf32);
    REQUIRE(result == expected);
}

TEST_CASE("UTF32->UTF16 Conversion (NEON) (all paths)", "[unicode]")
{
    // Test string with exactly 16 characters
    std::u32string utf32_16 = U"ABCDEFGHIJKLMNOP";
    std::u16string expected_16 = u"ABCDEFGHIJKLMNOP";
    REQUIRE(details::utf32_to_utf16_neon(utf32_16) == expected_16);

    // Test string with characters requiring surrogate pairs
    // std::u32string utf32_surrogate = U"ABC🌍DEF";
    // std::u16string expected_surrogate = u"ABC\U0001F30FDEF";
    // auto result_surrogate = details::utf32_to_utf16_neon(utf32_surrogate);
    // REQUIRE(result_surrogate == expected_surrogate);
    
    // Test string with characters requiring surrogate pairs
    std::u32string utf32_surrogate = U"ABC\U0001F30FDEF";
    auto result_surrogate = details::utf32_to_utf16_neon(utf32_surrogate);
    REQUIRE(result_surrogate.length() == 8);
    REQUIRE(static_cast<uint16_t>(result_surrogate[0]) == 0x0041); // 'A'
    REQUIRE(static_cast<uint16_t>(result_surrogate[1]) == 0x0042); // 'B'
    REQUIRE(static_cast<uint16_t>(result_surrogate[2]) == 0x0043); // 'C'
    REQUIRE(static_cast<uint16_t>(result_surrogate[3]) == 0xD83C); // High surrogate
    REQUIRE(static_cast<uint16_t>(result_surrogate[4]) == 0xDF0F); // Low surrogate (corrected)
    REQUIRE(static_cast<uint16_t>(result_surrogate[5]) == 0x0044); // 'D'
    REQUIRE(static_cast<uint16_t>(result_surrogate[6]) == 0x0045); // 'E'
    REQUIRE(static_cast<uint16_t>(result_surrogate[7]) == 0x0046); // 'F'
    
    // Additional check for surrogate pairs
    REQUIRE(result_surrogate.length() == 8);
    REQUIRE(result_surrogate[3] == 0xD83C);  // High surrogate
    REQUIRE(result_surrogate[4] == 0xDF0F);  // Low surrogate

    // Test string with mixed ASCII and non-ASCII
    std::u32string utf32_mixed = U"Hello, 世界!";
    std::u16string expected_mixed = u"Hello, 世界!";
    REQUIRE(details::utf32_to_utf16_neon(utf32_mixed) == expected_mixed);

    // Test very long string to ensure multiple blocks are processed
    std::u32string utf32_long(100000, U'A');
    std::u16string expected_long(100000, u'A');
    REQUIRE(details::utf32_to_utf16_neon(utf32_long) == expected_long);

    // Test string with characters just below and above 0xFFFF
    std::u32string utf32_edge = U"\U0000FFFE\U0000FFFF\U00010000\U00010001";
    std::u16string expected_edge = u"\xFFFE\xFFFF\xD800\xDC00\xD800\xDC01";
    auto result_edge = details::utf32_to_utf16_neon(utf32_edge);
    REQUIRE(result_edge == expected_edge);
    REQUIRE(result_edge.length() == 6);
}

TEST_CASE("UTF32->UTF16 Conversion (NEON) (surrogate pairs and edge cases)", "[unicode]")
{
    struct TestCase {
        char32_t input;
        std::vector<char16_t> expected;
    };

    std::vector<TestCase> testCases = {
        {0x0000, {0x0000}},
        {0x007F, {0x007F}},
        {0x0080, {0x0080}},
        {0x07FF, {0x07FF}},
        {0x0800, {0x0800}},
        {0xFFFF, {0xFFFF}},
        {0x10000, {0xD800, 0xDC00}},
        {0x10FFFF, {0xDBFF, 0xDFFF}},
        {0x1F30F, {0xD83C, 0xDF0F}},  // 🌍
        {0x1F600, {0xD83D, 0xDE00}},  // 😀
    };

    for (const auto& tc : testCases) {
        INFO("Testing U+" << std::hex << tc.input);
        std::u32string input(1, tc.input);
        std::u16string result = details::utf32_to_utf16_neon(input);
        REQUIRE(result.length() == tc.expected.size());
        for (size_t i = 0; i < result.length(); ++i) {
            REQUIRE(static_cast<uint16_t>(result[i]) == tc.expected[i]);
        }
    }
}

// UTF16->UTF32 Conversion Speed Test

TEST_CASE("UTF16->UTF32 Conversion Speed Test (scalar) (ASCII)", "[unicode]")
{
    std::u16string utf16(1000000, u'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u32string result1 = details::utf16_to_utf32_fallback(utf16); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF16->UTF32 Scalar ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF16->UTF32 Conversion Speed Test (scalar) (non-ASCII)", "[unicode]")
{
    std::u16string utf16;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf16.append(u"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u32string result1 = details::utf16_to_utf32_fallback(utf16); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF16->UTF32 Scalar Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF16->UTF32 Conversion Speed Test (optimized) (ASCII)", "[unicode]")
{
    std::u16string utf16(1000000, u'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u32string result1 = details::utf16_to_utf32(utf16); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF16->UTF32 Optimized ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF16->UTF32 Conversion Speed Test (optimized) (non-ASCII)", "[unicode]")
{
    std::u16string utf16;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf16.append(u"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u32string result1 = details::utf16_to_utf32(utf16); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF16->UTF32 Optimized Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

// UTF8->UTF32 Conversion Speed Test

TEST_CASE("UTF8->UTF32 Conversion Speed Test (scalar) (ASCII)", "[unicode]")
{
    std::string utf8(1000000, 'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u32string result1 = details::utf8_to_utf32_fallback(utf8); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF8->UTF32 Scalar ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF8->UTF32 Conversion Speed Test (scalar) (non-ASCII)", "[unicode]")
{
    std::string utf8;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf8.append("世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u32string result1 = details::utf8_to_utf32_fallback(utf8); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF8->UTF32 Scalar Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF8->UTF32 Conversion Speed Test (optimized) (ASCII)", "[unicode]")
{
    std::string utf8(1000000, 'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u32string result1 = details::utf8_to_utf32(utf8); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF8->UTF32 Optimized ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF8->UTF32 Conversion Speed Test (optimized) (non-ASCII)", "[unicode]")
{
    std::string utf8;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf8.append("世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::u32string result1 = details::utf8_to_utf32(utf8); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF8->UTF32 Optimized Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

// UTF32->UTF8 Conversion Speed Test

TEST_CASE("UTF32->UTF8 Conversion Speed Test (scalar) (ASCII)", "[unicode]")
{
    std::u32string utf32(1000000, U'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::string result1 = details::utf32_to_utf8_fallback(utf32); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF8 Scalar ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF8 Conversion Speed Test (scalar) (non-ASCII)", "[unicode]")
{
    std::u32string utf32;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf32.append(U"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::string result1 = details::utf32_to_utf8_fallback(utf32); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF8 Scalar Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF8 Conversion Speed Test (optimized) (ASCII)", "[unicode]")
{
    std::u32string utf32(1000000, U'A'); // 1,000,000 ASCII characters
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::string result1 = details::utf32_to_utf8(utf32); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF8 Optimized ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

TEST_CASE("UTF32->UTF8 Conversion Speed Test (optimized) (non-ASCII)", "[unicode]")
{
    std::u32string utf32;
    for(size_t i = 0UL; i < 1000000UL; ++i)
    {
        utf32.append(U"世");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i)
    {
        std::string result1 = details::utf32_to_utf8(utf32); // Non-intrinsic
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "UTF32->UTF8 Optimized Non-ASCII Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh smallint, ll bigint, ul numeric(20), "
                 "d float, num76 numeric(7,6), "
                 "tm datetime, i1 integer, i2 integer, i3 integer, "
                 "name varchar(20))";
    }
};

struct table_creator_two : public table_creator_base
{
    table_creator_two(soci::session & sql)
        : table_creator_base(sql)
    {
        sql  << "create table soci_test(num_float float, num_int integer,"
                     " name varchar(20), sometime datetime, chr char)";
    }
};

struct table_creator_three : public table_creator_base
{
    table_creator_three(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(name varchar(100) not null, "
            "phone varchar(15))";
    }
};

struct table_creator_for_get_affected_rows : table_creator_base
{
    table_creator_for_get_affected_rows(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val integer)";
    }
};

struct table_creator_for_clob : table_creator_base
{
    table_creator_for_clob(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, s text)";
    }
};

struct table_creator_for_xml : table_creator_base
{
    table_creator_for_xml(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, x xml)";
    }
};

struct table_creator_for_get_last_insert_id : table_creator_base
{
    table_creator_for_get_last_insert_id(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test (id integer identity(1, 1), val integer)";
    }
};

//
// Support for SOCI Common Tests
//

class test_context : public test_context_base
{
public:
    test_context(backend_factory const &backend,
                std::string const &connstr)
        : test_context_base(backend, connstr) {}

    table_creator_base* table_creator_1(soci::session& s) const override
    {
        return new table_creator_one(s);
    }

    table_creator_base* table_creator_2(soci::session& s) const override
    {
        return new table_creator_two(s);
    }

    table_creator_base* table_creator_3(soci::session& s) const override
    {
        return new table_creator_three(s);
    }

    table_creator_base * table_creator_4(soci::session& s) const override
    {
        return new table_creator_for_get_affected_rows(s);
    }

    tests::table_creator_base* table_creator_clob(soci::session& s) const override
    {
        return new table_creator_for_clob(s);
    }

    tests::table_creator_base* table_creator_xml(soci::session& s) const override
    {
        return new table_creator_for_xml(s);
    }

    tests::table_creator_base* table_creator_get_last_insert_id(soci::session& s) const override
    {
        return new table_creator_for_get_last_insert_id(s);
    }

    bool has_real_xml_support() const override
    {
        return true;
    }

    std::string to_date_time(std::string const &datdt_string) const override
    {
        return "convert(datetime, \'" + datdt_string + "\', 120)";
    }

    bool has_multiple_select_bug() const override
    {
        // MS SQL does support MARS (multiple active result sets) since 2005
        // version, but this support needs to be explicitly enabled and is not
        // implemented in FreeTDS ODBC driver used under Unix currently, so err
        // on the side of caution and suppose that it's not supported.
        return true;
    }

    std::string sql_length(std::string const& s) const override
    {
        return "len(" + s + ")";
    }
};

int main(int argc, char** argv)
{
#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run assert()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

    if (argc >= 2 && argv[1][0] != '-')
    {
        connectString = argv[1];

        // Replace the connect string with the process name to ensure that
        // CATCH uses the correct name in its messages.
        argv[1] = argv[0];

        argc--;
        argv++;
    }
    else
    {
        connectString = "FILEDSN=./test-mssql.dsn";
    }

    test_context tc(backEnd, connectString);
    

    return Catch::Session().run(argc, argv);
    
}
