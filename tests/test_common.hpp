#ifndef FTL_TEST_COMMON_HPP
#define FTL_TEST_COMMON_HPP

#include <cstdint>

namespace ftl_test
{
    struct trivial_type {
        trivial_type() = default;
        trivial_type(const trivial_type&) = default;
        trivial_type(trivial_type&&) = default;
        trivial_type& operator=(const trivial_type&) = default;
        trivial_type& operator=(trivial_type&&) = default;
        ~trivial_type() = default;
    };

    struct nontrivial_noexcept_type {
        nontrivial_noexcept_type() noexcept {};
        nontrivial_noexcept_type(const nontrivial_noexcept_type&) noexcept {};
        nontrivial_noexcept_type(nontrivial_noexcept_type&&) noexcept {};
        nontrivial_noexcept_type& operator=(const nontrivial_noexcept_type&) noexcept { return *this; };
        nontrivial_noexcept_type& operator=(nontrivial_noexcept_type&&) noexcept { return *this; }
        ~nontrivial_noexcept_type() {};
    };

    struct nontrivial_type {
        nontrivial_type() {};
        nontrivial_type(const nontrivial_type&) {};
        nontrivial_type(nontrivial_type&&) {};
        nontrivial_type& operator=(const nontrivial_type&) { return *this; };
        nontrivial_type& operator=(nontrivial_type&&) { return *this; }
        ~nontrivial_type() {};
    };

    template<std::size_t N>
    struct ct_string_wrap
    {
        consteval ct_string_wrap(const char (&str)[N]) {
            for (std::size_t idx = 0; idx < N; ++idx)
                strptr[idx] = str[idx];
        }

        char strptr[N];
    };

    template <ct_string_wrap id>
    struct counted_ctr_dtr {
        counted_ctr_dtr() { default_constructed++; }
        counted_ctr_dtr(const counted_ctr_dtr&) { copy_constructed++; }
        counted_ctr_dtr(counted_ctr_dtr&&) { move_constructed++; }
        ~counted_ctr_dtr() { destroyed++; }

        counted_ctr_dtr& operator=(const counted_ctr_dtr& c) = default;
        counted_ctr_dtr& operator=(counted_ctr_dtr&& c) = default;

        inline static std::size_t default_constructed;
        inline static std::size_t copy_constructed;
        inline static std::size_t move_constructed;
        inline static std::size_t destroyed;
    };
}

#endif
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