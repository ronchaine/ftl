#ifndef FTL_UTILITY_HPP
#define FTL_UTILITY_HPP

#include <type_traits>

namespace ftl
{
    struct no_init_tag_t {};
    [[maybe_unused]] constexpr static no_init_tag_t do_not_init;

    template <typename... T> [[maybe_unused]]
    constexpr static bool always_false = false;

    template <typename T, typename First, typename... Dims>
    constexpr static bool each_convertible_to() noexcept
    {
        if constexpr (not std::is_convertible_v<First, T>)
            return false;
        if constexpr (sizeof...(Dims))
            return each_convertible_to<T, Dims...>();

        return true;
    }
};

namespace ftl::detail
{
    // These are just so that the template specialisations for common
    // template parameters are more readable
    [[maybe_unused]] constexpr static bool REFERENCE = true;
    [[maybe_unused]] constexpr static bool NOT_REFERENCE = false;

    [[maybe_unused]] constexpr static bool TRIVIALLY_DESTRUCTIBLE = true;
    [[maybe_unused]] constexpr static bool NOT_TRIVIALLY_DESTRUCTIBLE = false;

}

#define FTL_MOVE(...) \
    static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)

#define FTL_FORWARD(...) \
    static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

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
