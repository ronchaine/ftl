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
