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
        // for providing decent-ish error message if allocator wasn't good enough
        template <ftl::any_good_enough_allocator T>
        constexpr void test_allocator_suitability() {}

        template <typename T,
                  typename U,
                  bool IsReference = std::is_reference<T>::value,
                  bool IsTriviallyDestructible = std::is_trivially_destructible<T>::value>
        struct ring_buffer_storage
        {
            static_assert(test_allocator_suitability<U>(), "Could not use provided storage type as allocator or static storage");
            static_assert(not IsReference, "Reference storage not implemented");
        };

        template <typename T, any_good_enough_allocator Allocator, bool CallDestructor>
        struct ring_buffer_storage<T, Allocator, NOT_REFERENCE, CallDestructor>
        {
            public:
                using allocator_traits  = typename std::conditional<has_allocator_traits<Allocator>(),
                                          std::allocator_traits<Allocator>,
                                          Allocator>::type;

                static_assert(std::is_same_v<T, typename allocator_traits::value_type>);

                using value_type        = T;
                using allocator_type    = typename allocator_traits::allocator_type;
                using pointer           = typename allocator_traits::pointer;
                using const_pointer     = const T* const; // FIXME: typename allocator_traits::const_pointer;
                using size_type         = typename allocator_traits::size_type;

                constexpr static bool   is_dynamic = true;

                constexpr ring_buffer_storage() noexcept = default;
                constexpr ~ring_buffer_storage() {
                    if (data_begin == nullptr)
                        return;

                    if constexpr(not std::is_trivially_destructible_v<value_type>) {
                        if (read_head == nullptr)
                            return;

                        if (write_head == nullptr)
                            write_head = read_head == data() + get_capacity() - 1 ? data() : read_head + 1;

                        while(write_head != read_head) {
                            get_read_head()->~T();
                            advance_read_head();
                        }
                    }

                    allocator.deallocate(data_begin, get_size());
                };

                constexpr inline bool is_empty() const noexcept { return (write_head == read_head) || read_head == nullptr; }
                constexpr inline bool is_full() const noexcept { return write_head == nullptr; }

                constexpr inline void advance_write_head() noexcept {
                    write_head = write_head == data() + get_capacity() - 1 ? data() : write_head + 1;
                    if (write_head == read_head)
                        write_head = nullptr;
                }

                constexpr inline void advance_read_head() noexcept {
                    read_head = read_head == data() + get_capacity() - 1 ? data() : read_head + 1;
                }

                constexpr inline void release() const noexcept requires std::is_trivially_destructible_v<T> {}

                constexpr inline void release() noexcept requires (!std::is_trivially_destructible_v<T>) {
                    if constexpr(not std::is_trivially_destructible_v<T>)
                        get_read_head()->~T();
                }

                constexpr void resize(size_type new_size) {
                    if (new_size <= get_capacity())
                        return;

                    pointer old_data_begin = data_begin;
                    pointer new_data_ptr = allocator.allocate(new_size);

                    size_t it = 0;
                    if (old_data_begin != nullptr) {
                        while (read_head != write_head) {
                            if constexpr(std::is_move_assignable_v<T>)
                                new_data_ptr[it++] = static_cast<T&&>(*read_head);
                            else 
                                new_data_ptr[it++] = *read_head;
                            advance_read_head();
                        }
                    }
                    data_begin = new_data_ptr;
                    data_end = new_data_ptr + new_size;

                    read_head = data_begin;
                    write_head = data_begin + it;
                }

                constexpr pointer data() noexcept { return data_begin; }
                constexpr size_type get_capacity() const noexcept { return (data_end - data_begin); }
                constexpr size_type get_size() const noexcept {
                    if (write_head == nullptr)
                        return get_capacity();

                    if (write_head == read_head || read_head == nullptr)
                        return 0;
                    return write_head > read_head
                        ? static_cast<size_type>(write_head - read_head)
                        : static_cast<size_type>(get_capacity() - (read_head - write_head));
                }

                [[nodiscard]] constexpr allocator_type get_allocator() noexcept { return allocator; }

            protected:
                constexpr static size_t initial_power   = 3;
                constexpr static size_t initial_size    = 2 << (initial_power - 1);
                constexpr static size_t grow_factor     = 2;

                inline void grow() {
                    const size_t new_size = get_capacity() == 0 ? initial_size : get_capacity() * grow_factor;
                    resize(new_size);
                }

                constexpr inline pointer& get_write_head() noexcept { return write_head; }
                constexpr inline pointer& get_read_head() noexcept { return read_head; }

                constexpr inline const_pointer& get_write_head() const noexcept { return write_head; }
                constexpr inline const_pointer& get_read_head() const noexcept { return read_head; }

            private:
                pointer read_head = nullptr;
                pointer write_head = nullptr;

                pointer data_begin = nullptr;
                pointer data_end = nullptr;

                allocator_type allocator;
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
                constexpr size_type get_size() const noexcept {
                    if (write_head == nullptr)
                        return StaticSize;

                    if (is_empty())
                        return 0;

                    return write_head > read_head
                        ? static_cast<size_type>(write_head - read_head)
                        : static_cast<size_type>(StaticSize - (read_head - write_head));
                }

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

                    if (write_head == nullptr)
                        write_head = read_head == data() + data_size - 1 ? data() : read_head + 1;

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
                constexpr size_type get_size() const noexcept {
                    if (write_head == nullptr)
                        return StaticSize;
                    if (write_head == read_head || read_head == nullptr)
                        return 0;
                    return write_head > read_head
                        ? static_cast<size_type>(write_head - read_head)
                        : static_cast<size_type>(StaticSize - (read_head - write_head));
                }

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
            using size_type = typename ring_buffer_storage<T, Storage>::size_type;

            using ring_buffer_storage<T, Storage>::is_dynamic;
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

                if constexpr(is_dynamic) {
                    if (is_full())
                        ring_buffer_storage<T, Storage>::grow();
                } else {
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
                }
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

            constexpr T read_copy_delete() {
                #ifdef __cpp_exceptions
                    if (is_empty()) throw std::out_of_range("read from empty ring buffer");
                #endif
                assert(not is_empty());

                T val = *get_read_head();
                if (get_write_head() == nullptr)
                    get_write_head() = get_read_head();

                release();

                advance_read_head();
                return T{val};
            }

            constexpr void clear() noexcept {
                if constexpr(std::is_trivially_destructible_v<T>) {
                    while(get_read_head() != get_write_head()) {
                        release();
                        advance_read_head();
                    }
                }
                get_read_head() = nullptr;
                get_write_head() = data();
            }

            constexpr bool is_contiguous() const noexcept {
                if (get_read_head() == nullptr)
                    return true;
                else if (get_write_head() == nullptr)
                    return get_read_head() == data();
                else
                    return get_read_head() < get_write_head();
            };
        };
    }

    template <typename T, typename Storage = FTL_DEFAULT_ALLOCATOR>
    class ring_buffer : detail::ring_buffer_details<T, Storage>
    {
        public:
            using value_type        = T;
            using size_type         = std::size_t;
            using pointer           = typename detail::ring_buffer_storage<T, Storage>::pointer;

            using allocator_type    = typename detail::ring_buffer_storage<T, Storage>::allocator_type;

            template <bool is_const>
            class rb_iterator;
            using iterator = rb_iterator<false>;
            using const_iterator = rb_iterator<true>;

            constexpr ring_buffer() = default;

            // iterators
            constexpr iterator begin() noexcept { return iterator{*this}; }
            constexpr const_iterator begin() const noexcept { return const_iterator{*this}; }

            constexpr iterator end() noexcept { return iterator{*this, nullptr}; }
            constexpr const_iterator end() const noexcept { return const_iterator{*this, nullptr}; }

            // modifiers
            template <typename U> requires std::is_convertible_v<U, T>
            constexpr void push(U&& elem) noexcept(std::is_nothrow_move_constructible<T>::value) { this->construct(FTL_FORWARD(elem)); }

            template <typename U>
            constexpr void push(const U& elem) noexcept(std::is_nothrow_copy_constructible<T>::value) { this->construct(elem); }

            template <typename U> requires std::is_convertible_v<U, T>
            constexpr void push_overwrite(const T& elem) noexcept(std::is_nothrow_copy_constructible<T>::value) { this->template construct<U, true>(FTL_FORWARD(elem)); }

            [[nodiscard]] constexpr T&& pop() requires std::is_move_assignable_v<T> { return this->read_delete(); }

            constexpr void resize(size_type count) requires detail::ring_buffer_storage<T, Storage>::is_dynamic { detail::ring_buffer_storage<T, Storage>::resize(count); }
            constexpr void resize(size_type count) const noexcept requires (!detail::ring_buffer_storage<T, Storage>::is_dynamic) {}
            constexpr void clear() noexcept { detail::ring_buffer_details<T, Storage>::clear(); }
            constexpr void swap(ring_buffer& rhs) { swap(*this, rhs); }

            // FIXME: This causes an extra copy.
            [[nodiscard]] constexpr T pop() requires (!std::is_move_assignable_v<T>) { return this->read_copy_delete(); }

            // queries
            [[nodiscard]] constexpr size_type size() const noexcept { return detail::ring_buffer_storage<T, Storage>::get_size(); }
            [[nodiscard]] constexpr size_type capacity() const noexcept { return detail::ring_buffer_storage<T, Storage>::get_capacity(); }
            [[nodiscard]] constexpr bool is_empty() const noexcept { return detail::ring_buffer_storage<T, Storage>::is_empty(); }
            [[nodiscard]] constexpr bool is_full() const noexcept { return detail::ring_buffer_storage<T, Storage>::is_full(); }

            [[nodiscard]] constexpr bool is_contiguous() const noexcept { return detail::ring_buffer_storage<T, Storage>::is_contiguous(); }

    };

    // Cannot use iterator concepts before Defect report P2325R3 is fixed in compilers,
    // we are not default-constructible
    template <typename T, typename Storage> template <bool Is_Const>
    class ring_buffer<T, Storage>::rb_iterator
    {
        using target_reference = typename std::conditional<Is_Const, const ring_buffer<T, Storage>&, ring_buffer<T, Storage>&>::type;
        using target_pointer = typename std::conditional<Is_Const, const ring_buffer<T, Storage>*, ring_buffer<T, Storage>*>::type;

        public:
            using value_type        = ring_buffer<T, Storage>::value_type;
            using pointer           = typename std::conditional<Is_Const, const T*, T*>::type;
            using difference_type   = std::ptrdiff_t;

            using reference         = typename std::conditional<Is_Const, const value_type&, value_type&>::type;

            constexpr rb_iterator(target_reference ref) : ref{&ref} {}
            constexpr rb_iterator(target_reference ref, pointer ptr) : ref{&ref}, ptr{ptr} {}

            constexpr rb_iterator(const rb_iterator&) noexcept = default;

            constexpr rb_iterator&  operator=(const rb_iterator&) noexcept = default;

            constexpr reference     operator++() noexcept { advance_ptr(); return **this; }
            constexpr reference     operator--() noexcept { backtrack_ptr(); return **this; }

            constexpr reference     operator*() noexcept { return *ptr; }
            constexpr pointer       operator->() noexcept { return ptr; }

            constexpr bool          operator==(const rb_iterator& rhs) const noexcept = default;

            constexpr value_type    operator++(int) { value_type tmp{*this}; advance_ptr(); return tmp; }
            constexpr value_type    operator--(int) { value_type tmp{*this}; backtrack_ptr(); return tmp; }


        private:
            constexpr void advance_ptr() noexcept {
                if (ptr == nullptr)
                    ptr = ref->get_read_head();
                else {
                    ptr = ptr == ref->data() + ref->capacity() - 1 ? ref->data() : ptr + 1;
                    if (ptr == ref->get_write_head() || ptr == ref->get_read_head()) {
                        ptr = nullptr;
                    }
                }
            }

            constexpr void backtrack_ptr() noexcept {
                if (ptr == nullptr)
                    ptr = ref->get_write_head();
                else {
                    ptr = ptr == ref.data() ? ref->get_write_ptr() : ptr - 1;
                    if (ptr == ref->get_write_head())
                        ptr = nullptr;
                }
            }

            target_pointer ref;
            pointer ptr = ref->get_read_head();
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
