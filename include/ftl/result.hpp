#ifndef FTL_RESULT_HPP
#define FTL_RESULT_HPP

#if __STDC_HOSTED__ == 1
#include <stdexcept>
#endif

#ifdef __cpp_exceptions
namespace ftl
{
    struct bad_result_access : public std::exception {
        bad_result_access() = default;
        const char* what() const noexcept override { return "Bad result access"; } 
    };
}
#endif

#include "utility.hpp"

namespace ftl
{
    namespace detail {
        template <typename T,
                  typename E,
                  bool HoldsReference = std::is_reference<T>::value,
                  bool TriviallyDestructibleT = std::is_trivially_destructible<T>::value,
                  bool TriviallyDestructibleE = std::is_trivially_destructible<E>::value>
        struct result_storage_type;

        template <typename T, typename E>
        struct result_type_impl;

        struct success_tag {};
        struct error_tag {};
    }

    template <typename E>
    struct error
    {
        using decay_type = std::decay_t<E>;
        public:
            constexpr error(E value) requires (std::is_fundamental_v<decay_type>)
                : value(value) {}

            // for non-fundamental types
            constexpr error(decay_type&& value) requires (not std::is_fundamental_v<decay_type>)
                : value(FTL_MOVE(value)) {}

            constexpr operator const E&() const & {
                return value;
            }

            constexpr operator E&&() && {
                return FTL_MOVE(value);
            }

            constexpr E&& get_and_discard() && {
                return FTL_MOVE(value);
            }

        private:
            E value;
    };
    template<class WtfGcc>
    error(WtfGcc b) -> error<typename std::decay_t<WtfGcc>>;

    template <typename T>
    class ok
    {
        using decay_type = std::decay_t<T>;
        public:
            constexpr ok(T value) requires (std::is_fundamental_v<decay_type>)
                : value(value) {}

            // for non-fundamental types
            constexpr ok(decay_type&& value) requires (not std::is_fundamental_v<decay_type>)
                : value(FTL_MOVE(value)) {}

            constexpr operator const T&() const & {
                return value;
            }

            constexpr operator T&&() && {
                return FTL_MOVE(value);
            }

            constexpr T&& get_and_discard() && {
                return FTL_MOVE(value);
            }

        private:
            T value;
    };

    template<class WtfGcc>
    ok(WtfGcc b) -> ok<typename std::decay_t<WtfGcc>>;
}

namespace ftl::detail {
    template <typename T, typename E>
    struct result_storage_type<T, E, NOT_REFERENCE, TRIVIALLY_DESTRUCTIBLE, TRIVIALLY_DESTRUCTIBLE>
    {
        static_assert(not std::is_reference<E>::value, "error type cannot be a reference");
        constexpr result_storage_type() noexcept : uninitialised(), contains_value(false) {}
        constexpr result_storage_type(const T& value)
            noexcept(std::is_nothrow_copy_constructible<T>::value)
            requires (std::is_trivially_copy_constructible<T>::value)
            : stored_value(value), contains_value(true) {}


        constexpr result_storage_type(T&& value)
            noexcept(std::is_nothrow_move_constructible<T>::value)
            requires (std::is_trivially_move_constructible<T>::value)
            : stored_value(FTL_MOVE(value)), contains_value(true) {}

        constexpr result_storage_type(error_tag, const E& error)
            noexcept (std::is_nothrow_copy_constructible<E>::value)
            requires (std::is_trivially_copy_constructible<E>::value)
            : stored_error(error), contains_value(false) {}

        constexpr result_storage_type(error_tag, E&& error)
            noexcept (std::is_nothrow_move_constructible<E>::value)
            requires (std::is_trivially_move_constructible<E>::value)
            : stored_error(FTL_MOVE(error)), contains_value(false) {}

        constexpr result_storage_type(const result_storage_type&) noexcept = default;
        constexpr result_storage_type(result_storage_type&&) noexcept = default;
        constexpr result_storage_type& operator=(const result_storage_type&) noexcept = default;
        constexpr result_storage_type& operator=(result_storage_type&&) noexcept = default;

        ~result_storage_type() = default;

        union {
            T       stored_value;
            E       stored_error;
            char    uninitialised;
        };

        bool contains_value;
    };

    template <typename T, typename E, bool TriviallyDestructibleValue, bool TriviallyDestructibleError>
    struct result_storage_type<T, E, NOT_REFERENCE, TriviallyDestructibleValue, TriviallyDestructibleError>
    {
        static_assert(not std::is_reference<E>::value, "error type cannot be a reference");
        constexpr result_storage_type() noexcept : uninitialised(), contains_value(false) {}
        constexpr result_storage_type(const T& value)
            noexcept(std::is_nothrow_copy_constructible<T>::value)
            requires (std::is_trivially_copy_constructible<T>::value)
            : stored_value(value), contains_value(true) {}


        constexpr result_storage_type(T&& value)
            noexcept(std::is_nothrow_move_constructible<T>::value)
            requires (std::is_trivially_move_constructible<T>::value)
            : stored_value(FTL_MOVE(value)), contains_value(true) {}

        constexpr result_storage_type(error_tag, const E& error)
            noexcept (std::is_nothrow_copy_constructible<E>::value)
            requires (std::is_trivially_copy_constructible<E>::value)
            : stored_error(error), contains_value(false) {}

        constexpr result_storage_type(error_tag, E&& error)
            noexcept (std::is_nothrow_move_constructible<E>::value)
            requires (std::is_trivially_move_constructible<E>::value)
            : stored_error(FTL_MOVE(error)), contains_value(false) {}

        constexpr result_storage_type(const result_storage_type&) noexcept = default;
        constexpr result_storage_type(result_storage_type&&) noexcept = default;
        constexpr result_storage_type& operator=(const result_storage_type&) noexcept = default;
        constexpr result_storage_type& operator=(result_storage_type&&) noexcept = default;

        ~result_storage_type() {
            if constexpr (not TriviallyDestructibleValue)
                if (contains_value)
                    stored_value.~T();
            if constexpr (not TriviallyDestructibleError)
                if (not contains_value)
                    stored_error.~E();
        }

        union {
            T       stored_value;
            E       stored_error;
            char    uninitialised;
        };

        bool contains_value;
    };

    // Reference type
    template <typename T, typename E>
    struct result_storage_type<T, E, REFERENCE, TRIVIALLY_DESTRUCTIBLE, TRIVIALLY_DESTRUCTIBLE>
    {
        static_assert(not std::is_void<T>::value);
        static_assert(not std::is_reference<E>::value, "error type cannot be a reference");
        constexpr result_storage_type() noexcept : uninitialised(), contains_value(false) {}
        constexpr result_storage_type(T&& value) : stored_pointer(&value), contains_value(true) {}
        constexpr result_storage_type(error_tag, const E& error)
            noexcept (std::is_nothrow_copy_constructible<E>::value)
            requires (std::is_trivially_copy_constructible<E>::value)
            : stored_error(error), contains_value(false) {}

        constexpr result_storage_type(error_tag, E&& error)
            noexcept (std::is_nothrow_move_constructible<E>::value)
            requires (std::is_trivially_move_constructible<E>::value)
            : stored_error(FTL_MOVE(error)), contains_value(false) {}

        union {
            std::decay_t<T>* const  stored_pointer;
            E                       stored_error;
            char                    uninitialised;
        };

        bool contains_value;
    };

    // Void specialisations
    template <typename E>
    struct result_storage_type<void, E, NOT_REFERENCE, false, TRIVIALLY_DESTRUCTIBLE>
    {
        static_assert(not std::is_reference<E>::value, "error type cannot be a reference");
        struct dummy {};

        constexpr result_storage_type() noexcept : dummy_value(), contains_value(false) {}
        constexpr result_storage_type(success_tag) noexcept : dummy_value(), contains_value(true) {}
        constexpr result_storage_type(error_tag, const E& error)
            noexcept (std::is_nothrow_copy_constructible<E>::value)
            requires (std::is_trivially_copy_constructible<E>::value)
            : stored_error(error), contains_value(false) {}

        constexpr result_storage_type(error_tag, E&& error)
            noexcept (std::is_nothrow_move_constructible<E>::value)
            requires (std::is_trivially_move_constructible<E>::value)
            : stored_error(FTL_MOVE(error)), contains_value(false) {}

        union {
            dummy   dummy_value;
            E       stored_error;
        };

        bool    contains_value;
    };

    template <typename E>
    struct result_storage_type<void, E, NOT_REFERENCE, false, NOT_TRIVIALLY_DESTRUCTIBLE>
    {
        static_assert(not std::is_reference<E>::value, "error type cannot be a reference");
        struct dummy {};

        constexpr result_storage_type() noexcept : dummy_value(), contains_value(false) {}
        constexpr result_storage_type(success_tag) noexcept : dummy_value(), contains_value(true) {}
        constexpr result_storage_type(error_tag, const E& error)
            noexcept (std::is_nothrow_copy_constructible<E>::value)
            requires (std::is_trivially_copy_constructible<E>::value)
            : stored_error(error), contains_value(false) {}

        constexpr result_storage_type(error_tag, E&& error)
            noexcept (std::is_nothrow_move_constructible<E>::value)
            requires (std::is_trivially_move_constructible<E>::value)
            : stored_error(FTL_MOVE(error)), contains_value(false) {}

        ~result_storage_type() {
            if (not contains_value)
                stored_error.~E();
        }

        union {
            dummy   dummy_value;
            E       stored_error;
        };

        bool    contains_value;
    };


    template <typename T, typename E>
    struct result_type_impl : result_storage_type<T, E>
    {
        using result_storage_type<T, E>::result_storage_type;

        constexpr result_type_impl() noexcept = default;
        constexpr result_type_impl(const result_type_impl&) noexcept = default;

        /*
        constexpr result_type_impl(const T& v) noexcept(std::is_nothrow_copy_constructible_v<T>) = default;
        constexpr result_type_impl(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

        constexpr result_type_impl(error_tag, const E& error) noexcept(std::is_nothrow_copy_constructible_v<T>) = default;
        constexpr result_type_impl(error_tag, E&& error) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
        */

        template <typename... Args> constexpr void construct(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args...>)
            requires(not std::is_reference_v<T>)
        {
            new(&(this->stored_value)) T (FTL_FORWARD(args)...);
            this->contains_value = true;
        }

        template <typename Moved> constexpr void construct_with(Moved&& rhs)
            noexcept(std::is_nothrow_move_constructible_v<Moved>)
            requires(not std::is_reference_v<T>)
        {
            new(&(this->stored_value)) T (FTL_MOVE(rhs));
            this->contains_value = true;
        }

        template <typename... Args>
        constexpr void construct_error(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        {
            new (&(this->stored_error)) E (FTL_FORWARD(args)...);
            this->contains_value = false;
        }

        constexpr bool is_value() const noexcept { return this->contains_value; }
        constexpr bool is_error() const noexcept { return !this->contains_value; }

        constexpr T& get() & {
            if (this->contains_value) {
                if constexpr (std::is_reference<T>::value)
                    return *(this->stored_pointer);
                else
                    return this->stored_value;
            }
            FTL_THROW_OR_PANIC(bad_result_access{});
        }

        constexpr const T& get() const & {
            if (this->contains_value) {
                if constexpr (std::is_reference<T>::value)
                    return const_cast<const T&>(*(this->stored_pointer));
                else
                    return this->stored_value;
            }
            FTL_THROW_OR_PANIC(bad_result_access{});
        }

        constexpr T&& get() && requires (not std::is_reference<T>::value) {
            if (this->contains_value) {
                return FTL_MOVE(this->stored_value);
            }
            FTL_THROW_OR_PANIC(bad_result_access{});
        }

        constexpr const T&& get() const && requires (not std::is_reference<T>::value) {
            if (this->contains_value) {
                return FTL_MOVE(this->stored_value);
            }
            FTL_THROW_OR_PANIC(bad_result_access{});
        }

        constexpr E& get_error() & {
            if (!this->contains_value) {
                return this->stored_error;
            }
            FTL_THROW_OR_PANIC(bad_result_access{});
        }

        constexpr const E& get_error() const & {
            if (!this->contains_value) {
                return this->stored_error;
            }
            FTL_THROW_OR_PANIC(bad_result_access{});
        }

        constexpr E&& get_error() && {
            if (!this->contains_value) {
                return FTL_MOVE(this->stored_error);
            }
            FTL_THROW_OR_PANIC(bad_result_access{});
        }

        constexpr void destroy_value() { get().~T(); }
    };
}

namespace ftl
{
    template <typename T, typename E>
    struct result : detail::result_type_impl<T, E>
    {
        static_assert(not std::is_reference<E>::value, "error type cannot be a reference");
        static_assert(not std::is_same<void, E>::value, "error type cannot be a void type");
        public:
            using value_type = T;
            using error_type = E;

            constexpr result() = delete;

            constexpr result(const result&) noexcept(
                    std::is_nothrow_copy_constructible<value_type>::value
                 && std::is_nothrow_copy_constructible<error_type>::value) = default;

            constexpr result(result&&) noexcept(
                    std::is_nothrow_move_constructible<value_type>::value
                 && std::is_nothrow_move_constructible<error_type>::value) = default;

            constexpr result& operator=(const result&) noexcept(
                    std::is_nothrow_copy_assignable<value_type>::value
                 && std::is_nothrow_copy_assignable<error_type>::value) = default;

            template <typename U> requires (std::is_convertible<U&&, T>::value)
            constexpr result(ok<U>&& v) noexcept(std::is_nothrow_convertible<U&&, T>::value) {
                this->construct_with(FTL_MOVE(v).get_and_discard());
            }

            template <typename U> requires (std::is_convertible<U&&, E>::value)
            constexpr result(error<U>&& v) noexcept(std::is_nothrow_convertible<U&&, E>::value) {
                this->construct_error(FTL_MOVE(v).get_and_discard());
            }

            constexpr bool is_ok() const noexcept { return this->is_value(); }
            constexpr bool is_error() const noexcept { return this->template result_type_impl<T,E>::is_error(); }

            constexpr bool contains(const std::decay_t<T>& t) const noexcept { return is_ok() ? t == value() : false; }
            constexpr bool contains_error(const std::decay_t<E>& e) const noexcept { return is_error() ? e == error() : false; }

            constexpr const T& value() const & {
                if (is_error()) FTL_THROW_OR_PANIC(bad_result_access{});
                return this->get();
            }

            constexpr T&& value() && {
                if (is_error()) FTL_THROW_OR_PANIC(bad_result_access{});
                return this->get();
            }

            constexpr const E& error() const & {
                if (is_ok()) FTL_THROW_OR_PANIC(bad_result_access{});
                return this->get_error();
            }

            constexpr E&& error() && {
                if (is_ok()) FTL_THROW_OR_PANIC(bad_result_access{});
                return this->get_error();
            }
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
