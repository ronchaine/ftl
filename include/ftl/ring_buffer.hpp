#ifndef FTL_RINGBUFFER_HPP
#define FTL_RINGBUFFER_HPP

#include <type_traits>

#if __STDC_HOSTED__ == 1
# include <stdexcept>
# include <memory>
# include <cassert>
# define FTL_DEFAULT_ALLOCATOR std::allocator<T>
#else
// TODO:  we probably want our own assert since this is not in
// standard either, both gcc and clang provide it in freestanding
// though
# include <assert.h>
# define FTL_DEFAULT_ALLOCATOR ftl::static_storage<128>
#endif

namespace ftl
{
    template <size_t StorageBytes>
    struct static_storage
    {
        static_assert(StorageBytes != 0);
        constexpr static size_t size = StorageBytes;
    };

    template <typename T, typename Allocator>
    class ring_buffer_details
    {
        public:
            constexpr static bool   is_dynamic = true;

            using size_type         = std::size_t;
            using allocator_type    = Allocator;
            using pointer           = typename std::allocator_traits<Allocator>::pointer;
            using const_pointer     = typename std::allocator_traits<Allocator>::const_pointer;

            [[nodiscard]] constexpr allocator_type get_allocator() noexcept { return allocator; }

            ~ring_buffer_details() {
                if constexpr(is_dynamic)
                    allocator.deallocate(data_ptr, data_size);
            }

        protected:
            constexpr static size_t initial_power   = 2;
            constexpr static size_t initial_size    = 2 << initial_power;
            constexpr static size_t grow_factor     = 2;

            inline void grow(T*& read_head, T*& write_head) {
                T* old_data_ptr = data_ptr;
                const size_t new_size = data_size == 0 ? initial_size : data_size * grow_factor;

                T* new_data_ptr = this->allocator.allocate(new_size);

                size_t it = 0;
                while (read_head != write_head) {
                    new_data_ptr[it++] = static_cast<T&&>(*read_head);
                    read_head = read_head == data_size - 1 ? 0 : read_head + 1;
                }

                read_head = new_data_ptr;
                write_head = new_data_ptr + it;

                allocator.deallocate(old_data_ptr, data_size);
                data_ptr = new_data_ptr;
                data_size = new_size;
            }

            allocator_type          allocator;

            T*                      data_ptr        = nullptr;
            size_type               data_size       = 0;
    };

    template <typename T, size_t StaticSize>
    class ring_buffer_details<T, static_storage<StaticSize>>
    {
        public:
            constexpr static bool   is_dynamic = false;
            constexpr static size_t data_size = StaticSize;

            constexpr ring_buffer_details() noexcept = default;

        protected:
            T                       data_ptr[StaticSize] {};
    };

    template <typename T, typename Allocator = FTL_DEFAULT_ALLOCATOR>
    class ring_buffer : public ring_buffer_details<T, Allocator>
    {
        public:
            using value_type        = T;
            using size_type         = std::size_t;
            using difference_type   = std::ptrdiff_t;
            using reference         = T&;
            using const_reference   = const T&;

            using ring_buffer_details<T, Allocator>::is_dynamic;

            template <bool is_const>
            class rb_iterator;

            using iterator          = rb_iterator<false>;
            using const_iterator    = rb_iterator<true>;

            constexpr ring_buffer() = default;

            // modifiers
            constexpr void push(const T& elem);
            constexpr void push(T&& elem);

            constexpr void push_overwrite(const T& elem);
            constexpr void push_overwrite(T&& elem);

            constexpr void swap(ring_buffer& rhs) noexcept { swap(*this, rhs); }

            [[nodiscard]] constexpr T&& pop();

            // queries
            [[nodiscard]] constexpr size_type capacity() const noexcept { return data_size; }
            [[nodiscard]] constexpr bool is_empty() const noexcept { return write_head == read_head; }
            [[nodiscard]] constexpr bool is_full() const noexcept { return buffer_out_of_space(); }

        private:
            using ring_buffer_details<T, Allocator>::data_size;
            using ring_buffer_details<T, Allocator>::data_ptr;

            inline bool buffer_out_of_space() const noexcept;

            T* read_head    = data_ptr;
            T* write_head   = data_ptr;
    };

    template <typename T, typename A>
    bool ring_buffer<T,A>::buffer_out_of_space() const noexcept {
        if (data_size == 0)
            return true;

        if (write_head == nullptr)
            return true;

        return false;
    }

    template <typename T, typename A>
    constexpr void ring_buffer<T,A>::push(const T& elem) {
        T movable = elem;
        push(static_cast<T&&>(movable));
    }

    template <typename T, typename A>
    constexpr void ring_buffer<T,A>::push(T&& elem) {
        if (buffer_out_of_space()) {
            if constexpr(is_dynamic) {
                this->grow(read_head, write_head);
            } else {
                // We just overwrite on overflow
            }
        }

        *write_head = elem;
        write_head = write_head == data_ptr + data_size - 1 ? data_ptr : write_head + 1;
        if (write_head == read_head)
            write_head = nullptr;
    }

    template <typename T, typename A>
    [[nodiscard]] constexpr T&& ring_buffer<T,A>::pop() {
        #ifdef __cpp_exceptions
            if (is_empty()) throw std::out_of_range{"read from empty ring buffer"};
        #endif
        assert(not is_empty());
        T&& val = static_cast<T&&>(*read_head);

        if (write_head == nullptr)
            write_head = read_head;

        read_head = read_head == data_ptr + data_size - 1 ? data_ptr : read_head + 1;
        return static_cast<T&&>(val);
    }

    template <typename T, typename A>
    constexpr void swap(ring_buffer<T,A>& a, ring_buffer<T,A>& b) noexcept {
        auto tmp(a);
        a = b;
        b = tmp;
    }

    template <typename T, typename A> template <bool is_const>
    class ring_buffer<T,A>::rb_iterator
    {
        public:
            // TODO: Make sure we have something for std::forward_iterator_tag if
            //       on freestanding (works with gcc and clang currently)

            // The iterator is LegacyBidirectionalIterator, but not bidirectional_iterator
            // for the C++ standard
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            // LegacyIterator requirements
            constexpr reference operator*() { return *ptr; };

        private:
            T* ptr;
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
