#include "catch.hpp"

#include "zen_fmt.h"
#include <vector>

#define TEST_FORMAT_BASIC(expected, f, value) { \
        zen::fmt::buffer<> out{}; \
        std::string_view ex{expected}; \
        zen::format(out, f, value); \
        REQUIRE( ex.size() == out.size() ); \
        REQUIRE( memcmp(out.data(), ex.data(), ex.size()) == 0 ); \
    }

TEST_CASE("fmt integer", "[utility]") 
{
    const char* ptr = nullptr;
    TEST_FORMAT_BASIC("false"               , "{}"      , false);
    TEST_FORMAT_BASIC("true"                , "{}"      , true);
    TEST_FORMAT_BASIC("c"                   , "{}"      , 'c');
    TEST_FORMAT_BASIC("123"                 , "{}"      , 123);
    TEST_FORMAT_BASIC("123456789012"        , "{}"      , 123456789012);
    TEST_FORMAT_BASIC("-123456789012"       , "{}"      , -123456789012);
    TEST_FORMAT_BASIC("18446744073709551615", "{}"      , UINT64_C(18446744073709551615));
    TEST_FORMAT_BASIC("0xcafebabe"          , "{}"      , (const void*)(ptr + 0xcafebabe));
}

TEST_CASE("fmt float", "[utility]") 
{ 
    TEST_FORMAT_BASIC("2.1"                 , "{}"      , 2.1f);
    TEST_FORMAT_BASIC("2.1"                 , "{}"      , 2.1);
    TEST_FORMAT_BASIC("2.100"               , "{:.3}"   , 2.1f);
    TEST_FORMAT_BASIC("2.100"               , "{:.3}"   , 2.1);
}

TEST_CASE("fmt string", "[utility]") 
{
    TEST_FORMAT_BASIC("abc"                 , "{}"      , "abc");
    TEST_FORMAT_BASIC("abc"                 , "{}"      , zen::string_view{"abc"});
    TEST_FORMAT_BASIC("{a, b, c}"           , "{}"      , std::vector<char>({'a', 'b', 'c'}));
    TEST_FORMAT_BASIC("abc"                 , "{}"      , std::string{"abc"});
}

TEST_CASE("fmt spec - size", "[utility]") 
{
    TEST_FORMAT_BASIC("abc  "               , "{:5}"    , "abc");
    TEST_FORMAT_BASIC("abcde"               , "{:3}"    , "abcde");
}

TEST_CASE("fmt spec - align", "[utility]") 
{
    TEST_FORMAT_BASIC("abc  "               , "{:<5}"   , "abc");
    TEST_FORMAT_BASIC("  abc"               , "{:>5}"   , "abc");
    TEST_FORMAT_BASIC(" abc "               , "{:^5}"   , "abc");
}

TEST_CASE("fmt spec - fill", "[utility]") 
{
    TEST_FORMAT_BASIC("abc00"               , "{:0<5}"   , "abc");
    TEST_FORMAT_BASIC("00abc"               , "{:0>5}"   , "abc");
    TEST_FORMAT_BASIC("0abc0"               , "{:0^5}"   , "abc");
}
