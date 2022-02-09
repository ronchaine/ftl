#ifndef FTL_HASH_HPP
#define FTL_HASH_HPP

#include <type_traits>

namespace ftl
{
    struct hash_fnv1a
    {
        constexpr static uint64_t fnv64_prime = 0x100000001b3ULL;
        constexpr static uint64_t fnv64_init = 0xcbf29ce484222325ULL;

        static uint64_t hash64(const uint8_t* ptr, std::size_t byte_count) noexcept {
            uint64_t hval = fnv64_init;
            const uint8_t* start = ptr;

            while (ptr != start + byte_count)
            {
                hval ^= *ptr++;
                hval *= fnv64_prime;
            }

            return hval;
        }
    };
}

namespace ftl
{
    template<typename T, typename U>
    concept is_same_as = std::is_same_v<std::decay_t<T>, std::decay_t<U>>;

    template <typename HashType = hash_fnv1a, typename T> requires requires(T t) {
        typename T::value_type;
        { t.size() } -> is_same_as<typename T::size_type>;
        { t.begin() } -> is_same_as<typename T::iterator>;
        { t.end() } -> is_same_as<typename T::iterator>;
    }
    uint64_t hash(const T& buffer) {
        // FIXME: statically check if the buffer is contiguous
        return HashType::hash64(
                reinterpret_cast<const uint8_t*>(buffer.data()),
                buffer.size() * sizeof(typename T::value_type));
    }

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
