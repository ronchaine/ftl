#include "../doctest.h"
#include "../test_common.hpp"
#include <type_traits>
#include <string>
#include <ftl/ring_buffer.hpp>

template <typename T>
using std_alloc_ring_buffer = ftl::ring_buffer<T, std::allocator<T>>;

TEST_SUITE("ftl::ring buffer with allocator") {
    TEST_CASE("static requirements (builtin value)") {
        SUBCASE("ring buffer is nothrow constructible and assignable") {
            CHECK(std::is_nothrow_constructible<std_alloc_ring_buffer<int>>::value);
            CHECK(std::is_nothrow_copy_constructible<std_alloc_ring_buffer<int>>::value);
            CHECK(std::is_nothrow_move_constructible<std_alloc_ring_buffer<int>>::value);
            CHECK(std::is_nothrow_copy_assignable<std_alloc_ring_buffer<int>>::value);
            CHECK(std::is_nothrow_move_assignable<std_alloc_ring_buffer<int>>::value);
        }
    }

    TEST_CASE("static requirements (trivial value)") {
        SUBCASE("ring buffer is nothrow constructible and assignable") {
            CHECK(std::is_nothrow_constructible<std_alloc_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_nothrow_copy_constructible<std_alloc_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_nothrow_move_constructible<std_alloc_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_nothrow_copy_assignable<std_alloc_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_nothrow_move_assignable<std_alloc_ring_buffer<ftl_test::trivial_type>>::value);
        }

        SUBCASE("ring buffer is trivially assignable") {
            CHECK(std::is_trivially_copy_assignable<std_alloc_ring_buffer<ftl_test::trivial_type>>::value);
            CHECK(std::is_trivially_move_assignable<std_alloc_ring_buffer<ftl_test::trivial_type>>::value);
        }
    }

    TEST_CASE("static requirements (non-trivial value)") {
        SUBCASE("ring buffer is copyable / movable") {
            CHECK(std::is_copy_constructible<std_alloc_ring_buffer<ftl_test::nontrivial_type>>::value);
            CHECK(std::is_move_constructible<std_alloc_ring_buffer<ftl_test::nontrivial_type>>::value);
            CHECK(std::is_copy_assignable<std_alloc_ring_buffer<ftl_test::nontrivial_type>>::value);
            CHECK(std::is_move_assignable<std_alloc_ring_buffer<ftl_test::nontrivial_type>>::value);
        }
    }

    TEST_CASE("sizeof(ring_buffer) does not vary by value_type") {
        std_alloc_ring_buffer<int> test_buf_int;
        std_alloc_ring_buffer<char> test_buf_char;
        std_alloc_ring_buffer<std::string> test_buf_string;

        CHECK(sizeof(test_buf_int) == sizeof(test_buf_string));
        CHECK(sizeof(test_buf_int) == sizeof(test_buf_char));
    }

    TEST_CASE("capacity, size and emptiness / fullness") {
        SUBCASE("Default-constructed ring buffer is empty and full, allocation on first push") {
            std_alloc_ring_buffer<int> test_buffer;
            CHECK(test_buffer.is_empty());
            CHECK(test_buffer.is_full());
            CHECK(test_buffer.capacity() == 0);

            test_buffer.push(1);
            CHECK(test_buffer.capacity() > 0);
            CHECK(not test_buffer.is_empty());
            CHECK(not test_buffer.is_full());
        }

        SUBCASE("Filling a buffer makes it full and reading frees space") {
            std_alloc_ring_buffer<int> test_buffer;

            test_buffer.push(0); // init the buffer
            for (int i = 1; i < static_cast<int>(test_buffer.capacity()); ++i)
                test_buffer.push(i);

            CHECK(test_buffer.is_full());
            CHECK(not test_buffer.is_empty());

            int i = test_buffer.pop();
            (void)i;

            CHECK(not test_buffer.is_full());
        }

        SUBCASE("For full buffer size() == capacity()") {
            std_alloc_ring_buffer<int> buffer;

            REQUIRE(buffer.is_full());
            CHECK(buffer.size() == buffer.capacity());
        }
    }

    // TODO: It would be nice to make this a test case template at some point,
    //       this is repeated pretty much verbatim in ring_buffer_allocated.cpp
    TEST_CASE("construction / destruction of contained objects") {
        SUBCASE("Allocating storage doesn't cause object initialisation") {
            using counter_type = ftl_test::counted_ctr_dtr<"arb-cdc-0">;
            std_alloc_ring_buffer<counter_type> test_buf;

            REQUIRE(counter_type::default_constructed == 0);
            REQUIRE(counter_type::copy_constructed == 0);
            REQUIRE(counter_type::move_constructed == 0);
            REQUIRE(counter_type::destroyed == 0);
        }

        SUBCASE("push/pop with copy semantics") {
            ftl_test::counted_ctr_dtr<"arb-pp-copy"> counter;
            {
                std_alloc_ring_buffer<decltype(counter)> test_buf;

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
            ftl_test::counted_ctr_dtr<"arb-pp-move"> counter;
            {
                std_alloc_ring_buffer<decltype(counter)> test_buf;

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
