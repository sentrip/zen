#include "catch.hpp"

#include "zen_span.h"

TEST_CASE("test_span", "[containers]") 
{
    u8 data[8]{1,2,3,4,5,6,7,8};
    u8* ptr = data;
    const u8* cptr = data;

    SECTION("default span") {
        zen::span<u8> span{};
        REQUIRE( 0 == span.size() );
        REQUIRE( true == span.empty() );
        REQUIRE( nullptr == span.data() );
        REQUIRE( nullptr == span.begin() );
        REQUIRE( nullptr == span.end() );
    }
    SECTION("mutable span") {
        zen::span<u8> span{ptr, 8};
        REQUIRE( 8 == span.size() );
        REQUIRE( false == span.empty() );
        REQUIRE( ptr == span.data() );
        REQUIRE( ptr == span.begin() );
        REQUIRE( ptr + 8 == span.end() );
        REQUIRE( !std::is_const_v<std::remove_pointer_t<decltype(span.data())>> );
    }
    SECTION("const span") {
        zen::span<const u8> span{cptr, 8};
        REQUIRE( 8 == span.size() );
        REQUIRE( false == span.empty() );
        REQUIRE( cptr == span.data() );
        REQUIRE( cptr == span.begin() );
        REQUIRE( cptr + 8 == span.end() );
        REQUIRE( std::is_const_v<std::remove_pointer_t<decltype(span.data())>> );
    }
    SECTION("span constructors") {
        zen::span<u8> range{ptr, ptr + 8};
        REQUIRE( 8 == range.size() );

        zen::span<u8> array_mutable{data};
        REQUIRE( 8 == array_mutable.size() );

        zen::span<const u8> array_const{data};
        REQUIRE( 8 == array_const.size() );

        zen::span<u8> from_data_size{range};
        REQUIRE( 8 == from_data_size.size() );
    }
}
