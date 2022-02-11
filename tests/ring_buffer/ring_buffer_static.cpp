#include "../doctest.h"
#include "../test_common.hpp"
#include <type_traits>
#include <string>
#include <ftl/ring_buffer.hpp>

template <typename T>
using static_ring_buffer = ftl::ring_buffer<T, ftl::static_storage<16>>;

TEST_SUITE("ftl::ring buffer with static_storage") {
    TEST_CASE("static requirements (builtin value)") {
        SUBCASE("ring buffer is nothrow constructible and assignable") {
            CHECK(std::is_nothrow_constructible<static_ring_buffer<int>>::value);
            CHECK(std::is_nothrow_copy_constructible<static_ring_buffer<int>>::value);
            CHECK(std::is_nothrow_move_constructible<static_ring_buffer<int>>::value);
            CHECK(std::is_nothrow_copy_assignable<static_ring_buffer<int>>::value);
            CHECK(std::is_nothrow_move_assignable<static_ring_buffer<int>>::value);
        }

        SUBCASE("triviality") {
            CHECK(std::is_trivially_copy_constructible<static_ring_buffer<int>>::value);
            CHECK(std::is_trivially_move_constructible<static_ring_buffer<int>>::value);
            CHECK(std::is_trivially_copy_assignable<static_ring_buffer<int>>::value);
            CHECK(std::is_trivially_move_assignable<static_ring_buffer<int>>::value);
        }
    }

    TEST_CASE("static requirements (trivial value)") {
        SUBCASE("ring buffer is nothrow constructible and assignable") {
            CHECK(std::is_nothrow_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_nothrow_copy_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_nothrow_move_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_nothrow_copy_assignable<static_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_nothrow_move_assignable<static_ring_buffer<ftl_test::trivial_type>>::value);
        }

        SUBCASE("ring buffer is trivially copyable / movable") {
            CHECK(std::is_trivially_copy_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_trivially_move_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_trivially_copy_assignable<static_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_trivially_move_assignable<static_ring_buffer<ftl_test::trivial_type>>::value);
        }
    }

    TEST_CASE("static requirements (non-trivial value)") {
        SUBCASE("ring buffer is copyable / movable") {
            CHECK(std::is_copy_constructible<static_ring_buffer<ftl_test::nontrivial_type>>::value);
            CHECK(std::is_move_constructible<static_ring_buffer<ftl_test::nontrivial_type>>::value);
            CHECK(std::is_copy_assignable<static_ring_buffer<ftl_test::nontrivial_type>>::value);
            CHECK(std::is_move_assignable<static_ring_buffer<ftl_test::nontrivial_type>>::value);
        }
    }

    TEST_CASE("memory overhead") {
        ftl::ring_buffer<int, ftl::static_storage<16>> test_buf_i16;
        ftl::ring_buffer<int, ftl::static_storage<32>> test_buf_i32;
        ftl::ring_buffer<char, ftl::static_storage<8>> test_buf_c8;

        ftl::ring_buffer<std::string, ftl::static_storage<8>> test_buf_s8;

        // Overhead for static ring buf at most two pointers + the size of the storage
        CHECK(sizeof(test_buf_i16) <= sizeof(int) * 16 + sizeof(int*) * 2);
        CHECK(sizeof(test_buf_i32) <= sizeof(int) * 32 + sizeof(int*) * 2);
        CHECK(sizeof(test_buf_c8) <= sizeof(char) * 8 + sizeof(char*) * 2);
        CHECK(sizeof(test_buf_s8) <= sizeof(std::string) * 8 + sizeof(std::string*) * 2);

        // Check we remembered to actually include the bookkeeping pointers and init the array to right size
        REQUIRE(sizeof(test_buf_i16) > sizeof(int) * 16);
        REQUIRE(sizeof(test_buf_i32) > sizeof(int) * 32);
        REQUIRE(sizeof(test_buf_c8) > sizeof(char) * 8);
        REQUIRE(sizeof(test_buf_s8) > sizeof(std::string) * 8);
    }

    TEST_CASE("capacity and size") {
        SUBCASE("Capacity matches requested buffer size") {
            ftl::ring_buffer<int, ftl::static_storage<16>> test_buf_i16;
            ftl::ring_buffer<std::string, ftl::static_storage<32>> test_buf_s32;
            ftl::ring_buffer<char, ftl::static_storage<8>> test_buf_c8;
            CHECK(test_buf_i16.capacity() == 16);
            CHECK(test_buf_s32.capacity() == 32);
            CHECK(test_buf_c8.capacity() == 8);
        }
        SUBCASE("Type is large enough to store requested amount of elements") {
            ftl::ring_buffer<int, ftl::static_storage<16>> test_buf_i16;
            ftl::ring_buffer<std::string, ftl::static_storage<32>> test_buf_s32;
            ftl::ring_buffer<char, ftl::static_storage<8>> test_buf_c8;

            CHECK(sizeof(test_buf_i16) > sizeof(int) * 16);
            CHECK(sizeof(test_buf_s32) > sizeof(std::string) * 32);
            CHECK(sizeof(test_buf_c8) > sizeof(char) * 8);
        }
    }

    // TODO: It would be nice to make this a test case template at some point,
    //       this is repeated pretty much verbatim in ring_buffer_allocated.cpp
    TEST_CASE("construction / destruction of contained objects") {
        SUBCASE("Allocating storage doesn't cause object initialisation") {
            using counter_type = ftl_test::counted_ctr_dtr<"srb-cdc-0">;
            ftl::ring_buffer<counter_type, ftl::static_storage<16>> test_buf;

            REQUIRE(counter_type::default_constructed == 0);
            REQUIRE(counter_type::copy_constructed == 0);
            REQUIRE(counter_type::move_constructed == 0);
            REQUIRE(counter_type::destroyed == 0);
        }

        SUBCASE("push/pop with copy semantics") {
            ftl_test::counted_ctr_dtr<"srb-pp-copy"> counter;
            {
                ftl::ring_buffer<decltype(counter), ftl::static_storage<6>> test_buf;

                REQUIRE(counter.default_constructed == 1);
                REQUIRE(counter.copy_constructed == 0);
                REQUIRE(counter.move_constructed == 0);
                REQUIRE(counter.destroyed == 0);

                // check that pushing doesn't make extra copies (can't make this a
                // subcase, since it would affect the counter)
                test_buf.push(counter); // first instance
                CHECK(counter.default_constructed == 1);
                CHECK(counter.copy_constructed == 1);
                CHECK(counter.move_constructed == 0);
                CHECK(counter.destroyed == 0);

                test_buf.push(counter); // second instance
                CHECK(counter.default_constructed == 1);
                CHECK(counter.copy_constructed == 2);
                CHECK(counter.move_constructed == 0);
                CHECK(counter.destroyed == 0);

                // check popping moves the value and calls the destructor}
                auto ctr = test_buf.pop(); // moved value = 3rd instance
                (void)ctr;
                CHECK(counter.default_constructed == 1);
                CHECK(counter.copy_constructed == 2);
                CHECK(counter.move_constructed == 1);
                CHECK(counter.destroyed == 1);
            };

            // destructors called correctly for all 3 existing instances
            CHECK(counter.destroyed == 3);
        }

        SUBCASE("push/pop with move semantics") {
            ftl_test::counted_ctr_dtr<"srb-pp-move"> counter;
            {
                ftl::ring_buffer<decltype(counter), ftl::static_storage<6>> test_buf;

                REQUIRE(counter.default_constructed == 1);
                REQUIRE(counter.copy_constructed == 0);
                REQUIRE(counter.move_constructed == 0);
                REQUIRE(counter.destroyed == 0);

                // check that pushing doesn't make extra copies (can't make this a
                // subcase, since it would affect the counter)
                test_buf.push(FTL_MOVE(counter)); // first instance
                CHECK(counter.default_constructed == 1);
                CHECK(counter.copy_constructed == 0);
                CHECK(counter.move_constructed == 1);
                CHECK(counter.destroyed == 0);

                test_buf.push(FTL_MOVE(counter)); // second instance
                CHECK(counter.default_constructed == 1);
                CHECK(counter.copy_constructed == 0);
                CHECK(counter.move_constructed == 2);
                CHECK(counter.destroyed == 0);

                // check popping moves the value and calls the destructor}
                auto ctr = test_buf.pop(); // moved value = 3rd instance
                (void)ctr;
                CHECK(counter.default_constructed == 1);
                CHECK(counter.copy_constructed == 0);
                CHECK(counter.move_constructed == 3);
                CHECK(counter.destroyed == 1);
            };

            // destructors called correctly for all 3 existing instances
            CHECK(counter.destroyed == 3);
        }
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
