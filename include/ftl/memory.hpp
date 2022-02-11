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

namespace ftl
{
    template <typename T>
    concept Any_good_enough_allocator = requires(T t) {
        { T::pointer };
        { T::allocate() } -> std::same_as<typename T::pointer>;
        { T::deallocate() };
    };

    template <std::size_t StorageBytes>
    struct static_storage
    {
        static_assert(StorageBytes != 0);
        constexpr static std::size_t size = StorageBytes;
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
