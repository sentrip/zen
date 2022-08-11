#include "catch.hpp"

#include "zen_macros.h"

TEST_CASE("macros - pack get", "[utility]") 
{
    REQUIRE( 0 == PACK_GET(0, (0, 1, 2, 3, 4, 5, 6, 7)) );
    REQUIRE( 1 == PACK_GET(1, (0, 1, 2, 3, 4, 5, 6, 7)) );
    REQUIRE( 2 == PACK_GET(2, (0, 1, 2, 3, 4, 5, 6, 7)) );
    REQUIRE( 3 == PACK_GET(3, (0, 1, 2, 3, 4, 5, 6, 7)) );
    REQUIRE( 4 == PACK_GET(4, (0, 1, 2, 3, 4, 5, 6, 7)) );
    REQUIRE( 5 == PACK_GET(5, (0, 1, 2, 3, 4, 5, 6, 7)) );
    REQUIRE( 6 == PACK_GET(6, (0, 1, 2, 3, 4, 5, 6, 7)) );
    REQUIRE( 7 == PACK_GET(7, (0, 1, 2, 3, 4, 5, 6, 7)) );
}

TEST_CASE("macros - for each", "[utility]") 
{
    int idx = 0;
    #define F(i, v) REQUIRE(v == (i + 1)); REQUIRE(i == idx++);
    FOR_EACH(F, 1, 2, 3, 4, 5, 6, 7, 8);
    #undef F
}

TEST_CASE("macros - for each arg", "[utility]") 
{
    int idx = 0;
    #define F(a, i, v) REQUIRE(a == 5); REQUIRE(v == (i + 1)); REQUIRE(i == idx++);
    FOR_EACH_ARG(F, 5, 1, 2, 3, 4, 5, 6, 7, 8);
    #undef F
}

TEST_CASE("macros - for each fold", "[utility]") 
{
    #define F(i, v) v
    auto sum = FOR_EACH_FOLD(F, +, 1, 2, 3, 4, 5, 6, 7, 8);
    REQUIRE( sum == (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8));
    #undef F
}

TEST_CASE("macros - for each comma", "[utility]") 
{
    static constexpr auto func = [](int a, int b, int c, int d){
        REQUIRE( 1 == a );
        REQUIRE( 2 == b );
        REQUIRE( 3 == c );
        REQUIRE( 4 == d );
    };
    #define F(i, v) v
    func( FOR_EACH_COMMA(F, 1, 2, 3, 4) );
    #undef F
}
