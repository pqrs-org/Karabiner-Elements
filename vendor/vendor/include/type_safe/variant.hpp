// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_VARIANT_HPP_INCLUDED
#define TYPE_SAFE_VARIANT_HPP_INCLUDED

#include <type_safe/detail/variant_impl.hpp>
#include <type_safe/optional_ref.hpp>

namespace type_safe
{
/// Convenience alias for [ts::union_type]().
/// \module variant
template <typename T>
using variant_type = union_type<T>;

/// Convenience alias for [ts::union_types]().
/// \module variant
template <typename... Ts>
using variant_types = union_types<Ts...>;

/// Tag type to mark a [ts::basic_variant]() without a value.
/// \module variant
struct nullvar_t
{
    constexpr nullvar_t() {}
};

/// Tag object of type [ts::nullvar_t]().
/// \module variant
constexpr nullvar_t nullvar;

/// An improved `union` storing at most one of the given types at a time (or possibly none).
///
/// It is an improved version of [std::variant]().
/// A big problem with variant is implementing the operation that changes the type.
/// It has to destroy the old value and then create the new one.
/// But how to handle an exception when creating the new type?
/// There are multiple ways of handling this, so it is outsourced in a policy.
/// The variant policy is a class that must have the following members:
/// * `allow_empty` - either [std::true_type]() or [std::false_type]().
/// If it is "true", the variant can be put in the empty state explicitly.
/// * `void change_value(variant_type<T>, tagged_union<Types...>&, Args&&... args)` - changes the
/// value and type. It will be called when the variant already contains an object of a different
/// type. It must destroy the old type and create a new one with the given type and arguments.
/// \module variant
template <class VariantPolicy, typename HeadT, typename... TailT>
class basic_variant : detail::variant_copy<HeadT, TailT...>, detail::variant_move<HeadT, TailT...>
{
    using union_t = tagged_union<HeadT, TailT...>;
    using traits  = detail::traits<HeadT, TailT...>;

public:
    using types   = typename union_t::types;
    using type_id = typename union_t::type_id;

    using allow_empty = typename VariantPolicy::allow_empty;

    static constexpr type_id invalid_type = union_t::invalid_type;

    //=== constructors/destructors/assignment/swap ===//
    /// \effects Initializes the variant to the empty state.
    /// \notes This constructor only participates in overload resolution,
    /// if the policy allows an empty variant.
    /// \group default
    /// \param Dummy
    /// \exclude
    /// \param 1
    /// \exclude
    template <typename Dummy = void,
              typename = typename std::enable_if<VariantPolicy::allow_empty::value, Dummy>::type>
    basic_variant() noexcept
    {}

    /// \group default
    /// \param Dummy
    /// \exclude
    /// \param 1
    /// \exclude
    template <typename Dummy = void,
              typename = typename std::enable_if<VariantPolicy::allow_empty::value, Dummy>::type>
    basic_variant(nullvar_t) noexcept : basic_variant()
    {}

    /// Copy (1)/move (2) constructs a variant.
    /// \effects If the other variant is not empty, it will call
    /// [ts::copy](standardese://ts::copy_union/) (1) or [ts::move](standardese://ts::move_union/)
    /// (2). \throws Anything thrown by the copy (1)/move (2) constructor. \notes This constructor
    /// only participates in overload resolution, if all types are copy (1)/move (2) constructible./
    /// \notes The move constructor only moves the stored value,
    /// and does not make the other variant empty.
    /// \group copy_move_ctor
    basic_variant(const basic_variant&) = default;

    /// \group copy_move_ctor
    basic_variant(basic_variant&&)
        TYPE_SAFE_NOEXCEPT_DEFAULT(traits::nothrow_move_constructible::value)
        = default;

    /// Initializes it containing a new object of the given type.
    /// \effects Creates it by calling `T`s constructor with the perfectly forwarded arguments.
    /// \throws Anything thrown by `T`s constructor.
    /// \notes This constructor does not participate in overload resolution,
    /// unless `T` is a valid type for the variant and constructible from the arguments.
    /// \param 2
    /// \exclude
    template <typename T, typename... Args,
              typename = detail::enable_variant_type<union_t, T, Args&&...>>
    explicit basic_variant(variant_type<T> type, Args&&... args)
    {
        storage_.get_union().emplace(type, std::forward<Args>(args)...);
    }

    /// Initializes it with a copy of the given object.
    /// \effects Same as the type + argument constructor called with the decayed type of the
    /// argument and the object perfectly forwarded. \throws Anything thrown by `T`s copy/move
    /// constructor. \notes This constructor does not participate in overload resolution, unless `T`
    /// is a valid type for the variant and copy/move constructible. \param 1 \exclude
    template <typename T, typename = detail::enable_variant_type<union_t, T, T&&>>
    basic_variant(T&& obj)
    : basic_variant(variant_type<typename std::decay<T>::type>{}, std::forward<T>(obj))
    {}

    /// Initializes it from a [ts::tagged_union]().
    /// \effects Copies the currently stored type of the union
    /// into the variant by calling the copy (1)/move (2) constructor of the stored type.
    /// \throws Anything thrown by the selected copy (1)/move (2) constructor.
    /// \requires If the variant policy does not allow the empty state,
    /// the union must not be empty.
    /// \group ctor_union
    explicit basic_variant(const tagged_union<HeadT, TailT...>& u)
    {
        DEBUG_ASSERT(allow_empty::value || u.has_value(), detail::precondition_error_handler{});
        copy(storage_.get_union(), u);
    }

    /// \group ctor_union
    explicit basic_variant(tagged_union<HeadT, TailT...>&& u)
    {
        DEBUG_ASSERT(allow_empty::value || u.has_value(), detail::precondition_error_handler{});
        move(storage_.get_union(), std::move(u));
    }

    /// \effects Destroys the currently stored value,
    /// if there is any.
    ~basic_variant() noexcept = default;

    /// Copy (1)/move (2) assigns a variant.
    /// \effects If the other variant is empty,
    /// makes this one empty as well.
    /// Otherwise let the other variant contains an object of type `T`.
    /// If this variant contains the same type and there is a copy (1)/move (2) assignment operator
    /// available, assigns the object to this object. Else forwards to the variant policy's
    /// `change_value()` function. \throws Anything thrown by either the copy (1)/move (2)
    /// assignment operator or copy (1)/move (2) constructor. If the assignment operator throws, the
    /// variant will contain the partially assigned object. If the constructor throws, the state
    /// depends on the variant policy. \notes This function does not participate in overload
    /// resolution, unless all types are copy (1)/move (2) constructible. \group copy_move_assign
    basic_variant& operator=(const basic_variant&) = default;
    /// \group copy_move_assign
    basic_variant& operator=(basic_variant&&)
        TYPE_SAFE_NOEXCEPT_DEFAULT(traits::nothrow_move_assignable::value)
        = default;

    /// Alias for [*reset()]().
    /// \param Dummy
    /// \exclude
    /// \param 1
    /// \exclude
    template <typename Dummy = void,
              typename = typename std::enable_if<VariantPolicy::allow_empty::value, Dummy>::type>
    basic_variant& operator=(nullvar_t) noexcept
    {
        reset();
        return *this;
    }

    /// Same as the single argument `emplace()`.
    /// \effects Changes the value to a copy of `obj`.
    /// \throws Anything thrown by `T`s copy/move constructor.
    /// \notes This function does not participate in overload resolution,
    /// unless `T` is a valid type for the variant and copy/move constructible.
    /// \param 1
    /// \exclude
    template <typename T, typename = detail::enable_variant_type<union_t, T, T&&>>
    basic_variant& operator=(T&& obj)
    {
        emplace(variant_type<typename std::decay<T>::type>{}, std::forward<T>(obj));
        return *this;
    }

    /// Swaps two variants.
    /// \effects There are four cases:
    /// * Both variants are empty. Then the function has no effect.
    /// * Both variants contain the same type, `T`. Then it calls swap on the stored type.
    /// * Both variants contain a type, but different types.
    /// Then it swaps the variant by move constructing the objects from one type to the other,
    /// using the variant policy.
    /// * Only one variant contains an object. Then it moves the value to the empty variant,
    /// and destroys it in the non-empty variant.
    ///
    /// \effects In either case, it will only call the swap() function or the move constructor.
    /// \throws Anything thrown by the swap function,
    /// in which case both variants contain the partially swapped values,
    /// or the mvoe constructor, in which case the exact behavior depends on the variant policy.
    friend void swap(basic_variant& a, basic_variant& b) noexcept(traits::nothrow_swappable::value)
    {
        auto& a_union = a.storage_.get_union();
        auto& b_union = b.storage_.get_union();

        if (a_union.has_value() && b_union.has_value())
            detail::swap_union<VariantPolicy, union_t>::swap(a_union, b_union);
        else if (a_union.has_value() && !b_union.has_value())
        {
            b = std::move(a);
            a.reset();
        }
        else if (!a_union.has_value() && b_union.has_value())
        {
            a = std::move(b);
            b.reset();
        }
    }

    //=== modifiers ===//
    /// \effects Destroys the stored value in the variant, if any.
    /// \notes This function only participate in overload resolution,
    /// if the variant policy allows the empty state.
    /// \param Dummy
    /// \exclude
    /// \param 1
    /// \exclude
    template <typename Dummy = void,
              typename = typename std::enable_if<VariantPolicy::allow_empty::value, Dummy>::type>
    void reset() noexcept
    {
        destroy(storage_.get_union());
    }

    /// Changes the value to a new object of the given type.
    /// \effects If the variant contains an object of the same type,
    /// assigns the argument to it.
    /// Otherwise behaves as the other emplace version.
    /// \throws Anything thrown by the chosen assignment operator
    /// or the other `emplace()`.
    /// If the assignment operator throws,
    /// the variant contains a partially assigned object.
    /// Otherwise it depends on the variant policy.
    /// \notes This function does not participate in overload resolution,
    /// unless `T` is a valid type for the variant and assignable from the argument
    /// without creating an additional temporary.
    /// \param 2
    /// \exclude
    /// \param 3
    /// \exclude
    template <typename T, typename Arg,
              typename
              = typename std::enable_if<detail::is_direct_assignable<T, Arg&&>::value>::type,
              typename = detail::enable_variant_type<union_t, T, Arg&&>>
    void emplace(variant_type<T> type, Arg&& arg)
    {
        if (storage_.get_union().type() == typename union_t::type_id(type))
            storage_.get_union().value(type) = std::forward<Arg>(arg);
        else
            emplace_impl(type, std::forward<Arg>(arg));
    }

    /// Changes the value to a new object of given type.
    /// \effects If variant is empty, creates the object directly in place
    /// by perfectly forwarding the arguments.
    /// Otherwise it forwards to the variant policy's `change_value()` function.
    /// \throws Anything thrown by `T`s constructor or possibly move constructor.
    /// If the variant was empty before, it is still empty afterwards.
    /// Otherwise the state depends on the policy.
    /// \notes This function does not participate in overload resolution,
    /// unless `T` is a valid type for the variant and constructible from the arguments.
    /// \param 2
    /// \exclude
    template <typename T, typename... Args,
              typename = detail::enable_variant_type<union_t, T, Args&&...>>
    void emplace(variant_type<T> type, Args&&... args)
    {
        emplace_impl(type, std::forward<Args>(args)...);
    }

private:
    template <typename T, typename... Args>
    void emplace_impl(variant_type<T> type, Args&&... args)
    {
        if (storage_.get_union().has_value())
            VariantPolicy::change_value(type, storage_.get_union(), std::forward<Args>(args)...);
        else
            storage_.get_union().emplace(type, std::forward<Args>(args)...);
    }

    template <typename T>
    using enable_valid = typename std::enable_if<(type_id::template is_valid<T>)()>::type;

public:
    //=== observers ===//
    /// \returns The type id representing the type of the value currently stored in the variant.
    /// \notes If it does not have a value stored, returns [*invalid_type]().
    type_id type() const noexcept
    {
        return storage_.get_union().type();
    }

    /// \returns `true` if the variant currently contains a value,
    /// `false` otherwise.
    /// \notes Depending on the variant policy,
    /// it can be guaranteed to return `true` all the time.
    /// \group has_value
    bool has_value() const noexcept
    {
        return storage_.get_union().has_value();
    }

    /// \group has_value
    explicit operator bool() const noexcept
    {
        return has_value();
    }

    /// \group has_value
    bool has_value(variant_type<nullvar_t>) const noexcept
    {
        return !has_value();
    }

    /// \returns `true` if the variant currently stores an object of type `T`,
    /// `false` otherwise.
    /// \notes `T` must not necessarily be a type that can be stored in the variant.
    template <typename T>
    bool has_value(variant_type<T> type) const noexcept
    {
        return this->type() == type_id(type);
    }

    /// \returns A copy of [ts::nullvar]().
    /// \requires The variant must be empty.
    nullvar_t value(variant_type<nullvar_t>) const noexcept
    {
        DEBUG_ASSERT(!has_value(), detail::precondition_error_handler{});
        return nullvar;
    }

    /// \returns A (`const`) lvalue (1, 2)/rvalue (3, 4) reference to the stored object of the given
    /// type. \requires The variant must currently store an object of the given type, i.e.
    /// `has_value(type)` must return `true`. \group value \param 1 \exclude
    template <typename T, typename = enable_valid<T>>
    T& value(variant_type<T> type) TYPE_SAFE_LVALUE_REF noexcept
    {
        return storage_.get_union().value(type);
    }

    /// \group value
    /// \param 1
    /// \exclude
    template <typename T, typename = enable_valid<T>>
    const T& value(variant_type<T> type) const TYPE_SAFE_LVALUE_REF noexcept
    {
        return storage_.get_union().value(type);
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group value
    /// \param 1
    /// \exclude
    template <typename T, typename = enable_valid<T>>
        T&& value(variant_type<T> type) && noexcept
    {
        return std::move(storage_.get_union()).value(type);
    }

    /// \group value
    /// \param 1
    /// \exclude
    template <typename T, typename = enable_valid<T>>
    const T&& value(variant_type<T> type) const&& noexcept
    {
        return std::move(storage_.get_union()).value(type);
    }
#endif

    /// \returns A [ts::optional_ref]() to [ts::nullvar]().
    /// If the variant is not empty, returns a null reference.
    optional_ref<const nullvar_t> optional_value(variant_type<nullvar_t>) const noexcept
    {
        return has_value() ? nullptr : type_safe::opt_ref(&nullvar);
    }

    /// \returns A (`const`) [ts::optional_ref]() (1, 2)/[ts::optional_xvalue_ref]() to the stored
    /// value of given type. If it stores a different type, returns a null reference. \group
    /// optional_value
    template <typename T>
    optional_ref<T> optional_value(variant_type<T> type) TYPE_SAFE_LVALUE_REF noexcept
    {
        return has_value(type) ? type_safe::opt_ref(&storage_.get_union().value(type)) : nullptr;
    }

    /// \group optional_value
    template <typename T>
    optional_ref<const T> optional_value(variant_type<T> type) const TYPE_SAFE_LVALUE_REF noexcept
    {
        return has_value(type) ? type_safe::opt_ref(&storage_.get_union().value(type)) : nullptr;
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group optional_value
    template <typename T>
        optional_xvalue_ref<T> optional_value(variant_type<T> type) && noexcept
    {
        return has_value(type) ? type_safe::opt_xref(&storage_.get_union().value(type)) : nullptr;
    }

    /// \group optional_value
    template <typename T>
    optional_xvalue_ref<const T> optional_value(variant_type<T> type) const&& noexcept
    {
        return has_value(type) ? type_safe::opt_xref(&storage_.get_union().value(type)) : nullptr;
    }
#endif

    /// \returns If the variant currently stores an object of type `T`,
    /// returns a copy of that by copy (1)/move (2) constructing.
    /// Otherwise returns `other` converted to `T`.
    /// \throws Anything thrown by `T`s copy (1)/move (2) constructor or the converting constructor.
    /// \notes `T` must not necessarily be a type that can be stored in the variant./
    /// \notes This function does not participate in overload resolution,
    /// unless `T` is copy (1)/move (2) constructible and the fallback convertible to `T`.
    /// \group value_or
    /// \param 2
    /// \exclude
    template <typename T, typename U>
    T value_or(
        variant_type<T> type, U&& other,
        typename std::enable_if<
            std::is_copy_constructible<T>::value && std::is_convertible<U&&, T>::value, int>::type
        = 0) const TYPE_SAFE_LVALUE_REF
    {
        return has_value(type) ? value(type) : static_cast<T>(std::forward<U>(other));
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group value_or
    /// \param 2
    /// \exclude
    template <typename T, typename U>
    T value_or(
        variant_type<T> type, U&& other,
        typename std::enable_if<
            std::is_move_constructible<T>::value && std::is_convertible<U&&, T>::value, int>::type
        = 0) &&
    {
        return has_value(type) ? std::move(value(type)) : static_cast<T>(std::forward<U>(other));
    }
#endif

    /// Maps a variant with a function.
    /// \effects If the variant is not empty,
    /// calls the function using either `std::forward<Functor>(f)(current-value,
    /// std::forward<Args>(args)...)` or member call syntax
    /// `(current-value.*std::forward<Functor>(f))(std::forward<Args>(args)...)`. If those two
    /// expressions are both ill-formed, does nothing. \returns A new variant of the same type. It
    /// contains nothing, if `*this` contains nothing. Otherwise, if the function was called, it
    /// contains the result of the function. Otherwise, it is a copy of the current variant. \throws
    /// Anything thrown by the function or copy/move constructor, in which case the variant will be
    /// left unchanged, unless the object was already moved into the function and modified there.
    /// \requires The result of the function - if it is called - can be stored in the variant.
    /// \notes (1) will use the copy constructor, (2) will use the move constructor.
    /// The function does not participate in overload resolution,
    /// if copy (1)/move (2) constructors are not available for all types.
    /// \group map
    /// \param 1
    /// \exclude
    /// \param 2
    /// \exclude
    template <typename Functor, typename... Args, typename Dummy = void,
              typename = typename std::enable_if<traits::copy_constructible::value, Dummy>::type>
    basic_variant map(Functor&& f, Args&&... args) const TYPE_SAFE_LVALUE_REF
    {
        basic_variant result(force_empty{});
        if (!has_value())
            return result;
        detail::map_union<Functor&&, union_t>::map(result.storage_.get_union(),
                                                   storage_.get_union(), std::forward<Functor>(f),
                                                   std::forward<Args>(args)...);
        DEBUG_ASSERT(result.has_value(), detail::assert_handler{});
        return result;
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group map
    /// \param 1
    /// \exclude
    /// \param 2
    /// \exclude
    template <typename Functor, typename... Args, typename Dummy = void,
              typename = typename std::enable_if<traits::move_constructible::value, Dummy>::type>
    basic_variant map(Functor&& f, Args&&... args) &&
    {
        basic_variant result(force_empty{});
        if (!has_value())
            return result;
        detail::map_union<Functor&&, union_t>::map(result.storage_.get_union(),
                                                   std::move(storage_.get_union()),
                                                   std::forward<Functor>(f),
                                                   std::forward<Args>(args)...);
        DEBUG_ASSERT(result.has_value(), detail::assert_handler{});
        return result;
    }
#endif

private:
    struct force_empty
    {};

    basic_variant(force_empty) noexcept {}

    detail::variant_storage<VariantPolicy, HeadT, TailT...> storage_;

    friend detail::storage_access;
};

/// \exclude
template <class VariantPolicy, typename Head, typename... Types>
constexpr typename basic_variant<VariantPolicy, Head, Types...>::type_id
    basic_variant<VariantPolicy, Head, Types...>::invalid_type;

//=== comparison ===//
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op, Expr, Expr2)                                                  \
    template <class VariantPolicy, typename Head, typename... Types>                               \
    bool operator Op(const basic_variant<VariantPolicy, Head, Types...>& lhs, nullvar_t)           \
    {                                                                                              \
        return (void)lhs, Expr;                                                                    \
    }                                                                                              \
    /** \group variant_comp_null */                                                                \
    template <class VariantPolicy, typename Head, typename... Types>                               \
    bool operator Op(nullvar_t, const basic_variant<VariantPolicy, Head, Types...>& rhs)           \
    {                                                                                              \
        return (void)rhs, Expr2;                                                                   \
    }

/// Compares a [ts::basic_variant]() with [ts::nullvar]().
///
/// A variant compares equal to `nullvar`, when it does not have a value.
/// A variant compares never less to `nullvar`, `nullvar` compares less only if the variant has a
/// value. The other comparisons behave accordingly. \group variant_comp_null \module variant
TYPE_SAFE_DETAIL_MAKE_OP(==, !lhs.has_value(), !rhs.has_value())
/// \group variant_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(!=, lhs.has_value(), rhs.has_value())
/// \group variant_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(<, false, rhs.has_value())
/// \group variant_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(<=, !lhs.has_value(), true)
/// \group variant_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(>, lhs.has_value(), false)
/// \group variant_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(>=, true, !rhs.has_value())

#undef TYPE_SAFE_DETAIL_MAKE_OP

/// Compares a [ts::basic_variant]() with a value.
///
/// A variant compares equal to a value, if it contains an object of the same type and the object
/// compares equal. A variant compares less to a value, if - when it has a different type - the type
/// id compares less than the type id of the value, or - when it has the same type - the object
/// compares less to the value. The other comparisons behave accordingly. \notes The value must not
/// necessarily have a type that can be stored in the variant. \group variant_comp_t \module variant
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator==(const basic_variant<VariantPolicy, Head, Types...>& lhs, const T& rhs)
{
    return lhs.has_value(variant_type<T>{}) && lhs.value(variant_type<T>{}) == rhs;
}
/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator==(const T& lhs, const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return rhs == lhs;
}

/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator!=(const basic_variant<VariantPolicy, Head, Types...>& lhs, const T& rhs)
{
    return !(lhs == rhs);
}
/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator!=(const T& lhs, const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return !(rhs == lhs);
}

/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator<(const basic_variant<VariantPolicy, Head, Types...>& lhs, const T& rhs)
{
    constexpr auto id =
        typename basic_variant<VariantPolicy, Head, Types...>::type_id(variant_type<T>{});
    if (lhs.type() != id)
        return lhs.type() < id;
    return lhs.value(variant_type<T>{}) < rhs;
}
/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator<(const T& lhs, const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    constexpr auto id =
        typename basic_variant<VariantPolicy, Head, Types...>::type_id(variant_type<T>{});
    if (id != rhs.type())
        return id < rhs.type();
    return lhs < rhs.value(variant_type<T>{});
}

/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator<=(const basic_variant<VariantPolicy, Head, Types...>& lhs, const T& rhs)
{
    return !(rhs < lhs);
}
/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator<=(const T& lhs, const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return !(rhs < lhs);
}

/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator>(const basic_variant<VariantPolicy, Head, Types...>& lhs, const T& rhs)
{
    return rhs < lhs;
}
/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator>(const T& lhs, const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return rhs < lhs;
}

/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator>=(const basic_variant<VariantPolicy, Head, Types...>& lhs, const T& rhs)
{
    return !(lhs < rhs);
}
/// \group variant_comp_t
template <class VariantPolicy, typename Head, typename... Types, typename T>
bool operator>=(const T& lhs, const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return !(lhs < rhs);
}

/// Compares two [ts::basic_variant]()s.
///
/// They compare equal if both store the same type (or none) and the stored object compares equal.
/// A variant is less than another if they store mismatched types and the type id of the first is
/// less than the other, or if they store the same type and the stored object compares less. The
/// other comparisons behave accordingly. \module variant \group variant_comp
template <class VariantPolicy, typename Head, typename... Types>
bool operator==(const basic_variant<VariantPolicy, Head, Types...>& lhs,
                const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return detail::compare_variant<
        basic_variant<VariantPolicy, Head, Types...>>::compare_equal(lhs, rhs);
}

/// \group variant_comp
template <class VariantPolicy, typename Head, typename... Types>
bool operator!=(const basic_variant<VariantPolicy, Head, Types...>& lhs,
                const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return !(lhs == rhs);
}

/// \group variant_comp
template <class VariantPolicy, typename Head, typename... Types>
bool operator<(const basic_variant<VariantPolicy, Head, Types...>& lhs,
               const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return detail::compare_variant<basic_variant<VariantPolicy, Head, Types...>>::compare_less(lhs,
                                                                                               rhs);
}

/// \group variant_comp
template <class VariantPolicy, typename Head, typename... Types>
bool operator<=(const basic_variant<VariantPolicy, Head, Types...>& lhs,
                const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return !(rhs < lhs);
}

/// \group variant_comp
template <class VariantPolicy, typename Head, typename... Types>
bool operator>(const basic_variant<VariantPolicy, Head, Types...>& lhs,
               const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return rhs < lhs;
}

/// \group variant_comp
template <class VariantPolicy, typename Head, typename... Types>
bool operator>=(const basic_variant<VariantPolicy, Head, Types...>& lhs,
                const basic_variant<VariantPolicy, Head, Types...>& rhs)
{
    return !(lhs < rhs);
}

/// \effects If the variant is empty, does nothing.
/// Otherwise let the variant contain an object of type `T`.
/// If the functor is callable for the `T`, calls its `operator()` passing it the stored object.
/// Else does nothing.
/// \module variant
/// \group variant_with
template <class VariantPolicy, typename Head, typename... Types, typename Func, typename... Args>
void with(basic_variant<VariantPolicy, Head, Types...>& variant, Func&& func,
          Args&&... additional_args)
{
    with(detail::storage_access::get(variant).get_union(), std::forward<Func>(func),
         std::forward<Args>(additional_args)...);
}

/// \group variant_with
template <class VariantPolicy, typename Head, typename... Types, typename Func, typename... Args>
void with(const basic_variant<VariantPolicy, Head, Types...>& variant, Func&& func,
          Args&&... additional_args)
{
    with(detail::storage_access::get(variant).get_union(), std::forward<Func>(func),
         std::forward<Args>(additional_args)...);
}

/// \group variant_with
template <class VariantPolicy, typename Head, typename... Types, typename Func, typename... Args>
void with(basic_variant<VariantPolicy, Head, Types...>&& variant, Func&& func,
          Args&&... additional_args)
{
    with(std::move(detail::storage_access::get(variant).get_union()), std::forward<Func>(func),
         std::forward<Args>(additional_args)...);
}

/// \group variant_with
template <class VariantPolicy, typename Head, typename... Types, typename Func, typename... Args>
void with(const basic_variant<VariantPolicy, Head, Types...>&& variant, Func&& func,
          Args&&... additional_args)
{
    with(std::move(detail::storage_access::get(variant).get_union()), std::forward<Func>(func),
         std::forward<Args>(additional_args)...);
}

/// A variant policy for [ts::basic_variant]() that uses a fallback type.
///
/// When changing the type of the variant throws an exception,
/// the variant will create an object of the fallback type instead.
/// The variant will never be empty.
/// \requires `Fallback` must be nothrow default constructible
/// and a type that can be stored in the variant.
/// \module variant
template <typename Fallback>
class fallback_variant_policy
{
    static_assert(std::is_nothrow_default_constructible<Fallback>::value,
                  "fallback must be nothrow default constructible");

public:
    using allow_empty = std::false_type;

    template <typename T, typename... Types, typename... Args>
    static void change_value(union_type<T> type, tagged_union<Types...>& storage, Args&&... args)
    {
        change_value_impl(type, storage, std::forward<Args>(args)...);
    }

private:
    template <typename T, typename... Types, typename... Args>
    static auto change_value_impl(union_type<T> type, tagged_union<Types...>& storage,
                                  Args&&... args) noexcept ->
        typename std::enable_if<std::is_nothrow_constructible<T, Args&&...>::value>::type
    {
        destroy(storage);
        // won't throw
        storage.emplace(type, std::forward<Args>(args)...);
    }

    template <typename T, typename... Types, typename... Args>
    static auto change_value_impl(union_type<T> type, tagged_union<Types...>& storage,
                                  Args&&... args) ->
        typename std::enable_if<!std::is_nothrow_constructible<T, Args&&...>::value>::type
    {
        destroy(storage);
        TYPE_SAFE_TRY
        {
            // might throw
            storage.emplace(type, std::forward<Args>(args)...);
        }
        TYPE_SAFE_CATCH_ALL
        {
            // won't throw
            storage.emplace(union_type<Fallback>{});
            TYPE_SAFE_RETHROW;
        }
    }
};

/// A [ts::basic_variant]() using the [ts::fallback_variant_policy]().
///
/// This is a variant that is never empty, where exceptions on changing the type
/// leaves it with a default-constructed object of the `Fallback` type.
/// \requires `Fallback` must be nothrow default constructible.
/// \module variant
template <typename Fallback, typename... OtherTypes>
using fallback_variant = basic_variant<fallback_variant_policy<Fallback>, Fallback, OtherTypes...>;

/// A variant policy for [ts::basic_variant]() that creates a variant with explicit empty state.
///
/// It allows an empty variant explicitly.
/// When changing the type of the variant throws an exception,
/// the variant will be left in that empty state.
/// \module variant
class optional_variant_policy
{
public:
    using allow_empty = std::true_type;

    template <typename T, typename... Types, typename... Args>
    static void change_value(union_type<T> type, tagged_union<Types...>& storage, Args&&... args)
    {
        destroy(storage);
        storage.emplace(type, std::forward<Args>(args)...);
    }
};

/// \exclude
namespace detail
{
    template <bool ForceNonEmpty>
    class non_empty_variant_policy
    {
    public:
        using allow_empty = std::false_type;

        template <typename T, typename... Types, typename... Args>
        static void change_value(union_type<T> type, tagged_union<Types...>& storage,
                                 Args&&... args)
        {
            change_value_impl(type, storage, std::forward<Args>(args)...);
        }

    private:
        template <typename T, typename... Types>
        static void move_emplace(union_type<T> type, tagged_union<Types...>& storage,
                                 T&& obj) noexcept(ForceNonEmpty)
        {
            // if this throws, there's nothing we can do
            storage.emplace(type, std::move(obj));
        }

        template <typename T, typename... Types>
        static void change_value_impl(union_type<T> type, tagged_union<Types...>& storage, T&& obj)
        {
            destroy(storage);
            move_emplace(type, storage, std::move(obj)); // throw handled
        }

        template <typename T, typename... Types, typename... Args>
        static auto change_value_impl(union_type<T> type, tagged_union<Types...>& storage,
                                      Args&&... args) ->
            typename std::enable_if<std::is_nothrow_constructible<T, Args&&...>::value>::type
        {
            destroy(storage);
            // won't throw
            storage.emplace(type, std::forward<Args>(args)...);
        }

        template <typename T, typename... Types, typename... Args>
        static auto change_value_impl(union_type<T> type, tagged_union<Types...>& storage,
                                      Args&&... args) ->
            typename std::enable_if<!std::is_nothrow_constructible<T, Args&&...>::value>::type
        {
            T tmp(std::forward<Args>(args)...); // might throw
            destroy(storage);
            move_emplace(type, storage, std::move(tmp)); // throw handled
        }
    };
} // namespace detail

/// A variant policy for [ts::basic_variant]() that creates a variant which is rarely empty.
///
/// When changing the type of the variant, it will use a the move constructor with a temporary.
/// If the move constructor throws, the variant will be left in the empty state.
/// Putting it into the empty state explicitly is not allowed.
/// \module variant
using rarely_empty_variant_policy = detail::non_empty_variant_policy<false>;

/// A variant policy for [ts::basic_variant]() that creates a variant which is never empty.
///
/// Similar to [ts::rarely_empty_variant_policy]() but when the move constructor throws,
/// it calls [std::terminate()]().
/// \module variant
using never_empty_variant_policy = detail::non_empty_variant_policy<true>;

/// \exclude
namespace detail
{
    template <typename... Types>
    struct select_variant_policy
    {
        using type = basic_variant<rarely_empty_variant_policy, Types...>;
    };

    template <typename... Types>
    struct select_variant_policy<nullvar_t, Types...>
    {
        using type = basic_variant<optional_variant_policy, Types...>;
    };
} // namespace detail

/// A [ts::basic_variant]() with the recommended default semantics.
///
/// If the first type is [ts::nullvar_t]() it will use the [ts::optional_variant_policy](),
/// which explicitly allows the empty state.
/// Otherwise it will use the [ts::rarely_empty_variant_policy]()
/// where it tries to avoid the empty state as good as possible.
/// \notes If you pass [ts::nullvar_t]() as the first type,
/// it is not actually one of the types that can be stored in the variant,
/// but a tag to enable the empty state.
/// \module variant
template <typename... Types>
using variant = typename detail::select_variant_policy<Types...>::type;
} // namespace type_safe

#endif // TYPE_SAFE_VARIANT_HPP_INCLUDED
