#ifndef FTL_ARRAY_HPP
#define FTL_ARRAY_HPP

#include <cstdint>
#include <type_traits>

#include "utility.hpp"
#include "hash.hpp"

namespace ftl
{
    template <typename T, std::size_t... Dimensions>
    class array
    {
        template <std::size_t Depth, bool IsConst>
        struct array_access_proxy;

        public:
            using value_type        = T;
            using reference         = T&;
            using const_reference   = const T&;
            using size_type         = std::size_t;
            using difference_type   = std::ptrdiff_t;
            using pointer           = T*;
            using const_pointer     = const T*;
            using iterator          = T*;
            using const_iterator    = const T*;

            constexpr static std::size_t dimension = sizeof...(Dimensions);

            constexpr array() noexcept = default;

            template <typename... Values>
            explicit constexpr array(Values&&... v) noexcept : data_array{ FTL_FORWARD(v)... } {}

            // element access
            [[nodiscard]] constexpr auto operator[](size_type index) noexcept requires (dimension > 1) {
                return array_access_proxy<1, false>(*this, index);
            }

            [[nodiscard]] constexpr const auto operator[](size_type index) const noexcept requires (dimension > 1) {
                return array_access_proxy<1, true>(*this, index);
            }

            [[nodiscard]] constexpr reference operator[](size_type index) noexcept requires (sizeof...(Dimensions) == 1) {
                return data_array[index];
            }

            [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept requires (sizeof...(Dimensions) == 1) {
                return data_array[index];
            }

            template <typename... Dims>
            [[nodiscard]] constexpr reference at(Dims... d) noexcept requires (sizeof...(Dims) == sizeof...(Dimensions))
            {
                static_assert(each_convertible_to<size_type, Dims...>());
                return data_array[calc_array_index(d...)];
            }

            template <typename... Dims>
            [[nodiscard]] constexpr const_reference at(Dims... d) const noexcept requires (sizeof...(Dims) == sizeof...(Dimensions))
            {
                static_assert(each_convertible_to<size_type, Dims...>());
                return data_array[calc_array_index(d...)];
            }

            [[nodiscard]] constexpr reference front() noexcept                { return data_array[0]; }
            [[nodiscard]] constexpr const_reference front() const noexcept    { return data_array[0]; }
            [[nodiscard]] constexpr reference back() noexcept                 { return data_array[size() - 1]; }
            [[nodiscard]] constexpr const_reference back() const noexcept     { return data_array[size() - 1]; }
            [[nodiscard]] constexpr pointer data() noexcept                   { return data_array; }
            [[nodiscard]] constexpr const_pointer data() const noexcept       { return data_array; }

            // iterators
            [[nodiscard]] constexpr iterator begin() noexcept { return iterator(data_array); }
            [[nodiscard]] constexpr iterator end() noexcept { return iterator(data_array + size()); };
            [[nodiscard]] constexpr const_iterator begin() const noexcept { return iterator(data_array); }
            [[nodiscard]] constexpr const_iterator end() const noexcept { return iterator(data_array + size()); };

            // capacity
            [[nodiscard]] constexpr static bool empty() noexcept { return size() == 0; }
            [[nodiscard]] constexpr static size_type size() noexcept { return (Dimensions * ...); }
            [[nodiscard]] constexpr static size_type max_size() noexcept { return (Dimensions * ...); }

            [[nodiscard]] constexpr static size_type byte_size() noexcept { return size() * sizeof(T); }

            // operations
            constexpr void fill(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>);
            constexpr void swap(array& other) noexcept(std::is_nothrow_swappable_v<T>);

            [[nodiscard]] constexpr uint64_t hash() const noexcept { return ::ftl::hash(*this); }

        private:
            template <typename... Dims>
            constexpr static size_type calc_array_index(size_type idx, Dims... d) noexcept
            {
                if constexpr(sizeof...(Dims))
                    return idx * dim_size[sizeof...(d)] + calc_array_index(d...);
                return idx;
            }

            constexpr static size_type dim_size[] = { Dimensions... };
            value_type data_array[size()];
    };

    template <typename T, std::size_t... Dimensions>
    void swap(array<T, Dimensions...>& a, array<T, Dimensions...>& b) noexcept(std::is_nothrow_swappable_v<T>) {
        auto temp = FTL_MOVE(a);
        a = FTL_MOVE(b);
        b = FTL_MOVE(temp);
    }

    template <typename T, std::size_t... Dimensions>
    constexpr bool operator==(const array<T, Dimensions...>& lhs, const array<T, Dimensions...>& rhs) noexcept
    {
        auto lhs_it = lhs.begin();
        auto rhs_it = rhs.begin();

        while(lhs_it != lhs.end())
            if (*lhs_it++ != *rhs_it++)
                return false;

        return true;
    }

    template <typename T, std::size_t... Dimensions>
    template <std::size_t Depth, bool IsConst>
    struct array<T, Dimensions...>::array_access_proxy
    {
        array&          ref;
        size_type       cindex;

        constexpr explicit array_access_proxy(array& arr, std::size_t cindex) noexcept requires (!IsConst) : ref(arr), cindex(cindex) {}
        constexpr explicit array_access_proxy(const array& arr, std::size_t cindex) noexcept requires (IsConst): ref(arr), cindex(cindex) {}

        constexpr reference operator[](std::size_t index) && noexcept 
            requires (!IsConst) && (Depth + 1 == array::dimension)
        {
            return ref.data_array[cindex * ref.dim_size[Depth - 1] + index];
        }

        constexpr const_reference operator[](std::size_t index) const && noexcept
            requires (Depth + 1 == array::dimension) && IsConst
        {
            return ref.data_array[cindex * ref.dim_size[Depth - 1] + index];
        }

        constexpr auto&& operator[](std::size_t index) const && noexcept requires (Depth + 1 < sizeof...(Dimensions)) {
            return array_access_proxy<Depth+1, IsConst>(ref, index + (cindex * ref.dim_size[Depth-1]));
        }
    };

    template <typename T, std::size_t... Dimensions>
    constexpr void array<T, Dimensions...>::fill(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        // TODO: make this sane, this is probably slow af unless the compiler
        //       figures out some optimisation here
        for (T& e : data_array)
            e = value;
    }

    template <typename T, std::size_t... Dimensions>
    constexpr void array<T, Dimensions...>::swap(array& other) noexcept(std::is_nothrow_swappable_v<T>)
    {
        ::ftl::swap(*this, other);
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
