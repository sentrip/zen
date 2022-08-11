#include "catch.hpp"

#include "zen_enum.h"
#include "zen_fmt.h"

ZEN_ENUM(Names,
    Value0,
    Value1,
    Value2
);


ZEN_ENUM_VALUES(Values,
    (Value0, 1),
    (Value1, 3),
    (Value2, 5)
);


ZEN_ENUM_FLAG(Flag,
    Success,
    Flag0,
    Flag1,
    Flag2
);


ZEN_ENUM_FLAG_VALUES(FlagValues,
    Success,
    (Flag0, 0x1),
    (Flag1, 0x4),
    (Flag2, 0x10)
);


TEST_CASE("enum - names only", "[utility]")
{
    REQUIRE( 3 == zen::enum_size<Names> );
    REQUIRE( 0u == u32(Names::Value0) );
    REQUIRE( 1u == u32(Names::Value1) );
    REQUIRE( 2u == u32(Names::Value2) );
    REQUIRE( zen::string_view{"Value0"} == zen::string_view{zen::enum_name<Names>(Names::Value0)} );
    REQUIRE( zen::string_view{"Value1"} == zen::string_view{zen::enum_name<Names>(Names::Value1)} );
    REQUIRE( zen::string_view{"Value2"} == zen::string_view{zen::enum_name<Names>(Names::Value2)} );
    
    zen::fmt::buffer<> buf{};
    zen::format(buf, "{}", Names::Value0);
    REQUIRE( "Value0" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", Names::Value1);
    REQUIRE( "Value1" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", Names::Value2);
    REQUIRE( "Value2" == buf.view() );
    buf.clear();
}

TEST_CASE("enum - names values", "[utility]")
{
    REQUIRE( 3 == zen::enum_size<Values> );
    REQUIRE( 1u == u32(Values::Value0) );
    REQUIRE( 3u == u32(Values::Value1) );
    REQUIRE( 5u == u32(Values::Value2) );
    REQUIRE( zen::string_view{"Value0"} == zen::string_view{zen::enum_name<Values>(Values::Value0)} );
    REQUIRE( zen::string_view{"Value1"} == zen::string_view{zen::enum_name<Values>(Values::Value1)} );
    REQUIRE( zen::string_view{"Value2"} == zen::string_view{zen::enum_name<Values>(Values::Value2)} );
    
    zen::fmt::buffer<> buf{};
    zen::format(buf, "{}", Values::Value0);
    REQUIRE( "Value0" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", Values::Value1);
    REQUIRE( "Value1" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", Values::Value2);
    REQUIRE( "Value2" == buf.view() );
    buf.clear();
}

TEST_CASE("enum - flag", "[utility]")
{
    REQUIRE( 0   == u32(Flag::Success) );
    REQUIRE( 0x1 == u32(Flag::Flag0) );
    REQUIRE( 0x2 == u32(Flag::Flag1) );
    REQUIRE( 0x4 == u32(Flag::Flag2) );
    REQUIRE( 0x3 == u32(Flag::Flag0 | Flag::Flag1) );
    REQUIRE( 0x0 == u32(Flag::Flag0 & Flag::Flag1) );
    REQUIRE( 0x2 == u32(Flag::Flag1 & Flag::Flag1) );
    
    zen::fmt::buffer<> buf{};
    zen::format(buf, "{}", Flag::Success);
    REQUIRE( "Success" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", Flag::Flag0);
    REQUIRE( "Flag0" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", Flag::Flag1);
    REQUIRE( "Flag1" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", Flag::Flag2);
    REQUIRE( "Flag2" == buf.view() );
    buf.clear();
    
    zen::format(buf, "{}", Flag::Flag0 | Flag::Flag1);
    REQUIRE( "Flag0 | Flag1" == buf.view() );
    buf.clear();
}

TEST_CASE("enum - flag values", "[utility]")
{
    REQUIRE( 0    == u32(FlagValues::Success) );
    REQUIRE( 0x1  == u32(FlagValues::Flag0) );
    REQUIRE( 0x4  == u32(FlagValues::Flag1) );
    REQUIRE( 0x10 == u32(FlagValues::Flag2) );
    REQUIRE( 0x5  == u32(FlagValues::Flag0 | FlagValues::Flag1) );
    REQUIRE( 0x0  == u32(FlagValues::Flag0 & FlagValues::Flag1) );
    REQUIRE( 0x4  == u32(FlagValues::Flag1 & FlagValues::Flag1) );
    
    zen::fmt::buffer<> buf{};
    zen::format(buf, "{}", FlagValues::Success);
    REQUIRE( "Success" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", FlagValues::Flag0);
    REQUIRE( "Flag0" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", FlagValues::Flag1);
    REQUIRE( "Flag1" == buf.view() );
    buf.clear();

    zen::format(buf, "{}", FlagValues::Flag2);
    REQUIRE( "Flag2" == buf.view() );
    buf.clear();
    
    zen::format(buf, "{}", FlagValues::Flag0 | FlagValues::Flag1);
    REQUIRE( "Flag0 | Flag1" == buf.view() );
    buf.clear();
}
