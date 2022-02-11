#include "../doctest.h"
#include "../test_common.hpp"
#include <type_traits>
#include <string>
#include <ftl/ring_buffer.hpp>

template <typename T>
using static_ring_buffer = ftl::ring_buffer<T, ftl::static_storage<16>>;

TEST_CASE("ftl::ring buffer with static_storage - basic traits") {
    SUBCASE("constructibility / assignability requirements (builtin value)") {
        REQUIRE(std::is_nothrow_constructible<static_ring_buffer<int>>::value);
        REQUIRE(std::is_nothrow_copy_constructible<static_ring_buffer<int>>::value);
        REQUIRE(std::is_nothrow_move_constructible<static_ring_buffer<int>>::value);
        REQUIRE(std::is_nothrow_copy_assignable<static_ring_buffer<int>>::value);
        REQUIRE(std::is_nothrow_move_assignable<static_ring_buffer<int>>::value);

        REQUIRE(std::is_trivially_copy_constructible<static_ring_buffer<int>>::value);
        REQUIRE(std::is_trivially_move_constructible<static_ring_buffer<int>>::value);
        REQUIRE(std::is_trivially_copy_assignable<static_ring_buffer<int>>::value);
        REQUIRE(std::is_trivially_move_assignable<static_ring_buffer<int>>::value);
    }

    SUBCASE("constructibility / assignability requirements (trivial value)") {
        REQUIRE(std::is_nothrow_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
        REQUIRE(std::is_nothrow_copy_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
        REQUIRE(std::is_nothrow_move_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
        REQUIRE(std::is_nothrow_copy_assignable<static_ring_buffer<ftl_test::trivial_type>>::value);
        REQUIRE(std::is_nothrow_move_assignable<static_ring_buffer<ftl_test::trivial_type>>::value);

        REQUIRE(std::is_trivially_copy_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
        REQUIRE(std::is_trivially_move_constructible<static_ring_buffer<ftl_test::trivial_type>>::value);
        REQUIRE(std::is_trivially_copy_assignable<static_ring_buffer<ftl_test::trivial_type>>::value);
        REQUIRE(std::is_trivially_move_assignable<static_ring_buffer<ftl_test::trivial_type>>::value);
    }

    SUBCASE("constructibility / assignability requirements (non-trivial value)") {
        REQUIRE(std::is_nothrow_constructible<static_ring_buffer<ftl_test::nontrivial_type>>::value);
        REQUIRE(std::is_copy_constructible<static_ring_buffer<ftl_test::nontrivial_type>>::value);
        REQUIRE(std::is_move_constructible<static_ring_buffer<ftl_test::nontrivial_type>>::value);
        REQUIRE(std::is_copy_assignable<static_ring_buffer<ftl_test::nontrivial_type>>::value);
        REQUIRE(std::is_move_assignable<static_ring_buffer<ftl_test::nontrivial_type>>::value);
    }

    SUBCASE("properly destroying contained objects") {
        // causes default_constructed to increase to 1
        ftl_test::counted_ctr_dtr<"static ftl::ring_buffer tests"> counter;

        ftl::ring_buffer<decltype(counter), ftl::static_storage<16>> test_buf;

        // elements in static ring buffer are not default_constructed
        REQUIRE(counter.default_constructed == 1);

        /*
        // Verify initial state
        REQUIRE(counter.copy_constructed == 0);
        REQUIRE(counter.move_constructed == 0);
        REQUIRE(counter.destroyed == 0);

        {
            test_buf.push(counter);
            REQUIRE(counter.default_constructed == 1);
            REQUIRE(counter.copy_constructed == 1);
            REQUIRE(counter.move_constructed == 0);

            test_buf.push(static_cast<decltype(counter)&&>(counter));
            REQUIRE(counter.default_constructed == 1);
            REQUIRE(counter.copy_constructed == 1);
            REQUIRE(counter.move_constructed <= 1);

            auto c_count = counter.move_constructed;

            // pop move constructs and deletes the element
            auto check = test_buf.pop();
            REQUIRE(counter.move_constructed == c_count + 1);
            REQUIRE(counter.destroyed == 1);
        }
        REQUIRE(counter.destroyed == 2);
        */
    }

    SUBCASE("overhead") {
        ftl::ring_buffer<int, ftl::static_storage<16>> test_buf_i16;
        ftl::ring_buffer<int, ftl::static_storage<32>> test_buf_i32;
        ftl::ring_buffer<char, ftl::static_storage<8>> test_buf_c8;

        ftl::ring_buffer<std::string, ftl::static_storage<8>> test_buf_s8;

        // Overhead for static ring buf at most two pointers + the size of the storage
        REQUIRE(sizeof(test_buf_i16) <= sizeof(int) * 16 + sizeof(int*) * 2);
        REQUIRE(sizeof(test_buf_i32) <= sizeof(int) * 32 + sizeof(int*) * 2);
        REQUIRE(sizeof(test_buf_c8) <= sizeof(char) * 8 + sizeof(char*) * 2);
        REQUIRE(sizeof(test_buf_s8) <= sizeof(std::string) * 8 + sizeof(std::string*) * 2);

        // Check we remembered to actually include the bookkeeping pointers and init the array to right size
        REQUIRE(sizeof(test_buf_i16) > sizeof(int) * 16);
        REQUIRE(sizeof(test_buf_i32) > sizeof(int) * 32);
        REQUIRE(sizeof(test_buf_c8) > sizeof(char) * 8);
        REQUIRE(sizeof(test_buf_s8) > sizeof(std::string) * 8);
    }

    SUBCASE("assignment and copy/move initialisation") {
        ftl::ring_buffer<int, ftl::static_storage<8>> buf0;
        ftl::ring_buffer<int, ftl::static_storage<8>> buf1;

        for (int i = 0; i < 8; ++i) {
            buf0.push(i);
            buf1.push(7-i);
        }

        {
            ftl::ring_buffer<int, ftl::static_storage<8>> tmp_buf;
            ftl::ring_buffer<int, ftl::static_storage<8>> tmp_buf2;

            tmp_buf = buf1;
            tmp_buf2 = std::move(buf1);

            for (int i = 0; i < 8; ++i) {
                REQUIRE(tmp_buf.pop() == tmp_buf2.pop());
            }
        }

        { // copy init
            ftl::ring_buffer<int, ftl::static_storage<8>> tmp_buf = buf0;
            ftl::ring_buffer<int, ftl::static_storage<8>> comparison_buffer = buf0;

            for (int i = 0; i < 8; ++i) {
                REQUIRE(tmp_buf.pop() == comparison_buffer.pop());
            }
        }

        { // move init
            ftl::ring_buffer<int, ftl::static_storage<8>> tmp_buf = buf0;
            ftl::ring_buffer<int, ftl::static_storage<8>> comparison_buffer = std::move(tmp_buf);
            tmp_buf = buf0;

            for (int i = 0; i < 8; ++i) {
                REQUIRE(tmp_buf.pop() == comparison_buffer.pop());
            }
        }
    }

    SUBCASE("pushing / popping values (builtin value)") {
        static_ring_buffer<int> test_ring;

        test_ring.push(1);
        REQUIRE(test_ring.pop() == 1);

        test_ring.push(2);
        test_ring.push(3);
        REQUIRE(test_ring.pop() == 2);
        REQUIRE(test_ring.pop() == 3);

        test_ring.push(4);
        test_ring.push(5);
        REQUIRE(test_ring.pop() == 4);
        test_ring.push(6);
        REQUIRE(test_ring.pop() == 5);
        REQUIRE(test_ring.pop() == 6);

        // this should be enough to wrap
        for (int i = 7; i < 18; ++i)
            test_ring.push(i);

        // read all remaining
        int i = 7;
        while(not test_ring.is_empty())
            REQUIRE(test_ring.pop() == i++);

        REQUIRE(test_ring.is_empty());
    }

    SUBCASE("capacity and emptiness / fullness") {
        // static buffer capacity comes straight from initialisation
        ftl::ring_buffer<int, ftl::static_storage<16>> test_buf_i16;
        ftl::ring_buffer<int, ftl::static_storage<32>> test_buf_i32;
        ftl::ring_buffer<char, ftl::static_storage<8>> test_buf_c8;

        ftl::ring_buffer<std::string, ftl::static_storage<8>> test_buf_s8;

        REQUIRE(test_buf_i16.capacity() == 16);
        REQUIRE(test_buf_i32.capacity() == 32);
        REQUIRE(test_buf_c8.capacity() == 8);
        REQUIRE(test_buf_s8.capacity() == 8);

        // static ring buffer is empty (and thus cannot be full) before modification
        ftl::ring_buffer<char, ftl::static_storage<8>> fresh_unmodified_buffer;
        REQUIRE(fresh_unmodified_buffer.is_empty());
        REQUIRE(not fresh_unmodified_buffer.is_full());

        for (int i = 0; i < static_cast<int>(test_buf_i16.capacity()); ++i)
            test_buf_i16.push(i);

        REQUIRE(test_buf_i16.is_full());

        int i = test_buf_i16.pop();
        (void)i;

        REQUIRE(not test_buf_i16.is_full());
    }

    SUBCASE("swap") {
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
