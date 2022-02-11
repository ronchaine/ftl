#ifndef FTL_RINGBUFFER_HPP
#define FTL_RINGBUFFER_HPP

#include <type_traits>
#include <new>

#include "memory.hpp"
#include "utility.hpp"

#if __STDC_HOSTED__ == 1
# include <stdexcept>
# include <memory>
# include <cassert>
#else
// TODO:  we probably want our own assert since this is not in
// standard either, both gcc and clang provide it in freestanding
// though
# include <assert.h>
#endif

namespace ftl
{
    namespace detail {
        // These are just so that the following specialisations are more readable
        constexpr static bool REFERENCE = true;
        constexpr static bool NOT_REFERENCE = false;

        constexpr static bool TRIVIALLY_DESTRUCTIBLE = true;
        constexpr static bool NOT_TRIVIALLY_DESTRUCTIBLE = false;

        template <typename T,
                  typename Unknown,
                  bool IsReference = std::is_reference<T>::value,
                  bool IsTriviallyDestructible = std::is_trivially_destructible<T>::value>
        struct ring_buffer_storage
        {
            static_assert(always_false<T>, "Invalid argument for ring buffer's storage");
        };

        template <typename T, Any_good_enough_allocator Allocator, bool Ref, bool CallDestructor>
        struct ring_buffer_storage<T, Allocator, Ref, CallDestructor>
        {
            constexpr static bool   is_dynamic = true;

            using value_type        = T;
            using allocator_type    = Allocator;
            using pointer           = typename Allocator::pointer;

            [[nodiscard]] constexpr allocator_type get_allocator() noexcept { return allocator; }

            allocator_type allocator;
        };

        template <typename T, size_t StaticSize, bool Dummy>
        struct ring_buffer_storage<T, ftl::static_storage<StaticSize>, REFERENCE, Dummy>
        {
            constexpr static bool       is_dynamic = false;

            constexpr static size_t     data_size = StaticSize;
        };

        template <typename T, size_t StaticSize>
        struct ring_buffer_storage<T, ftl::static_storage<StaticSize>, NOT_REFERENCE, TRIVIALLY_DESTRUCTIBLE>
        {
            public:
                using value_type                        = T;
                using allocator_type                    = void;
                using pointer                           = T*;
                using const_pointer                     = const T* const;
                using size_type                         = std::size_t;

                constexpr static bool is_dynamic        = false;
                constexpr static size_t data_size       = StaticSize;

                constexpr ring_buffer_storage() noexcept = default;
                constexpr ~ring_buffer_storage() noexcept = default;

                constexpr inline bool is_empty() const noexcept { return (write_head == read_head) || read_head == nullptr; }
                constexpr inline bool is_full() const noexcept { return write_head == nullptr; }

                constexpr inline void advance_write_head() noexcept {
                    write_head = write_head == data() + data_size - 1 ? data() : write_head + 1;
                    if (write_head == read_head)
                        write_head = nullptr;
                }

                constexpr inline void advance_read_head() noexcept {
                    read_head = read_head == data() + data_size - 1 ? data() : read_head + 1;
                }

                constexpr inline void release() const noexcept { return; }

                constexpr pointer data() noexcept { return std::launder(reinterpret_cast<pointer>(&store)); }
                constexpr size_type get_capacity() const noexcept { return StaticSize; }
                constexpr size_type get_size() const noexcept { return StaticSize; }

            protected:
                constexpr inline pointer& get_write_head() noexcept { return write_head; }
                constexpr inline pointer& get_read_head() noexcept { return read_head; }

                constexpr inline const_pointer& get_write_head() const noexcept { return write_head; }
                constexpr inline const_pointer& get_read_head() const noexcept { return read_head; }

            private:
                std::aligned_storage_t<sizeof(T), alignof(T)> store[StaticSize];

                pointer write_head = data();
                pointer read_head = nullptr;
        };

        template <typename T, size_t StaticSize>
        struct ring_buffer_storage<T, ftl::static_storage<StaticSize>, NOT_REFERENCE, NOT_TRIVIALLY_DESTRUCTIBLE>
        {
            public:
                using value_type                        = T;
                using allocator_type                    = void;
                using pointer                           = T*;
                using const_pointer                     = const T* const;
                using size_type                         = std::size_t;

                constexpr static bool is_dynamic        = false;
                constexpr static size_t data_size       = StaticSize;

                constexpr ring_buffer_storage() noexcept = default;

                constexpr inline void advance_write_head() noexcept {
                    write_head = write_head == data() + data_size - 1 ? data() : write_head + 1;
                    if (write_head == read_head)
                        write_head = nullptr;
                }

                constexpr inline void advance_read_head() noexcept {
                    read_head = read_head == data() + data_size - 1 ? data() : read_head + 1;
                }

                ~ring_buffer_storage() {
                    if (read_head == nullptr) {
                        return;
                    }

                    if (write_head == nullptr) {
                        // TODO: handle read_head == nullptr
                        write_head = read_head == data() + data_size - 1 ? data() : read_head + 1;
                    }

                    while(write_head != read_head) {
                        get_read_head()->~T();
                        advance_read_head();
                    }
                }

                constexpr void release() noexcept {
                    get_read_head()->~T();
                }

                constexpr pointer data() noexcept { return std::launder(reinterpret_cast<pointer>(&store)); }
                constexpr size_type get_capacity() const noexcept { return StaticSize; }

            protected:
                constexpr inline pointer& get_write_head() noexcept { return write_head; }
                constexpr inline pointer& get_read_head() noexcept { return read_head; }

                constexpr inline const_pointer& get_write_head() const noexcept { return write_head; }
                constexpr inline const_pointer& get_read_head() const noexcept { return read_head; }

            private:
                std::aligned_storage_t<sizeof(T), alignof(T)> store[StaticSize];

                pointer write_head = data();
                pointer read_head = nullptr;
        };

        template <typename T, typename Storage>
        struct ring_buffer_details : ring_buffer_storage<T, Storage>
        {
            using value_type = typename ring_buffer_storage<T, Storage>::value_type;

            using ring_buffer_storage<T, Storage>::get_write_head;
            using ring_buffer_storage<T, Storage>::get_read_head;
            using ring_buffer_storage<T, Storage>::advance_write_head;
            using ring_buffer_storage<T, Storage>::advance_read_head;
            using ring_buffer_storage<T, Storage>::release;
            using ring_buffer_storage<T, Storage>::data;

            constexpr bool is_empty() const noexcept { return (get_write_head() == get_read_head()) || get_read_head() == nullptr; }
            constexpr bool is_full() const noexcept { return get_write_head() == nullptr; }

            template <typename U, bool allow_overwrite = false> requires std::is_convertible_v<U, T>
            constexpr void construct(U&& elem) {
                if (get_read_head() == nullptr) [[unlikely]]
                    get_read_head() = data();

                if (is_full()) {
                    if constexpr(not allow_overwrite) {
                        #ifdef __cpp_exceptions
                            throw std::out_of_range("ring buffer full");
                        #endif
                        assert(not is_full());
                    }

                    // just overwrite if NDEBUG and no exceptions
                    release();
                    get_write_head() = get_read_head();
                    advance_read_head();
                }
                // TODO: figure out if this move could be elided
                ::new (std::remove_reference_t<T*>(get_write_head())) value_type { FTL_FORWARD(elem) };
                advance_write_head();
            }

            constexpr T&& read_delete() {
                #ifdef __cpp_exceptions
                    if (is_empty()) throw std::out_of_range("read from empty ring buffer");
                #endif
                assert(not is_empty());

                // Does this need launder?
                T&& val = FTL_MOVE(*(get_read_head()));
                if (get_write_head() == nullptr)
                    get_write_head() = get_read_head();

                release();

                advance_read_head();
                return FTL_MOVE(val);
            }
        };
    }

    template <typename T, typename Storage = FTL_DEFAULT_ALLOCATOR>
    class ring_buffer : detail::ring_buffer_details<T, Storage>
    {
        public:
            using value_type        = T;
            using size_type         = std::size_t;

            template <bool is_const>
            class rb_iterator;
            using iterator = rb_iterator<false>;
            using const_iterator = rb_iterator<true>;

            constexpr ring_buffer() = default;

            // iterators
            constexpr iterator begin();
            constexpr const_iterator begin() const;

            constexpr iterator end();
            constexpr const_iterator end() const;

            // modifiers
            template <typename U> requires std::is_convertible_v<U, T>
            constexpr void push(U&& elem) noexcept(std::is_nothrow_copy_constructible<T>::value) { this->construct(FTL_FORWARD(elem)); }

            template <typename U> requires std::is_convertible_v<U, T>
            constexpr void push_overwrite(const T& elem) noexcept(std::is_nothrow_copy_constructible<T>::value) { this->template construct<U, true>(FTL_FORWARD(elem)); }

            constexpr void swap(ring_buffer& rhs) { swap(*this, rhs); }

            [[nodiscard]] constexpr T&& pop() { return this->read_delete(); }

            // queries
            [[nodiscard]] constexpr size_type size() const noexcept { return detail::ring_buffer_storage<T, Storage>::get_size(); }
            [[nodiscard]] constexpr size_type capacity() const noexcept { return detail::ring_buffer_storage<T, Storage>::get_capacity(); }
            [[nodiscard]] constexpr bool is_empty() const noexcept { return detail::ring_buffer_storage<T, Storage>::is_empty(); }
            [[nodiscard]] constexpr bool is_full() const noexcept { return detail::ring_buffer_storage<T, Storage>::is_full(); }

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
