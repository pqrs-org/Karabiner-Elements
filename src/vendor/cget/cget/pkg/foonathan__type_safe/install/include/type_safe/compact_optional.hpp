// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_COMPACT_OPTIONAL_HPP_INCLUDED
#define TYPE_SAFE_COMPACT_OPTIONAL_HPP_INCLUDED

#include <limits>
#include <type_traits>

#include <type_safe/optional.hpp>

namespace type_safe
{
/// \exclude
namespace detail
{
    template <typename StorageType, typename ValueType, typename T>
    using storage_reference =
        typename std::conditional<std::is_same<StorageType, ValueType>::value, T, ValueType>::type;
} // namespace detail

/// A `StoragePolicy` for [ts::basic_optional]() that is more space efficient than
/// [ts::direct_optional_storage]().
///
/// It is designed to have no space overhead compared to a regular object of the stored type.
/// This is accomplished by marking one value of the stored type as invalid
/// and using that in the empty state.
/// What the invalid value is is controlled by the `CompactPolicy`.
/// It must provide the following `static` member functions and typedefs:
/// * `value_type` - the value that is being stored conceptually
/// * `storage_type` - the actual type that is being stored
/// * `storage_type invalid_value()` - returns the value that marks the optional as empty
/// * `bool is_invalid(const storage_type&)` - returns `true` if the given value is invalid, `false`
/// otherwise
///
/// In the cases where `value_type` and `storage_type` differ,
/// the `get_value()` functions will not return references, but a copy instead.
/// The implementation assumes that `invalid_value()` and `is_invalid()` are `noexcept` and cheap.
/// \notes For a compact optional of pointer type,
/// use [ts::optional_ref]().
/// \module optional
template <class CompactPolicy>
class compact_optional_storage
{
public:
    using value_type = typename std::remove_cv<typename CompactPolicy::value_type>::type;
    static_assert(!std::is_reference<value_type>::value,
                  "value_type must not be a reference; use optional_ref<T> for that");
    using storage_type = typename CompactPolicy::storage_type;

    using lvalue_reference = detail::storage_reference<storage_type, value_type, value_type&>;
    using const_lvalue_reference
        = detail::storage_reference<storage_type, value_type, const value_type&>;

    using rvalue_reference = detail::storage_reference<storage_type, value_type, value_type&&>;
    using const_rvalue_reference
        = detail::storage_reference<storage_type, value_type, const value_type&&>;

    template <typename U>
    using rebind = direct_optional_storage<U>;

    /// \effects Initializes it in the state without value,
    /// i.e. sets the storage to the invalid value.
    compact_optional_storage() noexcept : storage_(CompactPolicy::invalid_value()) {}

    /// \effects Creates a temporary `value_type` by perfectly forwarding `args`,
    /// converts that to the `storage_type` and assigns it.
    /// Afterwards `has_value()` will return `true`.
    /// \throws Anything thrown by the constructor of `value_type`/`storage_type` or its move
    /// assignment operator in which case `has_value()` is still `false`. \requires `has_value() ==
    /// false` and the given value must not be invalid. \notes This function does not participate in
    /// overload resolution unless `value_type` is constructible from `args`. \synopsis template
    /// \<typename ... Args\>\nvoid create_value(Args&&... args);
    template <typename... Args>
    auto create_value(Args&&... args) ->
        typename std::enable_if<std::is_constructible<value_type, Args&&...>::value>::type
    {
        storage_ = static_cast<storage_type>(value_type(std::forward<Args>(args)...));
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{},
                     "create_value() called creating an invalid value");
    }

    /// \effects Copy assigns the `storage_type`.
    void create_value(const compact_optional_storage& other)
    {
        storage_ = other.storage_;
    }

    /// \effects Move assigns the `storage_type`.
    void create_value(compact_optional_storage&& other)
    {
        storage_ = std::move(other.storage_);
    }

    void create_value_explicit() {}

    /// \effects Copy assigns the `storage_type`.
    void copy_value(const compact_optional_storage& other)
    {
        storage_ = other.storage_;
    }

    /// \effects Move assigns the `storage_type`.
    void copy_value(compact_optional_storage&& other)
    {
        storage_ = std::move(other.storage_);
    }

    /// \effects Swaps the `storage_type`.
    void swap_value(compact_optional_storage& other)
    {
        using std::swap;
        swap(storage_, other.storage_);
    }

    /// \effects Destroys the value by setting it to the invalid storage value.
    /// Afterwards `has_value()` will return `false`.
    /// \requires `has_value() == true`.
    void destroy_value() noexcept
    {
        storage_ = CompactPolicy::invalid_value();
    }

    /// \returns Whether or not there is a value stored,
    /// i.e. whether the stored value is not invalid.
    bool has_value() const noexcept
    {
        return !CompactPolicy::is_invalid(storage_);
    }

    /// \returns A (suitable) reference to the stored value
    /// or a copy of the value depending on the policy.
    /// \requires `has_value() == true`.
    /// \group get_value
    lvalue_reference get_value() TYPE_SAFE_LVALUE_REF noexcept
    {
        return static_cast<lvalue_reference>(storage_);
    }

    /// \group get_value
    const_lvalue_reference get_value() const TYPE_SAFE_LVALUE_REF noexcept
    {
        return static_cast<const_lvalue_reference>(storage_);
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group get_value
    rvalue_reference get_value() && noexcept
    {
        return static_cast<rvalue_reference>(storage_);
    }

    /// \group get_value
    const_rvalue_reference get_value() const&& noexcept
    {
        return static_cast<const_rvalue_reference>(storage_);
    }
#endif

    /// \returns Either `get_value()` or `u` converted to `value_type`.
    /// \requires `value_type` must be copy (1)/move (2) constructible and `u` convertible to
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
    storage_type storage_;
};

/// An alias for [ts::basic_optional]() using [ts::compact_optional_storage]() with the given
/// `CompactPolicy`. \module optional
template <class CompactPolicy>
using compact_optional = basic_optional<compact_optional_storage<CompactPolicy>>;

/// A `CompactPolicy` for [ts::compact_optional_storage]() for boolean types.
///
/// It is designed for either `bool` or [ts::boolean]().
/// \notes It uses a different `storage_type` and thus cannot return a reference to the stored
/// value. \module optional
template <typename Boolean>
class compact_bool_policy
{
public:
    using value_type   = Boolean;
    using storage_type = char;

    static storage_type invalid_value() noexcept
    {
        return 3;
    }

    static bool is_invalid(storage_type storage) noexcept
    {
        return storage == invalid_value();
    }
};

/// A `CompactPolicy` for [ts::compact_optional_storage]() for integer types.
///
/// The given `Invalid` value will be used to mark an empty optional.
template <typename Integer, Integer Invalid>
class compact_integer_policy
{
    static_assert(std::is_integral<Integer>::value, "must be an integral value");

public:
    using value_type   = Integer;
    using storage_type = Integer;

    static storage_type invalid_value() noexcept
    {
        return Invalid;
    }

    static bool is_invalid(const storage_type& storage) noexcept
    {
        return storage == Invalid;
    }
};

/// A `CompactPolicy` for [ts::compact_optional_storage]() for floating point types.
///
/// `NaN` will be used to mark an empty optional.
/// \module optional
template <typename FloatingPoint>
class compact_floating_point_policy
{
    static_assert(std::is_floating_point<FloatingPoint>::value, "must be a floating point");

public:
    using value_type   = FloatingPoint;
    using storage_type = FloatingPoint;

    static storage_type invalid_value() noexcept
    {
        return std::numeric_limits<value_type>::quiet_NaN();
    }

    static bool is_invalid(const storage_type& storage) noexcept
    {
        // NaN is not equal to anything
        return storage != storage;
    }
};

/// \exclude
namespace compact_enum_detail
{
    template <typename Enum>
    struct underlying_type_impl
    {
        static_assert(std::is_enum<Enum>::value, "must be an enumeration type");
        using type = typename std::underlying_type<Enum>::type;
    };

    template <typename Enum>
    using underlying_type = typename underlying_type_impl<Enum>::type;
} // namespace compact_enum_detail

/// A `CompactPolicy` for [ts::compact_optional_storage]() for enumeration types.
///
/// It uses the given `Invalid` value of the underlying type to mark an empty optional.
/// \notes It uses a different `storage_type` and thus cannot return a reference to the stored
/// value. \module optional
template <typename Enum, compact_enum_detail::underlying_type<Enum> Invalid>
class compact_enum_policy
{
public:
    using value_type   = Enum;
    using storage_type = compact_enum_detail::underlying_type<Enum>;

    static storage_type invalid_value() noexcept
    {
        return Invalid;
    }

    static bool is_invalid(const storage_type& storage) noexcept
    {
        return storage == Invalid;
    }
};

/// \exclude
namespace detail
{
    template <class Container>
    auto is_empty(int, const Container& c) -> decltype(c.empty())
    {
        return c.empty();
    }

    template <class Container>
    auto is_empty(short, const Container& c) -> decltype(empty(c))
    {
        return empty(c);
    }
} // namespace detail

/// A `CompactPolicy` for [ts::compact_optional_storage]() for container types.
///
/// A `Container` is a type with a cheap no-throwing default constructor initializing it empty,
/// and either an `empty()` member function or ADL function that returns `true` if the container is
/// empty, `false` otherwise. An empty container will be marked as an empty optional. \module
/// optional
template <class Container>
class compact_container_policy
{
public:
    using value_type   = Container;
    using storage_type = Container;

    static storage_type invalid_value() noexcept
    {
        return {};
    }

    static bool is_invalid(const storage_type& storage) noexcept
    {
        return static_cast<bool>(detail::is_empty(0, storage));
    }
};
} // namespace type_safe

#endif // TYPE_SAFE_COMPACT_OPTIONAL_HPP_INCLUDED
