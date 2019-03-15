// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_FLAG_SET_HPP_INCLUDED
#define TYPE_SAFE_FLAG_SET_HPP_INCLUDED

#include <climits>
#include <cstdint>
#include <type_traits>

#include <type_safe/flag.hpp>
#include <type_safe/types.hpp>

namespace type_safe
{
/// \exclude
namespace detail
{
    template <typename Enum, typename = void>
    struct is_flag_set : std::false_type
    {};

    template <typename Enum>
    struct is_flag_set<Enum, decltype(static_cast<void>(Enum::_flag_set_size))> : std::is_enum<Enum>
    {};

    template <typename Enum>
    constexpr typename std::enable_if<is_flag_set<Enum>::value, std::size_t>::type
        flag_set_size() noexcept
    {
        return static_cast<std::size_t>(Enum::_flag_set_size);
    }

    template <typename Enum>
    constexpr typename std::enable_if<!is_flag_set<Enum>::value, std::size_t>::type
        flag_set_size() noexcept
    {
        return 0u;
    }
} // namespace detail

/// Traits for the enum used in a [ts::flag_set]().
///
/// For each enum that should be used with [ts::flag_set]() it must provide the following interface:
/// * Inherit from [std::true_type]().
/// * `static constexpr std::size_t size()` that returns the number of enumerators.
///
/// The default specialization automatically works for enums that have an enumerator
/// `_flag_set_size`, whose value is the number of enumerators. But you can also specialize the
/// traits for your own enums. Enums which work with [ts::flag_set]() are called flags.
///
/// \requires For all specializations the enum must be contiguous starting at `0`,
/// simply don't set an explicit value to the enumerators.
template <typename Enum>
struct flag_set_traits : detail::is_flag_set<Enum>
{
    static constexpr std::size_t size() noexcept
    {
        return detail::flag_set_size<Enum>();
    }
};

/// Tag type to mark a [ts::flag_set]() without any flags set.
struct noflag_t
{
    constexpr noflag_t() {}
};

/// Tag object of type [ts::noflag_t]().
constexpr noflag_t noflag;

/// \exclude
namespace detail
{
    template <std::size_t Size, typename = void>
    struct select_flag_set_int
    {
        static_assert(Size != 0u,
                      "number of bits not supported, complain loud enough so I'll do it");
    };

/// \exclude
#define TYPE_SAFE_DETAIL_SELECT(Min, Max, Type)                                                    \
    template <std::size_t Size>                                                                    \
    struct select_flag_set_int<Size, typename std::enable_if<(Size > Min && Size <= Max)>::type>   \
    {                                                                                              \
        using type = Type;                                                                         \
    };

    TYPE_SAFE_DETAIL_SELECT(0u, 8u, std::uint_least8_t)
    TYPE_SAFE_DETAIL_SELECT(8u, 16u, std::uint_least16_t)
    TYPE_SAFE_DETAIL_SELECT(16u, 32u, std::uint_least32_t)
    TYPE_SAFE_DETAIL_SELECT(32u, sizeof(std::uint_least64_t) * CHAR_BIT, std::uint_least64_t)

#undef TYPE_SAFE_DETAIL_SELECT

    template <typename Enum, typename Tag = void>
    class flag_set_impl
    {
    public:
        using traits   = flag_set_traits<Enum>;
        using int_type = typename select_flag_set_int<traits::size()>::type;

        static constexpr flag_set_impl all_set()
        {
            return flag_set_impl(int_type((int_type(1) << traits::size()) - int_type(1)));
        }
        static constexpr flag_set_impl none_set()
        {
            return flag_set_impl(int_type(0));
        }

        explicit constexpr flag_set_impl(const Enum& e) : bits_(mask(e)) {}
        template <typename Tag2>
        explicit constexpr flag_set_impl(const flag_set_impl<Enum, Tag2>& other)
        : bits_(other.bits_)
        {}

        constexpr flag_set_impl set(const Enum& e) const
        {
            return flag_set_impl(bits_ | mask(e));
        }
        constexpr flag_set_impl reset(const Enum& e) const
        {
            return flag_set_impl(bits_ & ~mask(e));
        }
        constexpr flag_set_impl toggle(const Enum& e) const
        {
            return flag_set_impl(bits_ ^ mask(e));
        }

        constexpr flag_set_impl toggle_all() const
        {
            return flag_set_impl(bits_ ^ all_set().to_int());
        }

        constexpr int_type to_int() const
        {
            return bits_;
        }

        constexpr bool is_set(const Enum& e) const
        {
            return (bits_ & mask(e)) != int_type(0u);
        }

        constexpr flag_set_impl bitwise_or(const flag_set_impl& other) const
        {
            return flag_set_impl(bits_ | other.bits_);
        }

        constexpr flag_set_impl bitwise_xor(const flag_set_impl& other) const
        {
            return flag_set_impl(bits_ ^ other.bits_);
        }

        constexpr flag_set_impl bitwise_and(const flag_set_impl& other) const
        {
            return flag_set_impl(bits_ & other.bits_);
        }

    private:
        static constexpr int_type mask(const Enum& e)
        {
            return int_type(int_type(1u) << static_cast<std::size_t>(e));
        }

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value
                                                                 && std::is_unsigned<T>::value>>
        explicit constexpr flag_set_impl(T bits) : bits_(int_type(bits))
        {}

        int_type bits_;

        template <typename Enum2, typename Tag2>
        friend class flag_set_impl;
    };

    template <typename Enum>
    using flag_combo = flag_set_impl<Enum, struct combo_tag>;

    template <typename Enum>
    using flag_mask = flag_set_impl<Enum, struct mask_tag>;

    template <typename Enum>
    constexpr bool operator==(const flag_combo<Enum>& a, noflag_t)
    {
        return a == a.none_set();
    }
    template <typename Enum>
    constexpr bool operator==(noflag_t, const flag_combo<Enum>& a)
    {
        return a == a.none_set();
    }
    template <typename Enum>
    constexpr bool operator==(const flag_combo<Enum>& a, const flag_combo<Enum>& b)
    {
        return a.to_int() == b.to_int();
    }
    template <typename Enum>
    constexpr bool operator==(const flag_combo<Enum>& a, const Enum& b)
    {
        return a == flag_combo<Enum>(b);
    }
    template <typename Enum>
    constexpr bool operator==(const Enum& a, const flag_combo<Enum>& b)
    {
        return flag_combo<Enum>(a) == b;
    }

    template <typename Enum>
    constexpr bool operator!=(const flag_combo<Enum>& a, noflag_t b)
    {
        return !(a == b);
    }
    template <typename Enum>
    constexpr bool operator!=(noflag_t a, const flag_combo<Enum>& b)
    {
        return !(a == b);
    }
    template <typename Enum>
    constexpr bool operator!=(const flag_combo<Enum>& a, const flag_combo<Enum>& b)
    {
        return !(a == b);
    }
    template <typename Enum>
    constexpr bool operator!=(const flag_combo<Enum>& a, const Enum& b)
    {
        return !(a == b);
    }
    template <typename Enum>
    constexpr bool operator!=(const Enum& a, const flag_combo<Enum>& b)
    {
        return !(a == b);
    }

    template <typename Enum>
    constexpr flag_combo<Enum> operator|(const flag_combo<Enum>& a, const flag_combo<Enum>& b)
    {
        return a.bitwise_or(b);
    }
    template <typename Enum>
    constexpr flag_combo<Enum> operator|(const flag_combo<Enum>& a, const Enum& b)
    {
        return a | flag_combo<Enum>(b);
    }
    template <typename Enum>
    constexpr flag_combo<Enum> operator|(const Enum& a, const flag_combo<Enum>& b)
    {
        return flag_combo<Enum>(a) | b;
    }

    template <typename Enum>
    constexpr bool operator==(const flag_mask<Enum>& a, const flag_mask<Enum>& b)
    {
        return a.to_int() == b.to_int();
    }
    template <typename Enum>
    constexpr bool operator==(const flag_mask<Enum>& a, noflag_t)
    {
        return a == a.none_set();
    }
    template <typename Enum>
    constexpr bool operator==(noflag_t, const flag_mask<Enum>& a)
    {
        return a == a.none_set();
    }
    template <typename Enum>
    constexpr bool operator!=(const flag_mask<Enum>& a, const flag_mask<Enum>& b)
    {
        return !(a == b);
    }
    template <typename Enum>
    constexpr bool operator!=(const flag_mask<Enum>& a, noflag_t b)
    {
        return !(a == b);
    }
    template <typename Enum>
    constexpr bool operator!=(noflag_t a, const flag_mask<Enum>& b)
    {
        return !(a == b);
    }

    template <typename Enum>
    constexpr flag_mask<Enum> operator&(const flag_mask<Enum>& a, const flag_mask<Enum>& b)
    {
        return a.bitwise_and(b);
    }

    template <typename Enum>
    constexpr flag_mask<Enum> operator~(const flag_combo<Enum>& a)
    {
        return flag_mask<Enum>(a.toggle_all());
    }
    template <typename Enum>
    constexpr flag_combo<Enum> operator~(const flag_mask<Enum>& a)
    {
        return flag_combo<Enum>(a.toggle_all());
    }

    template <typename T, typename Enum>
    struct is_flag_combo : std::false_type
    {};

    template <typename Enum>
    struct is_flag_combo<Enum, Enum> : flag_set_traits<Enum>
    {};

    template <typename Enum>
    struct is_flag_combo<flag_combo<Enum>, Enum> : flag_set_traits<Enum>
    {};

    template <typename T>
    using enable_flag = typename std::enable_if<flag_set_traits<T>::value>::type;

    template <typename T, typename Enum>
    using enable_flag_combo = typename std::enable_if<is_flag_combo<T, Enum>::value>::type;

    struct get_flag_set_impl
    {
        template <class Set>
        static constexpr auto get(const Set& set) -> decltype(set.flags_)
        {
            return set.flags_;
        }
    };
} // namespace detail

/// Represents a combination of multiple flags.
///
/// This type is created when you write `a | b`,
/// where `a` and `b` are enumerators of a flag.
///
/// Objects of this type and the flags themselves
/// are flag combinations.
/// You can compare two flag combinations,
/// combine two with `|`
/// and use them in [ts::flag_set]() to set or toggle a combination of flags.
/// Creating the complement with `~` will create a [ts::flag_mask]().
///
/// \requires `Enum` must be a flag,
/// i.e. valid with the [ts::flag_set_traits]().
template <typename Enum>
using flag_combo = detail::flag_combo<Enum>;

/// Represents a mask of flags.
///
/// This type is created when you write `~a`,
/// where `a` is the enumerator of a flag.
///
/// Objects of this type are flag masks.
/// You can compare two flag masks,
/// combine them with `&`
/// and use them in [ts::flag_set]() to clear a combination of flags.
/// Creating the complement with `~` will create a [ts::flag_combo]().
///
/// \requires `Enum` must be a flag,
/// i.e. valid with the [ts::flag_set_traits]().
template <typename Enum>
using flag_mask = detail::flag_mask<Enum>;

/// Converts a flag mask to a flag combination.
/// \returns The flag combination with the same value as the mask.
/// \notes As you cannot use a mask to set flags in a [ts::flag_set](),
/// you cannot write `~a` to set all flags except `a` directly,
/// you have to be explicit.
template <typename Enum>
constexpr flag_combo<Enum> combo(const flag_mask<Enum>& mask) noexcept
{
    return flag_combo<Enum>(mask);
}

/// Converts a flag combination to a flag mask.
/// \returns The flag mask with the same value as the flag combination.
/// \notes (1) does not participate in overload resolution,
/// unless the argument is a flagg.
/// \notes As you cannot use a combination to clear flags in a [ts::flag_set](),
/// you cannot write `a` to clear all flags except `a` directly,
/// you have to be explicit.
/// \group mask_combo
/// \param 1
/// \exclude
template <typename Enum, typename = detail::enable_flag<Enum>>
constexpr flag_mask<Enum> mask(const Enum& flag) noexcept
{
    return flag_mask<Enum>(flag);
}

/// \group mask_combo
template <typename Enum>
constexpr flag_mask<Enum> mask(const flag_combo<Enum>& combo) noexcept
{
    return flag_mask<Enum>(combo);
}

/// A set of flags where each one can either be `0` or `1`.
///
/// Each enumeration member represents the index of one bit.
///
/// Unlike [ts::flag_combo]() or [ts::flag_mask]() this does not have this semantic distinction.
/// It is just a generic container of flags,
/// which can be set, cleared or toggled.
/// It can be interpreted as either a flag combination or flag mask, however.
///
/// \requires `Enum` must be a flag,
/// i.e. valid with the [ts::flag_set_traits]().
template <typename Enum>
class flag_set
{
    static_assert(std::is_enum<Enum>::value, "not an enum");
    static_assert(flag_set_traits<Enum>::value, "invalid enum for flag_set");

public:
    //=== constructors/assignment ===//
    /// \effects Creates a set where all flags are set to `0`.
    /// \group ctor_null
    constexpr flag_set() noexcept : flags_(detail::flag_set_impl<Enum>::none_set()) {}

    /// \group ctor_null
    constexpr flag_set(noflag_t) noexcept : flag_set() {}

    /// \effects Creates a set where all bits are set to `0` except the given ones.
    /// \notes This constructor only participates in overload resolution
    /// if the argument is a flag combination.
    template <typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
    constexpr flag_set(const FlagCombo& combo) noexcept : flags_(combo)
    {}

    /// \effects Same as `*this = flag_set(combo)`.
    template <typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
    flag_set& operator=(const FlagCombo& combo) noexcept
    {
        return *this = flag_set(combo);
    }

    /// \effects Same as [*reset_all]().
    flag_set& operator=(noflag_t) noexcept
    {
        reset_all();
        return *this;
    }

    //=== flag operation ===//
    /// \effects Sets the specified flag to `1` (1)/`value` (2/3).
    /// \notes (2) does not participate in overload resolution unless `T` is a boolean-like type.
    /// \group set
    void set(const Enum& flag) noexcept
    {
        flags_ = flags_.set(flag);
    }

    /// \group set
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_boolean<T>>
    void set(const Enum& flag, T value) noexcept
    {
        if (value)
            set(flag);
        else
            reset(flag);
    }

    /// \group set
    void set(const Enum& f, flag value) noexcept
    {
        set(f, value == true);
    }

    /// \effects Sets the specified flag to `0`.
    void reset(const Enum& flag) noexcept
    {
        flags_ = flags_.reset(flag);
    }

    /// \effects Toggles the specified flag.
    void toggle(const Enum& flag) noexcept
    {
        flags_ = flags_.toggle(flag);
    }

    /// \effects Sets/resets/toggles all flags.
    /// \group all
    void set_all() noexcept
    {
        flags_ = flags_.all_set();
    }

    /// \group all
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_boolean<T>>
    void set_all(T value) noexcept
    {
        if (value)
            set_all();
        else
            reset_all();
    }

    /// \group all
    void set_all(flag value) noexcept
    {
        set_all(value == true);
    }

    /// \group all
    void reset_all() noexcept
    {
        flags_ = flags_.none_set();
    }

    /// \group all
    void toggle_all() noexcept
    {
        flags_ = flags_.toggle_all();
    }

    /// \returns Whether or not the specified flag is set.
    constexpr bool is_set(const Enum& flag) const noexcept
    {
        return flags_.is_set(flag);
    }

    /// \returns Same as `flag(is_set(flag))`.
    constexpr flag as_flag(const Enum& flag) const noexcept
    {
        return is_set(flag);
    }

    //=== accessors ===//
    /// \returns Whether any flag is set.
    constexpr bool any() const noexcept
    {
        return flags_.to_int() != flags_.none_set().to_int();
    }

    /// \returns Whether all flags are set.
    constexpr bool all() const noexcept
    {
        return flags_.to_int() == flags_.all_set().to_int();
    }

    /// \returns Whether no flag is set.
    constexpr bool none() const noexcept
    {
        return !any();
    }

    /// \returns An integer where each bit has the value of the corresponding flag.
    /// \requires `T` must be an unsigned integer type with enough bits.
    template <typename T>
    constexpr T to_int() const noexcept
    {
        static_assert(std::is_unsigned<T>::value
                          && sizeof(T) * CHAR_BIT >= flag_set_traits<Enum>::size(),
                      "invalid integer type, lossy conversion");
        return flags_.to_int();
    }

    //=== bitwise operations ===//
    /// \returns A set with all the flags flipped.
    constexpr flag_set operator~() const noexcept
    {
        return flag_set(flag_combo<Enum>(flags_.toggle_all()));
    }

    /// \effects Sets all flags that are set in the given flag combination.
    /// \returns `*this`
    /// \notes This operator does not participate in overload resolution,
    /// unless the argument is a flag combination.
    /// If you truly want to write `set |= ~a`,
    /// i.e. set all flags except `a`, use `set |= combo(~a)`.
    template <typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
    flag_set& operator|=(const FlagCombo& other) noexcept
    {
        flags_ = flags_.bitwise_or(detail::flag_set_impl<Enum>(other));
        return *this;
    }

    /// \effects Toggles all flags that are set in the given flag combination.
    /// \returns `*this`
    /// \notes This operator does not participate in overload resolution,
    /// unless the argument is a flag combination.
    /// If you truly want to write `set ^= ~a`,
    /// i.e. toggle all flags except `a`, use `set ^= combo(~a)`.
    template <typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
    flag_set& operator^=(const FlagCombo& other) noexcept
    {
        flags_ = flags_.bitwise_xor(detail::flag_set_impl<Enum>(other));
        return *this;
    }

    /// \effects Clears all flags that aren't set in the given flag mask.
    /// \returns `*this`
    /// \notes This operator does not participate in overload resolution,
    /// unless the argument is a flag mask.
    /// If you truly want to write `set &= a`,
    /// i.e. clear all flags except `a`, use `set &= mask(a)`.
    flag_set& operator&=(const flag_mask<Enum>& other) noexcept
    {
        flags_ = flags_.bitwise_and(detail::flag_set_impl<Enum>(other));
        return *this;
    }

private:
    detail::flag_set_impl<Enum> flags_;

    friend detail::get_flag_set_impl;
};

/// Converts a [ts::flag_set]() to a flag combination.
/// \returns The flag combination with the same value as the set.
template <typename Enum>
constexpr flag_combo<Enum> combo(const flag_set<Enum>& set) noexcept
{
    return flag_combo<Enum>(detail::get_flag_set_impl::get(set));
}

/// Converts a [ts::flag_set]() to a flag mask.
/// \returns The flag mask with the same value as the set.
template <typename Enum>
constexpr flag_mask<Enum> mask(const flag_set<Enum>& set) noexcept
{
    return flag_mask<Enum>(detail::get_flag_set_impl::get(set));
}

/// `flag_set` equality comparison.
/// \returns Whether both flag sets have the same combination of flags set/not set.
/// \group flag_set_equal flag_set equality comparison
template <typename Enum>
constexpr bool operator==(const flag_set<Enum>& a, const flag_set<Enum>& b) noexcept
{
    return combo(a) == combo(b);
}

/// \group flag_set_equal
template <typename Enum>
constexpr bool operator==(const flag_set<Enum>& a, noflag_t b) noexcept
{
    return combo(a) == b;
}

/// \group flag_set_equal
template <typename Enum>
constexpr bool operator==(noflag_t a, const flag_set<Enum>& b) noexcept
{
    return a == combo(b);
}

/// \group flag_set_equal
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr bool operator==(const flag_set<Enum>& a, const FlagCombo& b) noexcept
{
    return combo(a) == b;
}

/// \group flag_set_equal
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr bool operator==(const FlagCombo& a, const flag_set<Enum>& b) noexcept
{
    return a == combo(b);
}

/// \group flag_set_equal
template <typename Enum>
constexpr bool operator!=(const flag_set<Enum>& a, const flag_set<Enum>& b) noexcept
{
    return !(a == b);
}

/// \group flag_set_equal
template <typename Enum>
constexpr bool operator!=(const flag_set<Enum>& a, noflag_t b) noexcept
{
    return !(a == b);
}

/// \group flag_set_equal
template <typename Enum>
constexpr bool operator!=(noflag_t a, const flag_set<Enum>& b) noexcept
{
    return !(a == b);
}

/// \group flag_set_equal
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr bool operator!=(const flag_set<Enum>& a, const FlagCombo& b) noexcept
{
    return !(a == b);
}

/// \group flag_set_equal
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr bool operator!=(const FlagCombo& a, const flag_set<Enum>& b) noexcept
{
    return !(a == b);
}

/// \returns The same as `a Op= b`.
/// \group bitwise_op Bitwise operations for flag_set
/// \param 2
/// \exclude
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr flag_set<Enum> operator|(const flag_set<Enum>& a, const FlagCombo& b)
{
    return combo(a).bitwise_or(flag_combo<Enum>(b));
}
/// \group bitwise_op
/// \param 2
/// \exclude
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr flag_set<Enum> operator|(const FlagCombo& a, const flag_set<Enum>& b)
{
    return b | a;
}
/// \group bitwise_op
/// \param 2
/// \exclude
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr flag_set<Enum> operator^(const flag_set<Enum>& a, const FlagCombo& b)
{
    return combo(a).bitwise_xor(flag_combo<Enum>(b));
}
/// \group bitwise_op
/// \param 2
/// \exclude
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr flag_set<Enum> operator^(const FlagCombo& a, const flag_set<Enum>& b)
{
    return b ^ a;
}
/// \group bitwise_op
template <typename Enum>
constexpr flag_set<Enum> operator&(const flag_set<Enum>& a, const flag_mask<Enum>& b)
{
    return combo(mask(a).bitwise_and(b));
}
/// \group bitwise_op
template <typename Enum>
constexpr flag_set<Enum> operator&(const flag_mask<Enum>& a, const flag_set<Enum>& b)
{
    return b & a;
}

/// Checks whether a combination of flags is set in `a`.
/// \returns `true` if all the flags set in `b` are also set in `a`,
/// `false` otherwise.
/// \notes These functions do not participate in overload resolution,
/// unless `FlagCombo` is a flag operation.
/// \group bitwise_and_check Bitwise and for flag_set
/// \param 2
/// \exclude
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr bool operator&(const flag_set<Enum>& a, const FlagCombo& b)
{
    return static_cast<bool>(combo(a).bitwise_and(flag_combo<Enum>(b)).to_int());
}
/// \group bitwise_and_check
/// \param 2
/// \exclude
template <typename Enum, typename FlagCombo, typename = detail::enable_flag_combo<FlagCombo, Enum>>
constexpr bool operator&(const FlagCombo& a, const flag_set<Enum>& b)
{
    return b & a;
}
} // namespace type_safe

/// Creates a [ts::flag_mask]() for the single enum value.
/// \returns A [ts::flag_mask]() where all bits are set,
/// unless the given one.
/// \notes This function does not participate in overload resolution,
/// unless `Enum` is an `enum` where the [ts::flag_set_traits]() are specialized.
/// \param 1
/// \exclude
template <typename Enum, typename = type_safe::detail::enable_flag<Enum>>
constexpr type_safe::flag_mask<Enum> operator~(const Enum& e) noexcept
{
    return type_safe::flag_mask<Enum>::all_set().reset(e);
}

/// Creates a [ts::flag_combo]() from two enums.
/// \returns A [ts::flag_combo]() where the two given bits are set.
/// \notes These functions do not participate in overload resolution,
/// unless `Enum` is an `enum` where the [ts::flag_set_traits]() are specialized.
/// \param 1
/// \exclude
template <typename Enum, typename = type_safe::detail::enable_flag<Enum>>
constexpr type_safe::flag_combo<Enum> operator|(const Enum& a, const Enum& b) noexcept
{
    return type_safe::flag_combo<Enum>(a) | b;
}

#endif // TYPE_SAFE_FLAG_SET_HPP_INCLUDED
