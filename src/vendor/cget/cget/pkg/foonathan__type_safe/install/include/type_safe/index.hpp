// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_INDEX_HPP_INCLUDED
#define TYPE_SAFE_INDEX_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#include <cstddef>
#endif

#include <type_safe/config.hpp>
#include <type_safe/strong_typedef.hpp>
#include <type_safe/types.hpp>

namespace type_safe
{
/// A type modelling the difference between two [ts::index_t]() objects.
///
/// It is a [ts::strong_typedef]() for [ts::ptrdiff_t]().
/// It is comparable and you can add and subtract two differences.
/// \module types
struct difference_t : strong_typedef<difference_t, ptrdiff_t>,
                      strong_typedef_op::equality_comparison<difference_t>,
                      strong_typedef_op::relational_comparison<difference_t>,
                      strong_typedef_op::unary_plus<difference_t>,
                      strong_typedef_op::unary_minus<difference_t>,
                      strong_typedef_op::addition<difference_t>,
                      strong_typedef_op::subtraction<difference_t>
{
    /// \effects Initializes it to `0`.
    constexpr difference_t() noexcept : strong_typedef(0) {}

    /// \effects Initializes it from a valid `signed` integer type.
    /// \notes This constructor does not participate in overload resolution,
    /// if `T` is not safely convertible to [ts::ptrdiff_t]().
    /// \group int_ctor
    /// \param 1
    /// \exclude
    template <typename T, typename = typename std::enable_if<
                              detail::is_safe_integer_conversion<T, std::ptrdiff_t>::value>::type>
    constexpr difference_t(T i) noexcept : strong_typedef(i)
    {}

    /// \group int_ctor
    /// \param 1
    /// \exclude
    template <typename T, class Policy,
              typename = typename std::enable_if<
                  detail::is_safe_integer_conversion<T, std::ptrdiff_t>::value>::type>
    constexpr difference_t(integer<T, Policy> i) noexcept : strong_typedef(static_cast<T>(i))
    {}
};

/// A type modelling an index into an array.
///
/// It is a [ts::strong_typedef]() for [ts::size_t]().
/// It is comparable and you can increment and decrement it,
/// as well as adding/subtracting a [ts::distance_t]().
/// \notes It has a similar interface to a `RandomAccessIterator`,
/// but without the dereference functions.
/// \module types
struct index_t : strong_typedef<index_t, size_t>,
                 strong_typedef_op::equality_comparison<index_t>,
                 strong_typedef_op::relational_comparison<index_t>,
                 strong_typedef_op::increment<index_t>,
                 strong_typedef_op::decrement<index_t>,
                 strong_typedef_op::unary_plus<index_t>
{
    /// \effects Initializes it to `0`.
    constexpr index_t() noexcept : strong_typedef(0u) {}

    /// \effects Initializes it from a valid `unsigned` integer type.
    /// \notes This constructor does not participate in overload resolution,
    /// if `T` is not safely convertible to [ts::size_t]().
    /// \group int_ctor
    /// \param 1
    /// \exclude
    template <typename T, typename = typename std::enable_if<
                              detail::is_safe_integer_conversion<T, std::size_t>::value>::type>
    constexpr index_t(T i) noexcept : strong_typedef(i)
    {}

    /// \group int_ctor
    /// \param 1
    /// \exclude
    template <typename T, class Policy,
              typename = typename std::enable_if<
                  detail::is_safe_integer_conversion<T, std::size_t>::value>::type>
    constexpr index_t(integer<T, Policy> i) noexcept : strong_typedef(static_cast<T>(i))
    {}

    /// \effects Advances the index by the distance specified in `rhs`.
    /// If `rhs` is a negative distance, it advances backwards.
    /// \requires The new index must be greater or equal to `0`.
    index_t& operator+=(const difference_t& rhs) noexcept
    {
        get(*this) = make_unsigned(make_signed(get(*this)) + get(rhs));
        return *this;
    }

    /// \effects Advances the index backwards by the distance specified in `rhs`.
    /// If `rhs` is a negative distance, it advances forwards.
    /// \requires The new index must be greater or equal to `0`.
    index_t& operator-=(const difference_t& rhs) noexcept
    {
        get(*this) = make_unsigned(make_signed(get(*this)) - get(rhs));
        return *this;
    }
};

/// \returns The given [ts::index_t]() advanced by the given [ts::distance_t]().
/// \module types
/// \group index_distance_plus
constexpr index_t operator+(const index_t& lhs, const difference_t& rhs) noexcept
{
    return index_t(make_unsigned(make_signed(get(lhs)) + get(rhs)));
}

/// \group index_distance_plus
constexpr index_t operator+(const difference_t& lhs, const index_t& rhs) noexcept
{
    return rhs + lhs;
}

/// \returns The given [ts::index_t]() advanced backwards by the given [ts::distance_t]().
/// \module types
constexpr index_t operator-(const index_t& lhs, const difference_t& rhs) noexcept
{
    return index_t(make_unsigned(make_signed(get(lhs)) - get(rhs)));
}

/// \returns Returns the distance between two indices.
/// This is the number of steps you need to increment `lhs` to reach `rhs`,
/// it is negative if `lhs > rhs`.
/// \module types
constexpr difference_t operator-(const index_t& lhs, const index_t& rhs) noexcept
{
    return difference_t(make_signed(get(lhs)) - make_signed(get(rhs)));
}

/// \exclude
namespace detail
{
    struct no_size
    {};

    template <typename Indexable>
    bool index_valid(no_size, const Indexable&, const std::size_t&)
    {
        return true;
    }

    struct non_member_size : no_size
    {};

    template <typename Indexable>
    auto index_valid(non_member_size, const Indexable& obj, const std::size_t& index)
        -> decltype(index < size(obj))
    {
        return index < size(obj);
    }

    struct member_size : non_member_size
    {};

    template <typename Indexable>
    auto index_valid(member_size, const Indexable& obj, const std::size_t& index)
        -> decltype(index < obj.size())
    {
        return index < obj.size();
    }

    template <typename T, std::size_t Size>
    bool index_valid(member_size, const T (&)[Size], const std::size_t& index)
    {
        return index < Size;
    }
} // namespace detail

/// \returns The `i`th element of `obj` by invoking its `operator[]` with the [ts::index_t]()
/// converted to `std::size_t`. \requires `index` must be a valid index for `obj`, i.e. less than
/// the size of `obj`. \exclude return \module types
template <typename Indexable>
auto at(Indexable&& obj, const index_t& index)
    -> decltype(std::forward<Indexable>(obj)[static_cast<std::size_t>(get(index))])
{
    DEBUG_ASSERT(detail::index_valid(detail::member_size{}, obj,
                                     static_cast<std::size_t>(get(index))),
                 detail::precondition_error_handler{});
    return std::forward<Indexable>(obj)[static_cast<std::size_t>(get(index))];
}

/// \effects Increments the [ts::index_t]() by the specified distance.
/// If the distance is negative, decrements the index instead.
/// \notes This is the same as `index += dist` and the equivalent of [std::advance()]().
/// \module types
inline void advance(index_t& index, const difference_t& dist)
{
    index += dist;
}

/// \returns The distance between two [ts::index_t]() objects,
/// i.e. how often you'd have to increment `a` to reach `b`.
/// \notes This is the same as `b - a` and the equivalent of [std::distance()]().
/// \module types
constexpr difference_t distance(const index_t& a, const index_t& b)
{
    return b - a;
}

/// \returns The [ts::index_t]() that is `dist` greater than `index`.
/// \notes This is the same as `index + dist` and the equivalent of [std::next()]().
/// \module types
constexpr index_t next(const index_t& index, const difference_t& dist = difference_t(1))
{
    return index + dist;
}

/// \returns The [ts::index_t]() that is `dist` smaller than `index`.
/// \notes This is the same as `index - dist` and the equivalent of [std::prev()]().
/// \module types
constexpr index_t prev(const index_t& index, const difference_t& dist = difference_t(1))
{
    return index - dist;
}
} // namespace type_safe

#endif // TYPE_SAFE_INDEX_HPP_INCLUDED
