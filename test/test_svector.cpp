#include "catch.hpp"

#include "zen_span.h"
#include "zen_svector.h"

TEST_CASE("svector, functionalities", "[Utilities]")
{
    zen::svector<int, 4> v;

    SECTION("constructors") {
        zen::fvector<int, 4> a0;
        zen::fvector<int, 4> a1{1,2,3,4};
        zen::fvector<int, 4> a2{zen::span<int>{a1}};
        REQUIRE( 4 == a1.size() );
        REQUIRE( 4 == a2.size() );
        // zen::fvector<int, 4> a3{std::pmr::get_default_resource()}; // compile error

        zen::svector<int, 4> v0;
        zen::svector<int, 4> v1{1,2,3,4};
        zen::svector<int, 4> v2{zen::span<int>{v1}};
        REQUIRE( 4 == v1.size() );
        REQUIRE( 4 == v2.size() );
        zen::svector<int, 4> v3{std::pmr::get_default_resource()};
    }

    SECTION("push-pop")
    {
        REQUIRE( 0 == v.size() );

        v.push_back(99);

        REQUIRE( 1 == v.size() );
        REQUIRE( 99 == v[0]);
        REQUIRE( 99 == v.back());

        v.push_back(25);

        REQUIRE( 2 == v.size() );
        REQUIRE( 99 == v[0]);
        REQUIRE( 25 == v[1]);
        REQUIRE( 25 == v.back());
    }

    SECTION("dynamic resize, shrink")
    {
        REQUIRE( 0 == v.size() );
        REQUIRE( 4 == v.capacity() );
        REQUIRE( v.small() );

        for (int i = 0; i < 4; ++i)
            v.push_back(i);

        REQUIRE( 4 == v.size() );
        REQUIRE( 4 == v.capacity() );
        REQUIRE( v.small() );

        v.push_back(4);

        REQUIRE( 5 == v.size() );
        REQUIRE( 8 == v.capacity() );
        REQUIRE( !v.small() );
        for (int i = 0; i < 5; ++i)
            REQUIRE( i == v[i] );

        v.shrink_to_fit();

        REQUIRE( 8 == v.capacity() );

        v.pop_back();
        v.shrink_to_fit();

        REQUIRE( 4 == v.size() );
        REQUIRE( 4 == v.capacity() );
        REQUIRE( v.small() );
        for (int i = 0; i < 4; ++i)
            REQUIRE( i == v[i] );
    }

    SECTION("resize")
    {
        // Stack -> Stack
        v.resize(2);

        REQUIRE( v.small() );
        REQUIRE( 2 == v.size() );
        REQUIRE( 4 == v.capacity() );
        REQUIRE( 0 == v[0] );
        REQUIRE( 0 == v[1] );

        // Stack -> Stack value
        v.resize(4, 9);
        REQUIRE( v.small() );
        REQUIRE( 4 == v.size() );
        REQUIRE( 4 == v.capacity() );
        REQUIRE( 0 == v[0] );
        REQUIRE( 0 == v[1] );
        REQUIRE( 9 == v[2] );
        REQUIRE( 9 == v[3] );

        // Stack -> Heap
        v.resize(6, 5);
        REQUIRE( !v.small() );
        REQUIRE( 6 == v.size() );
        REQUIRE( 8 == v.capacity() );
        REQUIRE( 0 == v[0] );
        REQUIRE( 0 == v[1] );
        REQUIRE( 9 == v[2] );
        REQUIRE( 9 == v[3] );
        REQUIRE( 5 == v[4] );
        REQUIRE( 5 == v[5] );

        // Heap -> Stack
        v.resize(4, 999);
        REQUIRE( v.small() );
        REQUIRE( 4 == v.size() );
        REQUIRE( 4 == v.capacity() );
        REQUIRE( 0 == v[0] );
        REQUIRE( 0 == v[1] );
        REQUIRE( 9 == v[2] );
        REQUIRE( 9 == v[3] );

        SECTION("bug - stack->heap resize when empty")
        {
            zen::svector<int, 8> b;
            for (int iter = 0; iter < 2; ++iter) {
                b.clear();
                if (iter == 1) {
                    b.push_back(5);
                }
                b.resize(10, 5);
                REQUIRE( !b.small() );
                REQUIRE( 10 == b.size() );
                REQUIRE( 16 == b.capacity() );
                for (int i = 0; i < 10; ++i) {
                    REQUIRE( 5 == b[i] );
                }
            }
        }
    }

    SECTION("insert")
    {
        SECTION("end")
        {
            // Single End
            v.insert(v.end(), 1);

            REQUIRE( 1 == v.size() );
            REQUIRE( 1 == v[0] );

            // Range end
            int vs[2]{3, 4};
            v.insert(v.end(), vs, vs + 2);

            REQUIRE( 3 == v.size() );
            REQUIRE( 4 == v.capacity() );
            REQUIRE( 3 == v[1] );
            REQUIRE( 4 == v[2] );
        }

        SECTION("end dynamic resize")
        {
            v.push_back(10);
            v.push_back(20);
            v.push_back(30);

            int vs[2]{3, 4};
            v.insert(v.end(), vs, vs + 2);

            REQUIRE( 5 == v.size() );
            REQUIRE( 8 == v.capacity() );
            REQUIRE( 3 == v[3] );
            REQUIRE( 4 == v[4] );
        }

        SECTION("middle single")
        {
            v.push_back(1);
            v.push_back(2);
            v.insert(v.begin() + 1, 3);

            REQUIRE( 3 == v.size() );
            REQUIRE( 4 == v.capacity() );
            REQUIRE( 1 == v[0] );
            REQUIRE( 3 == v[1] );
            REQUIRE( 2 == v[2] );
        }

        SECTION("middle range")
        {
            v.push_back(1);
            v.push_back(2);
            int vs[2]{3, 4};
            v.insert(v.begin() + 1, vs, vs + 2);

            REQUIRE( 4 == v.size() );
            REQUIRE( 4 == v.capacity() );
            REQUIRE( 1 == v[0] );
            REQUIRE( 3 == v[1] );
            REQUIRE( 4 == v[2] );
            REQUIRE( 2 == v[3] );
        }

        SECTION("middle n values")
        {
            v.push_back(1);
            v.push_back(2);
            v.insert(v.begin() + 1, size_t(2), 9);

            REQUIRE( 4 == v.size() );
            REQUIRE( 4 == v.capacity() );
            REQUIRE( 1 == v[0] );
            REQUIRE( 9 == v[1] );
            REQUIRE( 9 == v[2] );
            REQUIRE( 2 == v[3] );
        }
    }

    SECTION("erase")
    {
        SECTION("middle single")
        {
            v.push_back(1);
            v.push_back(2);
            v.push_back(3);
            v.erase(v.begin() + 1);

            REQUIRE( 2 == v.size() );
            REQUIRE( 4 == v.capacity() );
            REQUIRE( 1 == v[0] );
            REQUIRE( 3 == v[1] );
        }

        SECTION("middle range")
        {
            v.push_back(1);
            v.push_back(2);
            v.push_back(3);
            v.push_back(4);
            v.erase(v.begin() + 1, v.begin() + 3);

            REQUIRE( 2 == v.size() );
            REQUIRE( 4 == v.capacity() );
            REQUIRE( 1 == v[0] );
            REQUIRE( 4 == v[1] );
        }
    }

    SECTION("copy/move")
    {
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);

        decltype(v) cp{v};

        REQUIRE( 3 == cp.size() );
        REQUIRE( 4 == cp.capacity() );
        REQUIRE( 1 == cp[0] );
        REQUIRE( 2 == cp[1] );
        REQUIRE( 3 == cp[2] );
        REQUIRE( 3 == v.size() );
        REQUIRE( 4 == v.capacity() );

        decltype(v) mv{std::move(v)};

        REQUIRE( 3 == mv.size() );
        REQUIRE( 4 == mv.capacity() );
        REQUIRE( 1 == mv[0] );
        REQUIRE( 2 == mv[1] );
        REQUIRE( 3 == mv[2] );
        REQUIRE( 0 == v.size() );
        REQUIRE( 4 == v.capacity() );
    }

    SECTION("copy/move assign")
    {
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);

        auto cp = v;

        REQUIRE( 3 == cp.size() );
        REQUIRE( 4 == cp.capacity() );
        REQUIRE( 1 == cp[0] );
        REQUIRE( 2 == cp[1] );
        REQUIRE( 3 == cp[2] );
        REQUIRE( 3 == v.size() );
        REQUIRE( 4 == v.capacity() );

        auto mv = std::move(v);

        REQUIRE( 3 == mv.size() );
        REQUIRE( 4 == mv.capacity() );
        REQUIRE( 1 == mv[0] );
        REQUIRE( 2 == mv[1] );
        REQUIRE( 3 == mv[2] );
        REQUIRE( 0 == v.size() );
        REQUIRE( 4 == v.capacity() );
    }
}


struct value_info {
    int constructs{};
    int destructs{};
    int copies{};
    int moves{};
};

struct fake_value {
    int v{};
    operator int() const { return v; }

    static value_info info;
    fake_value() : v{} { ++info.constructs; }
    fake_value(int v) : v{v} { ++info.constructs; }
    ~fake_value() { ++info.destructs; }
    fake_value(const fake_value& o) { v = o.v; ++info.copies; }
    fake_value(fake_value&& o) noexcept { v = o.v; o.v = {}; ++info.moves; }
    fake_value& operator=(const fake_value& o) { v = o.v; ++info.copies; return *this; }
    fake_value& operator=(fake_value&& o) noexcept { v = o.v; o.v = {}; ++info.moves; return *this; }

    static void reset() { info = {}; }

};
value_info fake_value::info{};

TEST_CASE("svector RAII", "[Utilities]")
{
    // TODO: Test insert, erase, resize and shrink to fit RAII

    SECTION("single value")
    {
        zen::svector<fake_value, 4> v;

        SECTION("emplace back")
        {
            v.emplace_back(1);
            REQUIRE( 1 == fake_value::info.constructs );
            REQUIRE( 0 == fake_value::info.copies );
            REQUIRE( 0 == fake_value::info.moves );
            REQUIRE( 0 == fake_value::info.destructs );
        }

        SECTION("push back move")
        {
            v.push_back(1);
            REQUIRE( 1 == fake_value::info.constructs );
            REQUIRE( 0 == fake_value::info.copies );
            REQUIRE( 1 == fake_value::info.moves );
            REQUIRE( 1 == fake_value::info.destructs );
        }

        SECTION("push back copy")
        {
            fake_value val{2};
            v.push_back(val);
            REQUIRE( 1 == fake_value::info.constructs );
            REQUIRE( 1 == fake_value::info.copies );
            REQUIRE( 0 == fake_value::info.moves );
            REQUIRE( 0 == fake_value::info.destructs );
        }

        SECTION("dynamic resize - move")
        {
            for (int i = 0; i < 4; ++i)
                v.emplace_back(i);
            fake_value::reset();

            // Dynamic resize - move
            v.emplace_back(4);

            REQUIRE( 1 == fake_value::info.constructs );
            REQUIRE( 0 == fake_value::info.destructs );
            REQUIRE( 4 == fake_value::info.moves );
        }
    }

    SECTION("single value O(N) operations")
    {
        zen::svector<fake_value, 4> v;
    }

    SECTION("nested values")
    {
        zen::svector<zen::svector<fake_value, 4>, 4> v;
    }

    fake_value::reset();
}
