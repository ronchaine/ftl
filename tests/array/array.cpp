#include "../doctest.h"
#include "../test_common.hpp"
#include <type_traits>
#include <string>
#include <ftl/array.hpp>

TEST_SUITE("ftl::array") {
    TEST_CASE("static requirements (builtin type)") {
        SUBCASE("array is nothrow constructible and assignable") {
            CHECK(std::is_nothrow_constructible<ftl::array<int, 2>>::value);
            CHECK(std::is_nothrow_copy_constructible<ftl::array<int, 2>>::value);
            CHECK(std::is_nothrow_move_constructible<ftl::array<int, 2>>::value);
            CHECK(std::is_nothrow_copy_assignable<ftl::array<int, 2>>::value);
            CHECK(std::is_nothrow_move_assignable<ftl::array<int, 2>>::value);

            CHECK(std::is_nothrow_constructible<ftl::array<int, 2, 4>>::value);
            CHECK(std::is_nothrow_copy_constructible<ftl::array<int, 2, 4>>::value);
            CHECK(std::is_nothrow_move_constructible<ftl::array<int, 2, 4>>::value);
            CHECK(std::is_nothrow_copy_assignable<ftl::array<int, 2, 4>>::value);
            CHECK(std::is_nothrow_move_assignable<ftl::array<int, 2, 4>>::value);
        }

        SUBCASE("triviality") {
            CHECK(std::is_trivially_copy_constructible<ftl::array<int, 2>>::value);
            CHECK(std::is_trivially_move_constructible<ftl::array<int, 2>>::value);
            CHECK(std::is_trivially_copy_assignable<ftl::array<int, 2>>::value);
            CHECK(std::is_trivially_move_assignable<ftl::array<int, 2>>::value);

            CHECK(std::is_trivially_copy_constructible<ftl::array<int, 2, 4>>::value);
            CHECK(std::is_trivially_move_constructible<ftl::array<int, 2, 4>>::value);
            CHECK(std::is_trivially_copy_assignable<ftl::array<int, 2, 4>>::value);
            CHECK(std::is_trivially_move_assignable<ftl::array<int, 2, 4>>::value);
        }
    }

    TEST_CASE("static requirements (trivial value)") {
        SUBCASE("ring buffer is nothrow constructible and assignable") {
            CHECK(std::is_nothrow_constructible<ftl::array<ftl_test::trivial_type, 2>>::value);
            CHECK(std::is_nothrow_copy_constructible<ftl::array<ftl_test::trivial_type, 2>>::value);
            CHECK(std::is_nothrow_move_constructible<ftl::array<ftl_test::trivial_type, 2>>::value);
            CHECK(std::is_nothrow_copy_assignable<ftl::array<ftl_test::trivial_type, 2>>::value);
            CHECK(std::is_nothrow_move_assignable<ftl::array<ftl_test::trivial_type, 2>>::value);

            CHECK(std::is_nothrow_constructible<ftl::array<ftl_test::trivial_type, 2, 4>>::value);
            CHECK(std::is_nothrow_copy_constructible<ftl::array<ftl_test::trivial_type, 2, 3>>::value);
            CHECK(std::is_nothrow_move_constructible<ftl::array<ftl_test::trivial_type, 2, 1>>::value);
            CHECK(std::is_nothrow_copy_assignable<ftl::array<ftl_test::trivial_type, 2, 5>>::value);
            CHECK(std::is_nothrow_move_assignable<ftl::array<ftl_test::trivial_type, 2, 2>>::value);
        }

        SUBCASE("ring buffer is trivially copyable / movable") {
            CHECK(std::is_trivially_copy_constructible<ftl::array<ftl_test::trivial_type, 2>>::value);
            CHECK(std::is_trivially_move_constructible<ftl::array<ftl_test::trivial_type, 2>>::value);
            CHECK(std::is_trivially_copy_assignable<ftl::array<ftl_test::trivial_type, 2>>::value);
            CHECK(std::is_trivially_move_assignable<ftl::array<ftl_test::trivial_type, 2>>::value);

            CHECK(std::is_trivially_copy_constructible<ftl::array<ftl_test::trivial_type, 2, 4>>::value);
            CHECK(std::is_trivially_move_constructible<ftl::array<ftl_test::trivial_type, 2, 3>>::value);
            CHECK(std::is_trivially_copy_assignable<ftl::array<ftl_test::trivial_type, 2, 2>>::value);
            CHECK(std::is_trivially_move_assignable<ftl::array<ftl_test::trivial_type, 2, 5, 1>>::value);
        }
    }

    TEST_CASE("static requirements (non-trivial value)") {
        SUBCASE("ring buffer is copyable / movable") {
            CHECK(std::is_copy_constructible<ftl::array<ftl_test::nontrivial_type, 2>>::value);
            CHECK(std::is_move_constructible<ftl::array<ftl_test::nontrivial_type, 2>>::value);
            CHECK(std::is_copy_assignable<ftl::array<ftl_test::nontrivial_type, 2>>::value);
            CHECK(std::is_move_assignable<ftl::array<ftl_test::nontrivial_type, 2>>::value);
        }
    }

    TEST_CASE("No memory overhead over similar C-style array") {
        int c_int2x2[2][2];
        ftl::array<int, 2, 2> int2x2;

        CHECK(sizeof(int2x2) == sizeof(c_int2x2));
    }

    TEST_CASE("Multidimensional access overhead") {
        using counter_type = ftl_test::counted_ctr_dtr<"array-access-0">;
        ftl::array<counter_type, 2, 2> mdarray;

        REQUIRE(counter_type::default_constructed == 4);
        REQUIRE(counter_type::copy_constructed == 0);
        REQUIRE(counter_type::move_constructed == 0);
        REQUIRE(counter_type::destroyed == 0);


        counter_type retrieve_copy = mdarray[1][1];
        CHECK(counter_type::default_constructed == 4);
        CHECK(counter_type::copy_constructed == 1); // no additional copies
        CHECK(counter_type::move_constructed == 0);
        CHECK(counter_type::destroyed == 0);

        counter_type retrieve_move = FTL_MOVE(mdarray[1][1]);
        CHECK(counter_type::default_constructed == 4);
        CHECK(counter_type::copy_constructed == 1); // no additional copies
        CHECK(counter_type::move_constructed == 1); // no additional moves
        CHECK(counter_type::destroyed == 0);
    }
}
/*
    Copyright 2022 Jari Ronkainen

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
    associated documentation files (the "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions
    of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
    INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
    PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
    OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/
