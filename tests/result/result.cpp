#include "../doctest.h"
#include "../test_common.hpp"
#include <type_traits>
#include <string>
#include <ftl/result.hpp>

using simple_result = ftl::result<int, int>;
using trivial_result = ftl::result<ftl_test::trivial_type, ftl_test::trivial_type>;
using nontrivial_result = ftl::result<ftl_test::nontrivial_type, ftl_test::nontrivial_type>;

TEST_SUITE("ftl::result") {
    TEST_CASE("static requirements (builtin type)") {
        SUBCASE("result is nothrow constructible and assignable") {
            CHECK(std::is_nothrow_copy_constructible<simple_result>::value);
            CHECK(std::is_nothrow_move_constructible<simple_result>::value);
            CHECK(std::is_nothrow_copy_assignable<simple_result>::value);
            CHECK(std::is_nothrow_move_assignable<simple_result>::value);
        }

        SUBCASE("result is trivially copyable/movable/destructible if its template arguments are") {
            CHECK(std::is_trivially_copy_constructible<simple_result>::value);
            CHECK(std::is_trivially_move_constructible<simple_result>::value);
            CHECK(std::is_trivially_copy_assignable<simple_result>::value);
            CHECK(std::is_trivially_move_assignable<simple_result>::value);
            CHECK(std::is_trivially_destructible<simple_result>::value);

            CHECK(std::is_trivially_copy_constructible<trivial_result>::value);
            CHECK(std::is_trivially_move_constructible<trivial_result>::value);
            CHECK(std::is_trivially_copy_assignable<trivial_result>::value);
            CHECK(std::is_trivially_move_assignable<trivial_result>::value);
            CHECK(std::is_trivially_destructible<trivial_result>::value);
        }

        SUBCASE("Not assignable from plain value/error types") {
            CHECK(not std::is_constructible_v<simple_result, simple_result::value_type>);
            CHECK(not std::is_constructible_v<simple_result, simple_result::error_type>);
        }

        SUBCASE("result type can be correctly constructed from 'ok' or 'error' types") {
            CHECK(not std::is_default_constructible<simple_result>::value);
            CHECK(not std::is_default_constructible<trivial_result>::value);

            simple_result res_ok = ftl::ok{ 24 };
            simple_result res_err = ftl::error{ 12 };

            CHECK(res_ok.is_ok());
            CHECK(not res_ok.is_error());

            CHECK(res_err.is_error());
            CHECK(not res_err.is_ok());

            CHECK(res_ok.contains(24));
            CHECK(res_err.contains_error(12));
            CHECK(not res_ok.contains_error(24));
            CHECK(not res_err.contains(12));

            simple_result::value_type retrieved_value = res_ok.value();
            CHECK(retrieved_value == 24);

            simple_result::error_type retrieved_error = res_err.error();
            CHECK(retrieved_error == 12);
        }

        SUBCASE("Extra copies are not made (value)") {
            using counter_type = ftl_test::counted_ctr_dtr<"result-access-0">;
            using test_type = ftl::result<counter_type, int>;

            test_type test_ok = ftl::ok{counter_type{}};

            CHECK(counter_type::default_constructed == 1);
            CHECK(counter_type::copy_constructed == 0);
        }

        SUBCASE("Extra copies are not made (error)") {
            using counter_type = ftl_test::counted_ctr_dtr<"result-access-1">;
            using test_type = ftl::result<int, counter_type>;

            test_type test_err = ftl::error{counter_type{}};

            CHECK(counter_type::default_constructed == 1);
            CHECK(counter_type::copy_constructed == 0);
        }
    }
}
