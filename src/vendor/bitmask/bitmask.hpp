#pragma once

/*
    Bitmask
    =======

    A generic implementation of the BitmaskType C++ concept
    http://en.cppreference.com/w/cpp/concept/BitmaskType

    Version: 1.1

    Latest version and documentation:
        https://github.com/oliora/bitmask

    Copyright (c) 2016-2017 Andrey Upadyshev (oliora@gmail.com)

    Distributed under the Boost Software License, Version 1.0.
    See http://www.boost.org/LICENSE_1_0.txt

    Changes history
    ---------------
    v1.1:
        - Change namespace from `boost` to `bitmask`
        - Add CMake package
          https://github.com/oliora/bitmask/issues/1
    v1.0:
        - Initial release
 */

#include <type_traits>
#include <functional>  // for std::hash
#include <cassert>


namespace bitmask {

    namespace bitmask_detail {
        // Let's use std::void_t. It's introduced in C++17 but we can easily add it by ourself
        // See more at http://en.cppreference.com/w/cpp/types/void_t
        template<typename... Ts> struct make_void { typedef void type;};
        template<typename... Ts> using void_t = typename make_void<Ts...>::type;

        template<class T>
        struct underlying_type
        {
            static_assert(std::is_enum<T>::value, "T is not a enum type");

            using type = typename std::make_unsigned<typename std::underlying_type<T>::type>::type;
        };

        template<class T>
        using underlying_type_t = typename underlying_type<T>::type;

        template<class T, T MaxElement = T::_bitmask_max_element>
        struct mask_from_max_element
        {
            static constexpr underlying_type_t<T> max_element_value_ =
                static_cast<underlying_type_t<T>>(MaxElement);

            static_assert(max_element_value_ >= 0,
                          "Max element is negative");

            // If you really have to define a bitmask that uses the highest bit of signed type (i.e. the sign bit) then
            // define the value mask rather than the max element.
            static_assert(max_element_value_ <= (std::numeric_limits<typename std::underlying_type<T>::type>::max() >> 1) + 1,
                          "Max element is greater than the underlying type's highest bit");

            // `((value - 1) << 1) + 1` is used rather that simpler `(value << 1) - 1`
            // because latter overflows in case if `value` is the highest bit of the underlying type.
            static constexpr underlying_type_t<T> value =
                max_element_value_ ? ((max_element_value_ - 1) << 1) + 1 : 0;
        };

        template<class, class = void_t<>>
        struct has_max_element : std::false_type {};

        template<class T>
        struct has_max_element<T, void_t<decltype(T::_bitmask_max_element)>> : std::true_type {};

#if !defined _MSC_VER
        template<class, class = void_t<>>
        struct has_value_mask : std::false_type {};

        template<class T>
        struct has_value_mask<T, void_t<decltype(T::_bitmask_value_mask)>> : std::true_type {};
#else
        // MS Visual Studio 2015 (even Update 3) has weird support for expressions SFINAE
        // so I can't get a real check for `has_value_mask` to compile.
        template<class T>
        struct has_value_mask: std::integral_constant<bool, !has_max_element<T>::value> {};
#endif

        template<class T>
        struct is_valid_enum_definition : std::integral_constant<bool,
            !(has_value_mask<T>::value && has_max_element<T>::value)> {};

        template<class, class = void>
        struct enum_mask;

        template<class T>
        struct enum_mask<T, typename std::enable_if<has_max_element<T>::value>::type>
            : std::integral_constant<underlying_type_t<T>, mask_from_max_element<T>::value> {};

        template<class T>
        struct enum_mask<T, typename std::enable_if<has_value_mask<T>::value>::type>
            : std::integral_constant<underlying_type_t<T>, static_cast<underlying_type_t<T>>(T::_bitmask_value_mask)> {};

        template<class T>
        static constexpr underlying_type_t<T> disable_unused_function_warnings() noexcept
        {
            return (static_cast<T>(0) & static_cast<T>(0)).bits()
                & (static_cast<T>(0) | static_cast<T>(0)).bits()
                & (static_cast<T>(0) ^ static_cast<T>(0)).bits()
                & (~static_cast<T>(0)).bits()
                & bits(static_cast<T>(0));
        }

        template<class Assert>
        inline void constexpr_assert_failed(Assert&& a) noexcept { a(); }

        // When evaluated at compile time emits a compilation error if condition is not true.
        // Invokes the standard assert at run time.
        #define bitmask_constexpr_assert(cond) \
            ((void)((cond) ? 0 : (bitmask::bitmask_detail::constexpr_assert_failed([](){ assert(!#cond);}), 0)))

        template<class T>
        inline constexpr T checked_value(T value, T mask)
        {
            return bitmask_constexpr_assert((value & ~mask) == 0), value;
        }
    }

    template<class T>
    inline constexpr bitmask_detail::underlying_type_t<T> get_enum_mask(const T&) noexcept
    {
        static_assert(bitmask_detail::is_valid_enum_definition<T>::value,
                      "Both of _bitmask_max_element and _bitmask_value_mask are specified");
        return bitmask_detail::enum_mask<T>::value;
    }


    template<class T>
    class bitmask
    {
    public:
        using value_type = T;
        using underlying_type = bitmask_detail::underlying_type_t<T>;

        static constexpr underlying_type mask_value = get_enum_mask(static_cast<value_type>(0));

        constexpr bitmask() noexcept = default;
        constexpr bitmask(std::nullptr_t) noexcept: m_bits{0} {}

        constexpr bitmask(value_type value) noexcept
        : m_bits{bitmask_detail::checked_value(static_cast<underlying_type>(value), mask_value)} {}

        constexpr underlying_type bits() const noexcept { return m_bits; }

        constexpr explicit operator bool() const noexcept { return bits() ? true : false; }

        constexpr bitmask operator ~ () const noexcept
        {
            return bitmask{std::true_type{}, ~m_bits & mask_value};
        }

        constexpr bitmask operator & (const bitmask& r) const noexcept
        {
            return bitmask{std::true_type{}, m_bits & r.m_bits};
        }

        constexpr bitmask operator | (const bitmask& r) const noexcept
        {
            return bitmask{std::true_type{}, m_bits | r.m_bits};
        }

        constexpr bitmask operator ^ (const bitmask& r) const noexcept
        {
            return bitmask{std::true_type{}, m_bits ^ r.m_bits};
        }

        bitmask& operator |= (const bitmask& r) noexcept
        {
            m_bits |= r.m_bits;
            return *this;
        }

        bitmask& operator &= (const bitmask& r) noexcept
        {
            m_bits &= r.m_bits;
            return *this;
        }

        bitmask& operator ^= (const bitmask& r) noexcept
        {
            m_bits ^= r.m_bits;
            return *this;
        }

    private:
        template<class U>
        constexpr bitmask(std::true_type, U bits) noexcept
        : m_bits(static_cast<underlying_type>(bits)) {}

        underlying_type m_bits = 0;
    };

    template<class T>
    inline constexpr bitmask<T>
    operator & (T l, const bitmask<T>& r) noexcept { return r & l; }

    template<class T>
    inline constexpr bitmask<T>
    operator | (T l, const bitmask<T>& r) noexcept { return r | l; }

    template<class T>
    inline constexpr bitmask<T>
    operator ^ (T l, const bitmask<T>& r) noexcept { return r ^ l; }

    template<class T>
    inline constexpr bool
    operator != (const bitmask<T>& l, const bitmask<T>& r) noexcept { return l.bits() != r.bits(); }

    template<class T>
    inline constexpr bool
    operator == (const bitmask<T>& l, const bitmask<T>& r) noexcept { return ! operator != (l, r); }

    template<class T>
    inline constexpr bool
    operator != (T l, const bitmask<T>& r) noexcept { return static_cast<bitmask_detail::underlying_type_t<T>>(l) != r.bits(); }

    template<class T>
    inline constexpr bool
    operator == (T l, const bitmask<T>& r) noexcept { return ! operator != (l, r); }

    template<class T>
    inline constexpr bool
    operator != (const bitmask<T>& l, T r) noexcept { return l.bits() != static_cast<bitmask_detail::underlying_type_t<T>>(r); }

    template<class T>
    inline constexpr bool operator == (const bitmask<T>& l, T r) noexcept { return ! operator != (l, r); }

    template<class T>
    inline constexpr bool
    operator != (const bitmask_detail::underlying_type_t<T>& l, const bitmask<T>& r) noexcept { return l != r.bits(); }

    template<class T>
    inline constexpr bool
    operator == (const bitmask_detail::underlying_type_t<T>& l, const bitmask<T>& r) noexcept { return ! operator != (l, r); }

    template<class T>
    inline constexpr bool
    operator != (const bitmask<T>& l, const bitmask_detail::underlying_type_t<T>& r) noexcept { return l.bits() != r; }

    template<class T>
    inline constexpr bool
    operator == (const bitmask<T>& l, const bitmask_detail::underlying_type_t<T>& r) noexcept { return ! operator != (l, r); }

    // Allow `bitmask` to be be used as a map key
    template<class T>
    inline constexpr bool
    operator < (const bitmask<T>& l, const bitmask<T>& r) noexcept { return l.bits() < r.bits(); }

    template<class T>
    inline constexpr bitmask_detail::underlying_type_t<T>
    bits(const bitmask<T>& bm) noexcept { return bm.bits(); }


    // Implementation

    template<class T>
    constexpr typename bitmask<T>::underlying_type bitmask<T>::mask_value;
}


namespace std
{
    template<class T>
    struct hash<bitmask::bitmask<T>>
    {
        constexpr std::size_t operator() (const bitmask::bitmask<T>& op) const noexcept
        {
            using ut = typename bitmask::bitmask<T>::underlying_type;
            return std::hash<ut>{}(op.bits());
        }
    };
}

// Implementation detail macros

#define BITMASK_DETAIL_DEFINE_OPS(value_type) \
    inline constexpr bitmask::bitmask<value_type> operator & (value_type l, value_type r) noexcept { return bitmask::bitmask<value_type>{l} & r; } \
    inline constexpr bitmask::bitmask<value_type> operator | (value_type l, value_type r) noexcept { return bitmask::bitmask<value_type>{l} | r; } \
    inline constexpr bitmask::bitmask<value_type> operator ^ (value_type l, value_type r) noexcept { return bitmask::bitmask<value_type>{l} ^ r; } \
    inline constexpr bitmask::bitmask<value_type> operator ~ (value_type op) noexcept { return ~bitmask::bitmask<value_type>{op}; }                \
    inline constexpr bitmask::bitmask<value_type>::underlying_type bits(value_type op) noexcept { return bitmask::bitmask<value_type>{op}.bits(); }\
    using unused_bitmask_ ## value_type ## _t_ = decltype(bitmask::bitmask_detail::disable_unused_function_warnings<value_type>());

#define BITMASK_DETAIL_DEFINE_VALUE_MASK(value_type, value_mask) \
    inline constexpr bitmask::bitmask_detail::underlying_type_t<value_type> get_enum_mask(value_type) noexcept { return value_mask; }

#define BITMASK_DETAIL_DEFINE_MAX_ELEMENT(value_type, max_element) \
    inline constexpr bitmask::bitmask_detail::underlying_type_t<value_type> get_enum_mask(value_type) noexcept { \
        return bitmask::bitmask_detail::mask_from_max_element<value_type, value_type::max_element>::value;       \
    }


// Public macros

// Defines missing operations for a bit-mask elements enum 'value_type'
// Value mask is taken from 'value_type' definition. One should has either
// '_bitmask_value_mask' or '_bitmask_max_element' element defined.
#define BITMASK_DEFINE(value_type) \
    BITMASK_DETAIL_DEFINE_OPS(value_type)

// Defines missing operations and a value mask for
// a bit-mask elements enum 'value_type'
#define BITMASK_DEFINE_VALUE_MASK(value_type, value_mask) \
    BITMASK_DETAIL_DEFINE_VALUE_MASK(value_type, value_mask) \
    BITMASK_DETAIL_DEFINE_OPS(value_type)

// Defines missing operations and a value mask based on max element for
// a bit-mask elements enum 'value_type'
#define BITMASK_DEFINE_MAX_ELEMENT(value_type, max_element) \
    BITMASK_DETAIL_DEFINE_MAX_ELEMENT(value_type, max_element) \
    BITMASK_DETAIL_DEFINE_OPS(value_type)
