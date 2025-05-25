// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_OPTIONAL_HPP_INCLUDED
#define TYPE_SAFE_OPTIONAL_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#include <functional>
#include <new>
#include <type_traits>
#endif

#include <type_safe/detail/assert.hpp>
#include <type_safe/detail/assign_or_construct.hpp>
#include <type_safe/detail/copy_move_control.hpp>
#include <type_safe/detail/is_nothrow_swappable.hpp>
#include <type_safe/detail/map_invoke.hpp>

namespace type_safe
{
template <class StoragePolicy>
class basic_optional;

/// \exclude
namespace detail
{
    template <class StoragePolicy>
    struct optional_storage
    {
        StoragePolicy storage;

        optional_storage() noexcept = default;

        optional_storage(const optional_storage& other)
        {
            storage.create_value(other.storage);
        }

        optional_storage(optional_storage&& other) noexcept(
            std::is_nothrow_move_constructible<typename StoragePolicy::value_type>::value)
        {
            storage.create_value(std::move(other.storage));
        }

        ~optional_storage() noexcept
        {
            if (storage.has_value())
                storage.destroy_value();
        }

        optional_storage& operator=(const optional_storage& other)
        {
            storage.copy_value(other.storage);
            return *this;
        }

        optional_storage& operator=(optional_storage&& other) noexcept(
            std::is_nothrow_move_constructible<typename StoragePolicy::value_type>::value
            && (!std::is_move_assignable<typename StoragePolicy::value_type>::value
                || std::is_nothrow_move_assignable<typename StoragePolicy::value_type>::value))
        {
            storage.copy_value(std::move(other.storage));
            return *this;
        }
    };

    template <typename T>
    using optional_copy = copy_control<std::is_copy_constructible<T>::value>;

    template <typename T>
    using optional_move = move_control<std::is_move_constructible<T>::value>;

    //=== is_optional ===//
    template <typename T>
    struct is_optional_impl : std::false_type
    {};

    template <class StoragePolicy>
    struct is_optional_impl<basic_optional<StoragePolicy>> : std::true_type
    {};

    template <typename T>
    using is_optional = is_optional_impl<typename std::decay<T>::type>;
} // namespace detail

template <class StoragePolicy>
class basic_optional;

//=== basic_optional ===//
/// Tag type to mark a [ts::basic_optional]() without a value.
/// \module optional
/// \output_section Basic optional
struct nullopt_t
{
    constexpr nullopt_t() {}
};

/// Tag object of type [ts::nullopt_t]().
/// \module optional
constexpr nullopt_t nullopt;

/// Selects the storage policy used when rebinding a [ts::basic_optional]().
///
/// Some operations like [ts::basic_optional::map()]() change the type of an optional.
/// This traits controls which `StoragePolicy` is going to be used for the new optional.
/// You can for example requests a [ts::compact_optional_storage]() for your type,
/// simply specialize it and set a `type` typedef.
/// \module optional
template <typename T>
struct optional_storage_policy_for
{
    using type = void;
};

/// Specialization of [ts::optional_storage_policy_for]() for [ts::basic_optional]() itself.
///
/// It will simply forward to the same policy, so `ts::optional_for<ts::optional<T>>` is simply
/// `ts::optional<T>`, not `ts::optional<ts::optional<T>>`. \module optional
template <class StoragePolicy>
struct optional_storage_policy_for<basic_optional<StoragePolicy>>
{
    using type = StoragePolicy;
};

template <typename T, bool XValue = false>
class reference_optional_storage;

/// Specialization of [ts::optional_storage_policy_for]() for lvalue references.
///
/// It will use [ts::reference_optional_storage]() as policy.
/// \module optional
template <typename T>
struct optional_storage_policy_for<T&>
{
    using type = reference_optional_storage<T>;
};

/// Specialization of [ts::optional_storage_policy_for]() for rvalue references.
///
/// They are not supported.
/// \module optional
template <typename T>
struct optional_storage_policy_for<T&&>
{
    static_assert(sizeof(T) != sizeof(T), "no optional for rvalue references supported");
};

/// \exclude
namespace detail
{
    template <typename TraitsResult, typename Fallback>
    using select_optional_storage_policy =
        typename std::conditional<std::is_same<TraitsResult, void>::value, Fallback,
                                  TraitsResult>::type;

    template <typename T, typename Fallback>
    using rebind_optional = typename std::conditional<
        std::is_void<T>::value, void,
        basic_optional<select_optional_storage_policy<typename optional_storage_policy_for<T>::type,
                                                      Fallback>>>::type;
} // namespace detail

/// An optional type, i.e. a type that may or may not be there.
///
/// It is similar to [std::optional<T>]() but lacks some functions and provides some others.
/// It can be in one of two states: it contains a value of a certain type or it does not (it is
/// "empty").
///
/// The storage itself is managed via the `StoragePolicy`.
/// It must provide the following members:
/// * Typedef `value_type` - the type stored in the optional
/// * Typedef `(const_)lvalue_reference` - `const` lvalue reference type
/// * Typedef `(const_)rvalue_reference` - `const` rvalue reference type
/// * Template alias `rebind<U>` - the same policy for a different type
/// * `StoragePolicy() noexcept` - a no-throw default constructor that initializes it in the "empty"
/// state
/// * `void create_value(Args&&... args)` - creates a value by forwarding the arguments to its
/// constructor
/// * `void create_value_explicit(T&& obj)` - creates a value requiring an `explicit` constructor
/// * `void create_value(const StoragePolicy&/StoragePolicy&&)` - creates a value by using the value
/// stored in the other policy
/// * `void copy_value(const StoragePolicy&/StoragePolicy&&)` - similar to above, but *this may
/// contain a value already
/// * `void swap_value(StoragePolicy&)` - swaps the stored value (if any) with the one in the other
/// policy
/// * `void destroy_value() noexcept` - calls the destructor of the value, afterwards the storage is
/// "empty"
/// * `bool has_value() const noexcept` - returns whether or not there is a value, i.e.
/// `create_value()` has been called but `destroy_value()` has not
/// * `U get_value() (const)& noexcept` - returns a reference to the stored value, U is one of the
/// `XXX_reference` typedefs
/// * `U get_value() (const)&& noexcept` - returns a reference to the stored value, U is one of the
/// `XXX_reference` typedefs
/// * `U get_value_or(T&& val) [const&/&&]` - returns either `get_value()` or `val`
/// \module optional
template <class StoragePolicy>
class basic_optional : detail::optional_storage<StoragePolicy>,
                       detail::optional_copy<typename StoragePolicy::value_type>,
                       detail::optional_move<typename StoragePolicy::value_type>
{
public:
    using storage    = StoragePolicy;
    using value_type = typename storage::value_type;

    /// Rebinds the current optional to the type `U`.
    ///
    /// It will use [ts::optional_storage_policy_for]() to determine whether a change of storage
    /// policy is needed. \notes If `U` is `void`, the result will be `void` as well. \notes Due to
    /// a specialization of [ts::optional_storage_policy_for](), if `U` is an optional itself, the
    /// result will be `U`, not an optional of an optional. \exclude target
    template <typename U>
    using rebind = detail::rebind_optional<U, typename StoragePolicy::template rebind<U>>;

private:
    storage& get_storage() TYPE_SAFE_LVALUE_REF noexcept
    {
        return static_cast<detail::optional_storage<StoragePolicy>&>(*this).storage;
    }

    const storage& get_storage() const TYPE_SAFE_LVALUE_REF noexcept
    {
        return static_cast<const detail::optional_storage<StoragePolicy>&>(*this).storage;
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    storage&& get_storage() && noexcept
    {
        return std::move(static_cast<detail::optional_storage<StoragePolicy>&>(*this).storage);
    }

    const storage&& get_storage() const&& noexcept
    {
        return std::move(
            static_cast<const detail::optional_storage<StoragePolicy>&>(*this).storage);
    }
#endif

public:
    //=== constructors/destructors/assignment/swap ===//
    /// \effects Creates it without a value.
    /// \group empty
    basic_optional() noexcept = default;

    /// \group empty
    basic_optional(nullopt_t) noexcept {}

    /// \effects Creates it with a value by forwarding `value`.
    /// \throws Anything thrown by the constructor of `value_type`.
    /// \requires The `create_value()` function of the `StoragePolicy` must accept `value`.
    /// \param 1
    /// \exclude
    template <typename T, typename = typename std::enable_if<!std::is_same<
                              typename std::decay<T>::type, basic_optional<storage>>::value>::type>
    basic_optional(T&& value,
                   decltype(std::declval<storage>().create_value(std::declval<T>()), 0) = 0)
    {
        get_storage().create_value(std::forward<T>(value));
    }

    /// \effects Creates it with a value by forwarding `value`.
    /// \throws Anything thrown by the constructor of `value_type`.
    /// \requires The `create_value_explicit()` function of the `StoragePolicy` must accept `value`.
    /// \param 1
    /// \exclude
    template <
        typename T,
        typename std::enable_if<
            !std::is_same<typename std::decay<T>::type, basic_optional<storage>>::value, int>::type
        = 0>
    explicit basic_optional(
        T&& value,
        decltype(std::declval<storage>().create_value_explicit(std::declval<T>()), 0) = 0)
    {
        get_storage().create_value_explicit(std::forward<T>(value));
    }

    /// Copy constructor.
    /// \effects If `other` does not have a value, it will be created without a value as well.
    /// If `other` has a value, it will be created with a value by copying `other.value()`.
    /// \throws Anything thrown by the copy constructor of `value_type` if `other` has a value.
    /// \notes This constructor will not participate in overload resolution,
    /// unless the `value_type` is copy constructible.
    basic_optional(const basic_optional& other) = default;

    /// Move constructor.
    /// \effects If `other` does not have a value, it will be created without a value as well.
    /// If `other` has a value, it will be created with a value by moving `other.value()`.
    /// \throws Anything thrown by the move constructor of `value_type` if `other` has a value.
    /// \notes `other` will still have a value after the move operation,
    /// it is just in a moved-from state./
    /// \notes This constructor will not participate in overload resolution,
    /// unless the `value_type` is move constructible.
    basic_optional(basic_optional&& other)
        TYPE_SAFE_NOEXCEPT_DEFAULT(std::is_nothrow_move_constructible<value_type>::value)
        = default;

    /// Destructor.
    /// \effects If it has a value, it will be destroyed.
    ~basic_optional() noexcept = default;

    /// \effects Same as `reset()`.
    basic_optional& operator=(nullopt_t) noexcept
    {
        reset();
        return *this;
    }

    /// \effects Same as `emplace(std::forward<T>(t))`.
    /// \requires The call to `emplace()` must be well-formed.
    /// \param 1
    /// \exclude
    /// \synopsis_return basic_optional&
    template <typename T, typename = typename std::enable_if<!std::is_same<
                              typename std::decay<T>::type, basic_optional<storage>>::value>::type>
    auto operator=(T&& value) -> decltype(
        std::declval<basic_optional<storage>>().get_storage().create_value(std::forward<T>(value)),
        *this)
    {
        emplace(std::forward<T>(value));
        return *this;
    }

    /// Copy assignment operator.
    /// \effects If `other` has a value, does something equivalent to `emplace(other.value())` (this
    /// will always trigger the single parameter version). Otherwise will reset the optional to the
    /// empty state. \throws Anything thrown by the call to `emplace()`. \notes This operator will
    /// not participate in overload resolution, unless the `value_type` is copy constructible.
    basic_optional& operator=(const basic_optional& other) = default;

    /// Move assignment operator.
    /// \effects If `other` has a value, does something equivalent to
    /// `emplace(std::move(other).value())` (this will always trigger the single parameter version).
    /// Otherwise will reset the optional to the empty state.
    /// \throws Anything thrown by the call to `emplace()`.
    /// \notes This operator will not participate in overload resolution,
    /// unless the `value_type` is copy constructible.
    basic_optional& operator=(basic_optional&& other)
        TYPE_SAFE_NOEXCEPT_DEFAULT(std::is_nothrow_move_constructible<value_type>::value
                                   && (!std::is_move_assignable<value_type>::value
                                       || std::is_nothrow_move_assignable<value_type>::value))
        = default;

    /// \effects Swap.
    /// If both `a` and `b` have values, swaps the values with their swap function.
    /// Otherwise, if only one of them have a value, moves that value to the other one and makes the
    /// moved-from empty. Otherwise, if both are empty, does nothing. \throws Anything thrown by the
    /// move construction or swap.
    friend void swap(basic_optional& a, basic_optional& b) noexcept(
        std::is_nothrow_move_constructible<value_type>::value&&
            detail::is_nothrow_swappable<value_type>::value)
    {
        a.get_storage().swap_value(b.get_storage());
    }

    //=== modifiers ===//
    /// \effects Destroys the value by calling its destructor, if there is any stored.
    /// Afterwards `has_value()` will return `false`.
    /// \output_section Modifiers
    void reset() noexcept
    {
        if (has_value())
            get_storage().destroy_value();
    }

    /// \effects First destroys any old value like `reset()`.
    /// Then creates the value by perfectly forwarding `args...` to the constructor of `value_type`.
    /// \throws Anything thrown by the constructor of `value_type`.
    /// If this function is left by an exception, the optional will be empty.
    /// \notes If the `create_value()` function of the `StoragePolicy` does not accept the
    /// arguments, this function will not participate in overload resolution. \synopsis_return void
    template <typename... Args>
    auto emplace(Args&&... args) noexcept(std::is_nothrow_constructible<value_type, Args...>::value)
        -> decltype(std::declval<basic_optional<storage>>().get_storage().create_value(
            std::forward<Args>(args)...))
    {
        reset();
        get_storage().create_value(std::forward<Args>(args)...);
    }

    /// \effects If `has_value()` is `false` creates it by calling the constructor with `arg`
    /// perfectly forwarded. Otherwise assigns a perfectly forwarded `arg` to `value()`. \throws
    /// Anything thrown by the constructor or assignment operator chosen. \notes This function does
    /// not participate in overload resolution unless there is an `operator=` that takes `arg`
    /// without an implicit user-defined conversion and the `create_value()` function of the
    /// `StoragePolicy` accepts the argument. \synopsis_return void
    template <typename Arg,
              typename = typename std::enable_if<detail::is_direct_assignable<
                  decltype(std::declval<storage&>().get_value()), Arg&&>::value>::type>
    auto emplace(Arg&& arg) noexcept(std::is_nothrow_constructible<value_type, Arg>::value&&
                                         std::is_nothrow_assignable<value_type, Arg>::value)
        -> decltype(std::declval<basic_optional<storage>>().get_storage().create_value(
            std::forward<Arg>(arg)))
    {
        if (!has_value())
            get_storage().create_value(std::forward<Arg>(arg));
        else
            value() = std::forward<Arg>(arg);
    }

    //=== observers ===//
    /// \returns The same as `has_value()`.
    /// \output_section Observers
    explicit operator bool() const noexcept
    {
        return has_value();
    }

    /// \returns Whether or not the optional has a value.
    bool has_value() const noexcept
    {
        return get_storage().has_value();
    }

    /// Access to the stored value.
    /// \returns A reference to the stored value.
    /// The exact type depends on the `StoragePolicy`.
    /// \requires `has_value() == true`.
    /// \group value
    auto value() TYPE_SAFE_LVALUE_REF noexcept -> decltype(std::declval<storage&>().get_value())
    {
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{});
        return get_storage().get_value();
    }

    /// \group value
    auto value() const TYPE_SAFE_LVALUE_REF noexcept
        -> decltype(std::declval<const storage&>().get_value())
    {
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{});
        return get_storage().get_value();
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group value
    auto value() && noexcept -> decltype(std::declval<storage&&>().get_value())
    {
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{});
        return std::move(get_storage()).get_value();
    }

    /// \group value
    auto value() const && noexcept -> decltype(std::declval<const storage&&>().get_value())
    {
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{});
        return std::move(get_storage()).get_value();
    }
#endif

    /// \returns If it has a value, `value()`, otherwise `u` converted to the same type as
    /// `value()`. \requires `u` must be valid argument to the `value_or()` function of the
    /// `StoragePolicy`. \notes Depending on the `StoragePolicy`, this either returns a decayed type
    /// or a reference. \group value_or
    template <typename U>
    auto value_or(U&& u) const TYPE_SAFE_LVALUE_REF
        -> decltype(std::declval<const storage&>().get_value_or(std::forward<U>(u)))
    {
        return get_storage().get_value_or(std::forward<U>(u));
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group value_or
    template <typename U>
    auto value_or(U&& u) && -> decltype(std::declval<storage&&>().get_value_or(std::forward<U>(u)))
    {
        return std::move(get_storage()).get_value_or(std::forward<U>(u));
    }
#endif

    //=== factories ===//
    /// Maps an optional.
    /// \effects If the optional contains a value,
    /// calls the function with the value followed by the additional arguments perfectly forwarded.
    /// \returns A `basic_optional` rebound to the result type of the function,
    /// that is empty if `*this` is empty and contains the result of the function otherwise.
    /// \requires `f` must either be a function or function object of matching signature,
    /// or a member function pointer of the stored type with compatible signature.
    /// \notes Due to the way [ts::basic_optional::rebind]() works,
    /// if the result of the function is `void`, `map()` will return `void` as well,
    /// and if the result of the function is an optional itself,
    /// `map()` will return the optional unchanged.
    /// \unique_name *map
    /// \group map
    /// \exclude return
    template <typename Func, typename... Args>
    auto map(Func&& f, Args&&... args) TYPE_SAFE_LVALUE_REF
#if !TYPE_SAFE_USE_RETURN_TYPE_DEDUCTION
        -> rebind<decltype(detail::map_invoke(std::forward<Func>(f), this->value(),
                                              std::forward<Args>(args)...))>
#endif
    {
        using return_type = decltype(
            detail::map_invoke(std::forward<Func>(f), value(), std::forward<Args>(args)...));
        if (has_value())
            return rebind<return_type>(
                detail::map_invoke(std::forward<Func>(f), value(), std::forward<Args>(args)...));
        else
            return static_cast<rebind<return_type>>(nullopt);
    }

    /// \unique_name *map_const
    /// \group map
    /// \exclude return
    template <typename Func, typename... Args>
    auto map(Func&& f, Args&&... args) const TYPE_SAFE_LVALUE_REF
#if !TYPE_SAFE_USE_RETURN_TYPE_DEDUCTION
        -> rebind<decltype(detail::map_invoke(std::forward<Func>(f), this->value(),
                                              std::forward<Args>(args)...))>
#endif
    {
        using return_type = decltype(
            detail::map_invoke(std::forward<Func>(f), value(), std::forward<Args>(args)...));
        if (has_value())
            return rebind<return_type>(
                detail::map_invoke(std::forward<Func>(f), value(), std::forward<Args>(args)...));
        else
            return static_cast<rebind<return_type>>(nullopt);
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \unique_name *map_rvalue
    /// \group map
    /// \exclude return
    template <typename Func, typename... Args>
    auto map(Func&& f, Args&&... args) &&
#    if !TYPE_SAFE_USE_RETURN_TYPE_DEDUCTION
        -> rebind<decltype(detail::map_invoke(std::forward<Func>(f), this->value(),
                                              std::forward<Args>(args)...))>
#    endif
    {
        using return_type = decltype(
            detail::map_invoke(std::forward<Func>(f), value(), std::forward<Args>(args)...));
        if (has_value())
            return rebind<return_type>(
                detail::map_invoke(std::forward<Func>(f), value(), std::forward<Args>(args)...));
        else
            return static_cast<rebind<return_type>>(nullopt);
    }

    /// \unique_name *map_rvalue_const
    /// \group map
    /// \exclude return
    template <typename Func, typename... Args>
    auto map(Func&& f, Args&&... args) const&&
#    if !TYPE_SAFE_USE_RETURN_TYPE_DEDUCTION
        -> rebind<decltype(detail::map_invoke(std::forward<Func>(f), this->value(),
                                              std::forward<Args>(args)...))>
#    endif
    {
        using return_type = decltype(
            detail::map_invoke(std::forward<Func>(f), value(), std::forward<Args>(args)...));
        if (has_value())
            return rebind<return_type>(
                detail::map_invoke(std::forward<Func>(f), value(), std::forward<Args>(args)...));
        else
            return static_cast<rebind<return_type>>(nullopt);
    }
#endif
};

/// \entity TYPE_SAFE_DETAIL_MAKE_OP
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op, Expr, Expr2)                                                  \
    template <class StoragePolicy>                                                                 \
    bool operator Op(const basic_optional<StoragePolicy>& lhs, nullopt_t)                          \
    {                                                                                              \
        return (void)lhs, Expr;                                                                    \
    } /** \group optional_comp_null */                                                             \
    template <class StoragePolicy>                                                                 \
    bool operator Op(nullopt_t, const basic_optional<StoragePolicy>& rhs)                          \
    {                                                                                              \
        return (void)rhs, Expr2;                                                                   \
    }

/// Comparison of [ts::basic_optional]() with [ts::nullopt]().
///
/// An optional is equal to [ts::nullopt]() if it does not have a value.
/// Nothing is less than [ts::nullopt](), it is only less than an optional,
/// A optional compares equal to `nullopt`, when it does not have a value.
/// A optional compares never less to `nullopt`, `nullopt` compares less only if the optional has a
/// value. The other comparisons behave accordingly. \group optional_comp_null Optional null
/// comparison \module optional
TYPE_SAFE_DETAIL_MAKE_OP(==, !lhs.has_value(), !rhs.has_value())
/// \group optional_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(!=, lhs.has_value(), rhs.has_value())
/// \group optional_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(<, false, rhs.has_value())
/// \group optional_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(<=, !lhs.has_value(), true)
/// \group optional_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(>, lhs.has_value(), false)
/// \group optional_comp_null
TYPE_SAFE_DETAIL_MAKE_OP(>=, true, !rhs.has_value())

#undef TYPE_SAFE_DETAIL_MAKE_OP

/// \entity TYPE_SAFE_DETAIL_MAKE_OP
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op, Expr, Expr2)                                                  \
    template <class StoragePolicy, typename U>                                                     \
    auto operator Op(const basic_optional<StoragePolicy>& lhs, const U& rhs)                       \
        ->decltype(typename StoragePolicy::value_type(lhs.value()) Op rhs)                         \
    {                                                                                              \
        using value_type = typename StoragePolicy::value_type;                                     \
        return Expr;                                                                               \
    }                                                                                              \
    /** \synopsis_return bool                                                                      \
     * \group optional_comp_value */                                                               \
    template <class StoragePolicy, typename U>                                                     \
    auto operator Op(const U& lhs, const basic_optional<StoragePolicy>& rhs)                       \
        ->decltype(lhs Op typename StoragePolicy::value_type(rhs.value()))                         \
    {                                                                                              \
        using value_type = typename StoragePolicy::value_type;                                     \
        return Expr2;                                                                              \
    }

/// Compares a [ts::basic_optional]() with a value.
///
/// An optional compares equal to a value if it has a value
/// and the value compares equal.
/// An optional compares less to a value if it does not have a value
/// or the value compares less.
/// A value compares less to an optional if the optional has a value
/// and the value compares less than the optional.
/// The other comparisons behave accordingly.
///
/// Value comparison is done by the comparison operator of the `value_type`,
/// a function only participates in overload resolution if the `value_type`,
/// has that comparison function.
/// \synopsis_return bool
/// \group optional_comp_value Optional value comparison
/// \module optional
TYPE_SAFE_DETAIL_MAKE_OP(==, lhs.has_value() && value_type(lhs.value()) == rhs,
                         rhs.has_value() && lhs == value_type(rhs.value()))
/// \synopsis_return bool
/// \group optional_comp_value
TYPE_SAFE_DETAIL_MAKE_OP(!=, !lhs.has_value() || value_type(lhs.value()) != rhs,
                         !rhs.has_value() || lhs != value_type(rhs.value()))
/// \synopsis_return bool
/// \group optional_comp_value
TYPE_SAFE_DETAIL_MAKE_OP(<, !lhs.has_value() || value_type(lhs.value()) < rhs,
                         rhs.has_value() && lhs < value_type(rhs.value()))
/// \synopsis_return bool
/// \group optional_comp_value
TYPE_SAFE_DETAIL_MAKE_OP(<=, !lhs.has_value() || value_type(lhs.value()) <= rhs,
                         rhs.has_value() && lhs <= value_type(rhs.value()))
/// \synopsis_return bool
/// \group optional_comp_value
TYPE_SAFE_DETAIL_MAKE_OP(>, lhs.has_value() && value_type(lhs.value()) > rhs,
                         !rhs.has_value() || lhs > value_type(rhs.value()))
/// \synopsis_return bool
/// \group optional_comp_value
TYPE_SAFE_DETAIL_MAKE_OP(>=, lhs.has_value() && value_type(lhs.value()) >= rhs,
                         !rhs.has_value() || lhs >= value_type(rhs.value()))

#undef TYPE_SAFE_DETAIL_MAKE_OP

/// \entity TYPE_SAFE_DETAIL_MAKE_OP
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <class StoragePolicy>                                                                 \
    auto operator Op(const basic_optional<StoragePolicy>& lhs,                                     \
                     const basic_optional<StoragePolicy>& rhs)                                     \
        ->decltype(lhs.value() Op rhs.value())                                                     \
    {                                                                                              \
        return lhs.has_value() ? lhs.value() Op rhs : nullopt Op rhs;                              \
    }

/// Compares two [ts::basic_optional]() objects.
///
/// If `lhs` has a value, forwards to `lhs.value() <op> rhs`.
/// Else forwards to `nullopt <op> rhs`.
/// \synopsis_return bool
/// \group optional_comp Optional comparison
/// \module optional
TYPE_SAFE_DETAIL_MAKE_OP(==)
/// \synopsis_return bool
/// \group optional_comp
TYPE_SAFE_DETAIL_MAKE_OP(!=)
/// \synopsis_return bool
/// \group optional_comp
TYPE_SAFE_DETAIL_MAKE_OP(<)
/// \synopsis_return bool
/// \group optional_comp
TYPE_SAFE_DETAIL_MAKE_OP(<=)
/// \synopsis_return bool
/// \group optional_comp
TYPE_SAFE_DETAIL_MAKE_OP(>)
/// \synopsis_return bool
/// \group optional_comp
TYPE_SAFE_DETAIL_MAKE_OP(>=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

/// With operation for [ts::optional]().
/// \effects Calls the `operator()` of `f` passing it the value of `opt` and additional arguments,
/// if it has a value.
/// Otherwise does nothing.
/// \module optional
/// \param 3
/// \exclude
template <class Optional, typename Func, typename... Args,
          typename = typename std::enable_if<detail::is_optional<Optional>::value>::type>
void with(Optional&& opt, Func&& f, Args&&... additional_args)
{
    if (opt.has_value())
        std::forward<Func>(f)(std::forward<Optional>(opt).value(),
                              std::forward<Args>(additional_args)...);
}

//=== optional ===//
/// A `StoragePolicy` for [ts::basic_optional]() that is similar to [std::optional<T>]()'s
/// implementation.
///
/// It uses [std::aligned_storage]() and a `bool` flag whether a value was created.
/// \requires `T` must not be a reference.
/// \module optional
/// \output_section Optional
template <typename T>
class direct_optional_storage
{
    static_assert(!std::is_reference<T>::value,
                  "T must not be a reference; use optional_ref<T> for that");

public:
    using value_type             = typename std::remove_cv<T>::type;
    using lvalue_reference       = T&;
    using const_lvalue_reference = const T&;
    using rvalue_reference       = T&&;
    using const_rvalue_reference = const T&&;

    template <typename U>
    using rebind = direct_optional_storage<U>;

    /// \effects Initializes it in the state without value.
    direct_optional_storage() noexcept : empty_(true) {}

    /// \effects Calls the constructor of `value_type` by perfectly forwarding `args`.
    /// Afterwards `has_value()` will return `true`.
    /// \throws Anything thrown by the constructor of `value_type` in which case `has_value()` is
    /// still `false`. \requires `has_value() == false`. \notes This function does not participate
    /// in overload resolution unless `value_type` is constructible from `args`. \synopsis_return
    /// void
    template <typename... Args>
    auto create_value(Args&&... args) ->
        typename std::enable_if<std::is_constructible<value_type, Args&&...>::value>::type
    {
        ::new (as_void()) value_type(std::forward<Args>(args)...);
        empty_ = false;
    }

    /// \effects Creates a value by copy(1)/move(2) constructing from the value stored in `other`,
    /// if there is any.
    /// \group create_value
    void create_value(const direct_optional_storage& other)
    {
        if (other.has_value())
            create_value(other.get_value());
    }

    /// \group create_value
    void create_value(direct_optional_storage&& other)
    {
        if (other.has_value())
            create_value(std::move(other.get_value()));
    }

    void create_value_explicit() {}

    /// \effects Copies the policy from `other`, by copy-constructing or assigning the stored value,
    /// if any.
    /// \throws Anything thrown by the copy constructor or copy assignment operator of `other`.
    /// \group copy_value
    /// \unique_name copy_value_assign
    /// \param Dummy
    /// \exclude
    /// \param 1
    /// \exclude
    template <typename Dummy = T,
              typename       = typename std::enable_if<std::is_copy_assignable<Dummy>::value>::type>
    void copy_value(const direct_optional_storage& other)
    {
        if (has_value())
        {
            if (other.has_value())
                get_value() = other.get_value();
            else
                destroy_value();
        }
        else if (other.has_value())
            create_value(other.get_value());
    }

    /// \unique_name copy_value_construct
    /// \group copy_value
    /// \param Dummy
    /// \exclude
    /// \param 1
    /// \exclude
    template <typename Dummy                                                             = T,
              typename std::enable_if<!std::is_copy_assignable<Dummy>::value, int>::type = 0>
    void copy_value(const direct_optional_storage& other)
    {
        if (has_value())
            destroy_value();
        create_value(other.get_value());
    }

    /// \effects Copies the policy from `other`, by move-constructing or assigning the stored value,
    /// if any.
    /// \throws Anything thrown by the move constructor or move assignment operator of `other`.
    /// \unique_name copy_value_move_assign
    /// \group copy_value_move
    /// \param Dummy
    /// \exclude
    /// \param 1
    /// \exclude
    template <typename Dummy = T,
              typename       = typename std::enable_if<std::is_move_assignable<Dummy>::value>::type>
    void copy_value(direct_optional_storage&& other)
    {
        if (has_value())
        {
            if (other.has_value())
                get_value() = std::move(other).get_value();
            else
                destroy_value();
        }
        else if (other.has_value())
            create_value(std::move(other).get_value());
    }

    /// \group copy_value_move
    /// \param Dummy
    /// \exclude
    /// \param 1
    /// \exclude
    template <typename Dummy                                                             = T,
              typename std::enable_if<!std::is_move_assignable<Dummy>::value, int>::type = 0>
    void copy_value(direct_optional_storage&& other)
    {
        if (has_value())
            destroy_value();
        create_value(std::move(other).get_value());
    }

    /// \effects Swaps the value with the value in `other`.
    void swap_value(direct_optional_storage& other)
    {
        if (has_value() && other.has_value())
        {
            using std::swap;
            swap(get_value(), other.get_value());
        }
        else if (has_value())
        {
            other.create_value(std::move(get_value()));
            destroy_value();
        }
        else if (other.has_value())
        {
            create_value(std::move(other).get_value());
            other.destroy_value();
        }
    }

    /// \effects Calls the destructor of `value_type`.
    /// Afterwards `has_value()` will return `false`.
    /// \requires `has_value() == true`.
    void destroy_value() noexcept
    {
        get_value().~value_type();
        empty_ = true;
    }

    /// \returns Whether or not there is a value stored.
    bool has_value() const noexcept
    {
        return !empty_;
    }

    /// \returns A (`const`) (rvalue) reference to the stored value.
    /// \requires `has_value() == true`.
    /// \group value
    lvalue_reference get_value() TYPE_SAFE_LVALUE_REF noexcept
    {
        return *static_cast<value_type*>(as_void());
    }

    /// \group value
    const_lvalue_reference get_value() const TYPE_SAFE_LVALUE_REF noexcept
    {
        return *static_cast<const value_type*>(as_void());
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group value
    rvalue_reference get_value() && noexcept
    {
        return std::move(*static_cast<value_type*>(as_void()));
    }

    /// \group value
    const_rvalue_reference get_value() const&& noexcept
    {
        return std::move(*static_cast<const value_type*>(as_void()));
    }
#endif

    /// \returns Either `get_value()` or `u` converted to `value_type`.
    /// \requires `value_type` must be copy(1)/move(2) constructible and `u` convertible to
    /// `value_type`. \group get_value_or \param 1 \exclude
    template <typename U,
              typename
              = typename std::enable_if<std::is_copy_constructible<value_type>::value
                                        && std::is_convertible<U&&, value_type>::value>::type>
    value_type get_value_or(U&& u) const TYPE_SAFE_LVALUE_REF
    {
        return has_value() ? get_value() : static_cast<value_type>(std::forward<U>(u));
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group get_value_or
    /// \param 1
    /// \exclude
    template <typename U,
              typename
              = typename std::enable_if<std::is_move_constructible<value_type>::value
                                        && std::is_convertible<U&&, value_type>::value>::type>
    value_type get_value_or(U&& u) &&
    {
        return has_value() ? std::move(get_value()) : static_cast<value_type>(std::forward<U>(u));
    }
#endif

private:
    void* as_void() noexcept
    {
        return static_cast<void*>(&storage_);
    }

    const void* as_void() const noexcept
    {
        return static_cast<const void*>(&storage_);
    }

    alignas(value_type) unsigned char storage_[sizeof(value_type)];
    bool      empty_;
};

/// A [ts::basic_optional]() that uses [ts::direct_optional_storage<T>]().
/// \module optional
template <typename T>
using optional = basic_optional<direct_optional_storage<T>>;

/// Uses [ts::optional_storage_policy_for]() to select the appropriate [ts::basic_optional]().
///
/// By default, it uses [ts::direct_optional_storage]().
/// \notes If `T` is `void`, `optional_for` will also be `void`.
/// \module optional
template <typename T>
using optional_for = basic_optional<direct_optional_storage<int>>::rebind<T>;

/// \returns A new [ts::optional<T>]() storing a copy of `t`.
/// \module optional
template <typename T>
optional<typename std::decay<T>::type> make_optional(T&& t)
{
    return std::forward<T>(t);
}

/// \returns A new [ts::optional<T>]() with a value created by perfectly forwarding `args` to the
/// constructor. \module optional
template <typename T, typename... Args>
optional<T> make_optional(Args&&... args)
{
    optional<T> result;
    result.emplace(std::forward<Args>(args)...);
    return result;
}
} // namespace type_safe

namespace std
{
/// Hash for [ts::basic_optional]().
/// \module optional
template <class StoragePolicy>
struct hash<type_safe::basic_optional<StoragePolicy>>
{
    using value_type = typename StoragePolicy::value_type;
    using value_hash = std::hash<value_type>;

    std::size_t operator()(const type_safe::basic_optional<StoragePolicy>& opt) const
        noexcept(noexcept(value_hash{}(std::declval<value_type>())))
    {
        return opt ? value_hash{}(opt.value()) : 19937; // magic value
    }
};
} // namespace std

#endif // TYPE_SAFE_OPTIONAL_HPP_INCLUDED
