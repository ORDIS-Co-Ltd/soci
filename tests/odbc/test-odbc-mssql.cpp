//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/odbc/soci-odbc.h"
#include "test-context.h"
#include <iostream>
#include <string>
#include <ctime>
#include <cmath>

#include <catch.hpp>

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
      explicit wide_text_table_creator(soci::session &sql)
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
      explicit wide_text_table_creator(soci::session &sql)
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
      L"Hello, Galaxy!"};

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
      explicit wide_char_table_creator(soci::session &sql)
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
    explicit wide_char_table_creator(soci::session &sql)
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
      L'D'};

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

// TEST_CASE("MS SQL string stream implicit unicode conversion", "[odbc][mssql][string][stream][utf8-utf16-conversion]")
// {
//     soci::session sql(backEnd, connectString);

//     struct wide_text_table_creator : public table_creator_base
//     {
//         explicit wide_text_table_creator(soci::session& sql)
//             : table_creator_base(sql)
//         {
//             sql << "create table soci_test ("
//                 "wide_text nvarchar(40) null"
//                 ")";
//         }
//     } wide_text_table_creator(sql);

//     //std::string const str_in = u8"สวัสดี!";
//     std::string const str_in = "\xe0\xb8\xaa\xe0\xb8\xa7\xe0\xb8\xb1\xe0\xb8\xaa\xe0\xb8\x94\xe0\xb8\xb5!";

//     sql << "insert into soci_test(wide_text) values(N'" << str_in << "')";

//     std::string str_out;
//     sql << "select wide_text from soci_test", into(str_out);

//     std::wstring wstr_out;
//     sql << "select wide_text from soci_test", into(wstr_out);

//     CHECK(str_out == str_in);

// #if defined(SOCI_WCHAR_T_IS_WIDE) // Unices
//     CHECK(wstr_out == L"\U00000E2A\U00000E27\U00000E31\U00000E2A\U00000E14\U00000E35\U00000021");
// #else // Windows
//     CHECK(wstr_out == L"\u0E2A\u0E27\u0E31\u0E2A\u0E14\u0E35\u0021");
// #endif

// }

// TEST_CASE("MS SQL wide string stream implicit unicode conversion", "[odbc][mssql][wstring][stream][utf8-utf16-conversion]")
// {
//     soci::session sql(backEnd, connectString);

//     struct wide_text_table_creator : public table_creator_base
//     {
//         explicit wide_text_table_creator(soci::session& sql)
//             : table_creator_base(sql)
//         {
//             sql << "create table soci_test ("
//                 "wide_text nvarchar(40) null"
//                 ")";
//         }
//     } wide_text_table_creator(sql);

//     //std::string const str_in = u8"สวัสดี!";
//     std::wstring const wstr_in = L"\u0E2A\u0E27\u0E31\u0E2A\u0E14\u0E35\u0021";

//     sql << "insert into soci_test(wide_text) values(N'" << wstr_in << "')";

//     std::string str_out;
//     sql << "select wide_text from soci_test", into(str_out);

//     std::wstring wstr_out;
//     sql << "select wide_text from soci_test", into(wstr_out);

//     CHECK(str_out == "\xe0\xb8\xaa\xe0\xb8\xa7\xe0\xb8\xb1\xe0\xb8\xaa\xe0\xb8\x94\xe0\xb8\xb5!");
//     CHECK(wstr_out == wstr_in);

// }

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

class test_context : public test_context_common
{
public:
    test_context() = default;

    std::string get_example_connection_string() const override
    {
        return "FILEDSN=./test-mssql.dsn";
    }

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

test_context tc_odbc_mssql;
