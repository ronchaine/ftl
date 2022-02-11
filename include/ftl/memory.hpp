#ifndef FTL_MEMORY_HPP
#define FTL_MEMORY_HPP

#include <cstdint>
#include <type_traits>
#include <concepts>

#if __STDC_HOSTED__ == 1
# define FTL_DEFAULT_ALLOCATOR std::allocator<T>
#else
# define FTL_DEFAULT_ALLOCATOR ftl::static_storage<64>
#endif

// I don't know how standard this is, but it allows to check
// if allocator_traits is defined by checking its completeness
namespace std {
    template< class Alloc > struct allocator_traits;
}

namespace ftl
{
    template <std::size_t StorageBytes>
    struct static_storage
    {
        static_assert(StorageBytes != 0);
        constexpr static std::size_t size = StorageBytes;
    };

    template<typename, typename = void> [[maybe_unused]]
    constexpr static bool is_type_complete_v = false;

    template<typename T> [[maybe_unused]]
    constexpr static bool is_type_complete_v<T, std::void_t<decltype(sizeof(T))>> = true;

    template <typename T>
    consteval static bool is_type_complete() noexcept { return is_type_complete_v<T>; }

    template <typename T>
    concept any_with_required_allocator_traits = is_type_complete_v<T> && std::is_compound_v<T> && requires(T t) {
        typename T::value_type;

        // TODO: check doing this with declval or something, I tried it
        //        but I was tired and might've messed up the syntax...
        { t.allocate(0) } -> std::same_as<typename std::allocator_traits<T>::pointer>;
        { t.deallocate(nullptr, 0) };

        typename std::allocator_traits<T>::allocator_type;
        typename std::allocator_traits<T>::value_type;
        typename std::allocator_traits<T>::pointer;
        typename std::allocator_traits<T>::const_pointer;
        typename std::allocator_traits<T>::size_type;
    };

    template <typename T>
    consteval static bool has_allocator_traits() noexcept { return false; }

    template <any_with_required_allocator_traits T>
    consteval static bool has_allocator_traits() noexcept { return true; }

    template <typename T>
    concept suitable_raw_allocator = std::is_compound_v<T> && requires(T t) {
        typename T::allocator_type;
        typename T::value_type;
        typename T::pointer;
        typename T::const_pointer;
        typename T::size_type;

        { t.allocate() } -> std::same_as<typename T::pointer>;
        { t.deallocate(nullptr, 0) };
    };

    template <typename T>
    concept any_good_enough_allocator = any_with_required_allocator_traits<T> || suitable_raw_allocator<T>;
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
