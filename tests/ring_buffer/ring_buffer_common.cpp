#include "../doctest.h"
#include "../test_common.hpp"
#include <type_traits>
#include <string>
#include <ftl/ring_buffer.hpp>

template <typename T>
using static_ring_buffer = ftl::ring_buffer<T, ftl::static_storage<16>>;

template <typename T>
using std_alloc_ring_buffer = ftl::ring_buffer<T, std::allocator<T>>;

TYPE_TO_STRING(static_ring_buffer<int>);
TYPE_TO_STRING(std_alloc_ring_buffer<int>);

TEST_SUITE("ring buffer common functionality") {
    TEST_CASE_TEMPLATE("Pushing / popping elements", T, static_ring_buffer<int>, std_alloc_ring_buffer<int>) {
        SUBCASE("Pushing increases buffer size by 1") {
            T test_buf;
            REQUIRE(test_buf.size() == 0);

            test_buf.push(1);
            CHECK(test_buf.size() == 1);

            test_buf.push(2);
            CHECK(test_buf.size() == 2);

            test_buf.push(3);
            test_buf.push(4);
            CHECK(test_buf.size() == 4);
        }

        SUBCASE("Popping decreases buffer size by 1") {
            T test_buf;

            test_buf.push(1);
            test_buf.push(2);
            test_buf.push(3);
            test_buf.push(4);

            REQUIRE(test_buf.size() == 4);

            int a;
            a = test_buf.pop(); (void)a;
            CHECK(test_buf.size() == 3);

            a = test_buf.pop(); (void)a;
            CHECK(test_buf.size() == 2);

            a = test_buf.pop(); (void)a;
            CHECK(test_buf.size() == 1);

            a = test_buf.pop(); (void)a;
            CHECK(test_buf.size() == 0);
        }
        SUBCASE("Push/pop wraps correctly") {
            T test_buf;

            int count = 0;
            test_buf.push(0);
            while(not test_buf.is_full())
                test_buf.push(++count);

            int filled_size = test_buf.size();

            int a = test_buf.pop(); (void)a;
            test_buf.push(filled_size);

            count = 1;
            while(not test_buf.is_empty())
                CHECK(test_buf.pop() == count++);
        }
    }

    TEST_CASE_TEMPLATE("capacity, size and emptiness / fullness", T, static_ring_buffer<int>, std_alloc_ring_buffer<int>) {
        SUBCASE("Default-constructed ring buffer is empty") {
            T fresh_unmodified_buffer;
            CHECK(fresh_unmodified_buffer.is_empty());
        }

        SUBCASE("Filling a buffer makes it full and reading frees space") {
            T test_buffer;
            test_buffer.push(0);
            for (int i = 1; i < static_cast<int>(test_buffer.capacity()); ++i)
                test_buffer.push(i);

            CHECK(test_buffer.is_full());
            CHECK(not test_buffer.is_empty());

            int i = test_buffer.pop();
            (void)i;

            CHECK(not test_buffer.is_full());
        }

        SUBCASE("Popping a final element in a buffer makes it empty") {
            T test_buffer;
            test_buffer.push(0);

            int i = test_buffer.pop(); (void)i;
            CHECK(test_buffer.is_empty());
        }

        SUBCASE("For full buffer size() == capacity()") {
            T test_buffer;

            test_buffer.push(0);
            while(not test_buffer.is_full())
                test_buffer.push(0);

            CHECK(test_buffer.size() == test_buffer.capacity());
        }
    }
}
