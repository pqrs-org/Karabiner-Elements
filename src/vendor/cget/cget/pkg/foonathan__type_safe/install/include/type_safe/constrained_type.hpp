// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_CONSTRAINED_TYPE_HPP_INCLUDED
#define TYPE_SAFE_CONSTRAINED_TYPE_HPP_INCLUDED

#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <type_safe/config.hpp>
#include <type_safe/detail/assert.hpp>
#include <type_safe/detail/is_nothrow_swappable.hpp>

namespace type_safe
{
//=== constrained_type ===//
/// A `Verifier` for [ts::constrained_type]() that `DEBUG_ASSERT`s the constraint.
///
/// If [TYPE_SAFE_ENABLE_PRECONDITION_CHECKS]() is `true`,
/// it will assert that the value fulfills the predicate and returns it unchanged.
/// If assertions are disabled, it will just return the value unchanged.
/// \output_section Constrained type
struct assertion_verifier
{
    template <typename Value, typename Predicate>
    static constexpr auto verify(Value&& val, const Predicate& p) ->
        typename std::decay<Value>::type
    {
        return p(val) ? std::forward<Value>(val)
                      : (DEBUG_UNREACHABLE(detail::precondition_error_handler{},
                                           "value does not fulfill constraint"),
                         std::forward<Value>(val));
    }
};

/// The exception class thrown by the [ts::throwing_verifier]().
class constrain_error : public std::logic_error
{
public:
    constrain_error()
    : std::logic_error("Constraint of type_safe::constrained_type wasn't fulfilled")
    {}
};

/// A `Verifier` for [ts::constrained_type]() that throws an exception in case of failure.
///
/// Unlike [ts::assertion_verifier](), it will *always* check the constrain.
/// If it is not fulfilled, it throws an exception of type [ts::constrain_error](),
/// otherwise return the original value unchanged.
/// \notes [ts::assertion_verifier]() is the default,
/// because a constrain violation is a logic error,
/// usually done by a programmer.
/// Use this one only if you want to use [ts::constrained_type]()
/// with unsanitized user input, for example.
struct throwing_verifier
{
    template <typename Value, typename Predicate>
    static constexpr auto verify(Value&& val, const Predicate& p) ->
        typename std::decay<Value>::type
    {
        return p(val) ? std::forward<Value>(val)
                      : (TYPE_SAFE_THROW(constrain_error{}), std::forward<Value>(val));
    }
};

/// \exclude
namespace detail
{
    template <class Constraint, typename T>
    auto verify_static_constrained(int) -> typename Constraint::template is_valid<T>;

    template <class Constraint, typename T>
    auto verify_static_constrained(char) -> std::true_type;

    template <class Constraint, typename T>
    struct is_valid : decltype(verify_static_constrained<Constraint, T>(0))
    {};
} // namespace detail

template <typename T, class Constraint, class Verifier>
class constrained_modifier;

/// A value of type `T` that always fulfills the predicate `Constraint`.
///
/// The `Constraint` is checked by the `Verifier`.
/// The `Constraint` can also provide a nested template `is_valid<T>` to statically check types.
/// Those will be checked regardless of the `Verifier`.
///
/// If `T` is `const`, the `modify()` function will not be available,
/// you can only modify the type by assigning a completely new value to it.
/// \requires `T` must not be a reference, `Constraint` must be a moveable, non-final class where no
/// operation throws, and `Verifier` must provide a `static` function `[const] T[&] verify(const T&,
/// const Predicate&)`. The return value is stored and it must always fulfill the predicate. It also
/// requires that no `const` operation on `T` may modify it in a way that the predicate isn't
/// fulfilled anymore. \notes Additional requirements of the `Constraint` depend on the `Verifier`
/// used. If not stated otherwise, a `Verifier` in this library requires that the `Constraint` is a
/// `Predicate` for `T`.
template <typename T, typename Constraint, class Verifier = assertion_verifier>
class constrained_type : Constraint
{
public:
    using value_type           = typename std::remove_cv<T>::type;
    using constraint_predicate = Constraint;

    /// \effects Creates it giving it a valid `value` and a `predicate`.
    /// The `value` will be copied(1)/moved(2) and verified.
    /// \throws Anything thrown by the copy(1)/move(2) constructor of `value_type`
    /// or the `Verifier` if the `value` is invalid.
    /// \group value_ctor
    explicit constexpr constrained_type(const value_type&    value,
                                        constraint_predicate predicate = {})
    : Constraint(std::move(predicate)), value_(Verifier::verify(value, get_constraint()))
    {}

    /// \group value_ctor
    explicit constexpr constrained_type(
        value_type&&         value,
        constraint_predicate predicate
        = {}) noexcept(std::is_nothrow_constructible<value_type>::
                           value&& noexcept(Verifier::verify(std::move(value),
                                                             std::move(predicate))))
    : Constraint(std::move(predicate)), value_(Verifier::verify(std::move(value), get_constraint()))
    {}

    /// \exclude
    template <typename U,
              typename
              = typename std::enable_if<!detail::is_valid<constraint_predicate, U>::value>::type>
    constrained_type(U) = delete;

    /// \effects Copies the value and predicate of `other`.
    /// \throws Anything thrown by the copy constructor of `value_type`.
    /// \requires `Constraint` must be copyable.
    constexpr constrained_type(const constrained_type& other)
    : Constraint(other), value_(other.debug_verify())
    {}

    /// \effects Destroys the value.
    ~constrained_type() noexcept = default;

    /// \effects Same as assigning `constrained_type(other, get_constraint()).release()` to the
    /// stored value. It will invoke copy(1)/move(2) constructor followed by move assignment
    /// operator. \throws Anything thrown by the copy(1)/move(2) constructor or move assignment
    /// operator of `value_type`, or the `Verifier` if the `value` is invalid. If the `value` is
    /// invalid, nothing will be changed. \requires `Constraint` must be copyable. \group
    /// assign_value
    TYPE_SAFE_CONSTEXPR14 constrained_type& operator=(const value_type& other)
    {
        constrained_type tmp(other, get_constraint());
        value_ = std::move(tmp).release();
        return *this;
    }

    /// \group assign_value
    TYPE_SAFE_CONSTEXPR14 constrained_type& operator=(value_type&& other) noexcept(
        std::is_nothrow_move_assignable<value_type>::value&& noexcept(
            Verifier::verify(std::move(other), std::declval<Constraint&>())))
    {
        constrained_type tmp(std::move(other), get_constraint());
        value_ = std::move(tmp).release();
        return *this;
    }

    /// \exclude
    template <typename U,
              typename
              = typename std::enable_if<!detail::is_valid<constraint_predicate, U>::value>::type>
    constrained_type& operator=(U) = delete;

    /// \effects Copies the value and predicate from `other`.
    /// \throws Anything thrown by the copy assignment operator of `value_type`.
    /// \requires `Constraint` must be copyable.
    TYPE_SAFE_CONSTEXPR14 constrained_type& operator=(const constrained_type& other)
    {
        constrained_type tmp(other);
        swap(*this, tmp);
        return *this;
    }

    /// \effects Swaps the value and predicate of a `a` and `b`.
    /// \throws Anything thrown by the swap function of `value_type`.
    /// \requires `Constraint` must be swappable.
    friend TYPE_SAFE_CONSTEXPR14 void swap(constrained_type& a, constrained_type& b) noexcept(
        detail::is_nothrow_swappable<value_type>::value)
    {
        a.debug_verify();
        b.debug_verify();

        using std::swap;
        swap(a.value_, b.value_);
        swap(static_cast<Constraint&>(a), static_cast<Constraint&>(b));
    }

    /// \returns A proxy object to provide verified write-access to the stored value.
    /// \notes This function does not participate in overload resolution if `T` is `const`.
    template <typename Dummy = T,
              typename       = typename std::enable_if<!std::is_const<Dummy>::value>::type>
    constrained_modifier<T, Constraint, Verifier> modify() noexcept
    {
        debug_verify();
        return constrained_modifier<T, Constraint, Verifier>(*this);
    }

    /// \effects Moves the stored value out of the `constrained_type`,
    /// it will not be checked further.
    /// \returns An rvalue reference to the stored value.
    /// \notes After this function is called, the object must not be used anymore
    /// except as target for assignment or in the destructor.
    TYPE_SAFE_CONSTEXPR14 value_type&& release() TYPE_SAFE_RVALUE_REF noexcept
    {
        debug_verify();
        return std::move(value_);
    }

    /// Dereference operator.
    /// \returns A `const` reference to the stored value.
    constexpr const value_type& operator*() const noexcept
    {
        return get_value();
    }

    /// Member access operator.
    /// \returns A `const` pointer to the stored value.
    constexpr const value_type* operator->() const noexcept
    {
        return std::addressof(get_value());
    }

    /// \returns A `const` reference to the stored value.
    constexpr const value_type& get_value() const noexcept
    {
        return debug_verify();
    }

    /// \returns The predicate that determines validity.
    constexpr const constraint_predicate& get_constraint() const noexcept
    {
        return *this;
    }

private:
    constexpr const value_type& debug_verify() const noexcept
    {
#if TYPE_SAFE_ENABLE_ASSERTIONS
        return Verifier::verify(value_, get_constraint()), value_;
#else
        return value_;
#endif
    }

    TYPE_SAFE_CONSTEXPR14 value_type& get_non_const() noexcept
    {
        return value_;
    }

    value_type value_;
    friend constrained_modifier<T, Constraint, Verifier>;
};

/// Specialization of [ts::constrained_type]() for references.
///
/// It models a reference to a value that always fulfills the given constraint.
/// The value must not be changed by other means, it is thus perfect for function parameters.
/// \unique_name constrained_type_ref
template <typename T, class Constraint, class Verifier>
class constrained_type<T&, Constraint, Verifier> : Constraint
{
public:
    using value_type           = T;
    using constraint_predicate = Constraint;

    /// \effects Binds the reference to the given object.
    explicit constexpr constrained_type(T& value, constraint_predicate predicate = {})
    : Constraint(std::move(predicate)), ref_(&Verifier::verify(value, get_constraint()))
    {}

    /// \exclude
    template <typename U,
              typename
              = typename std::enable_if<!detail::is_valid<constraint_predicate, U>::value>::type>
    constrained_type(U) = delete;

    /// \returns A proxy object to provide verified write-access to the referred value.
    /// \notes This function does not participate in overload resolution if `T` is `const`.
    template <typename Dummy = T,
              typename       = typename std::enable_if<!std::is_const<Dummy>::value>::type>
    constrained_modifier<T&, Constraint, Verifier> modify() noexcept
    {
        debug_verify();
        return constrained_modifier<T&, Constraint, Verifier>(*this);
    }

    /// Dereference operator.
    /// \returns A `const` reference to the referred value.
    constexpr const value_type& operator*() const noexcept
    {
        return get_value();
    }

    /// Member access operator.
    /// \returns A `const` pointer to the referred value.
    constexpr const value_type* operator->() const noexcept
    {
        return &get_value();
    }

    /// \returns A `const` reference to the referred value.
    constexpr const value_type& get_value() const noexcept
    {
        return debug_verify();
    }

    /// \returns The predicate that determines validity.
    constexpr const constraint_predicate& get_constraint() const noexcept
    {
        return *this;
    }

private:
    constexpr const value_type& debug_verify() const noexcept
    {
#if TYPE_SAFE_ENABLE_ASSERTIONS
        return Verifier::verify(*ref_, get_constraint());
#else
        return *ref_;
#endif
    }

    TYPE_SAFE_CONSTEXPR14 value_type& get_non_const() noexcept
    {
        return *ref_;
    }

    T* ref_;
    friend constrained_modifier<T&, Constraint, Verifier>;
};

/// Alias for [ts::constrained_type<T&>](standardese://ts::constrained_type_ref/).
template <typename T, class Constraint, class Verifier = assertion_verifier>
using constrained_ref = constrained_type<T&, Constraint, Verifier>;

/// A proxy class to provide write access to the stored value of a [ts::constrained_type]().
///
/// The destructor will verify the value again.
template <typename T, class Constraint, class Verifier>
class constrained_modifier
{
public:
    using value_type = typename constrained_type<T, Constraint, Verifier>::value_type;

    /// \effects Move constructs it.
    /// `other` will not verify any value afterwards.
    constrained_modifier(constrained_modifier&& other) noexcept : value_(other.value_)
    {
        other.value_ = nullptr;
    }

    /// \effects Verifies the value, if there is any.
    ~constrained_modifier() noexcept(false)
    {
        if (value_)
            Verifier::verify(**value_, value_->get_constraint());
    }

    /// \effects Move assigns it.
    /// `other` will not verify any value afterwards.
    constrained_modifier& operator=(constrained_modifier&& other) noexcept
    {
        value_       = other.value_;
        other.value_ = nullptr;
        return *this;
    }

    /// Dereference operator.
    /// \returns A reference to the stored value.
    /// \requires It must not be in the moved-from state.
    value_type& operator*() noexcept
    {
        return get();
    }

    /// Member access operator.
    /// \returns A pointer to the stored value.
    /// \requires It must not be in the moved-from state.
    value_type* operator->() noexcept
    {
        return &get();
    }

    /// \returns A reference to the stored value.
    /// \requires It must not be in the moved-from state.
    value_type& get() noexcept
    {
        DEBUG_ASSERT(value_, detail::precondition_error_handler{});
        return value_->get_non_const();
    }

private:
    constrained_modifier(constrained_type<T, Constraint, Verifier>& value) noexcept : value_(&value)
    {}

    constrained_type<T, Constraint, Verifier>* value_;
    friend constrained_type<T, Constraint, Verifier>;
};

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename T, typename Constraint, class Verifier>                                     \
    constexpr auto operator Op(const constrained_type<T, Constraint, Verifier>& lhs,               \
                               const constrained_type<T, Constraint, Verifier>&                    \
                                   rhs) noexcept(noexcept(lhs.get_value() Op rhs.get_value()))     \
        ->decltype(lhs.get_value() Op rhs.get_value())                                             \
    {                                                                                              \
        return lhs.get_value() Op rhs.get_value();                                                 \
    }

/// Compares a [ts::constrained_type]().
/// \returns The result of the comparison of the underlying value.
/// \notes The comparison operators do not participate in overload resolution,
/// unless the stored type provides them as well.
/// \synopsis_return bool
/// \group constrained_comp -Constrained type comparison
TYPE_SAFE_DETAIL_MAKE_OP(==)
/// \synopsis_return bool
/// \group constrained_comp
TYPE_SAFE_DETAIL_MAKE_OP(!=)
/// \synopsis_return bool
/// \group constrained_comp
TYPE_SAFE_DETAIL_MAKE_OP(<)
/// \synopsis_return bool
/// \group constrained_comp
TYPE_SAFE_DETAIL_MAKE_OP(<=)
/// \synopsis_return bool
/// \group constrained_comp
TYPE_SAFE_DETAIL_MAKE_OP(>)
/// \synopsis_return bool
/// \group constrained_comp
TYPE_SAFE_DETAIL_MAKE_OP(>=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

/// Creates a [ts::constrained_type]().
/// \returns A [ts::constrained_type]() with the given `value`,  `Constraint` and `Verifier`.
/// \unique_name constrain_verifier
template <class Verifier, typename T, typename Constraint>
constexpr auto constrain(T&& value, Constraint c)
    -> constrained_type<typename std::decay<T>::type, Constraint, Verifier>
{
    return constrained_type<typename std::decay<T>::type, Constraint, Verifier>(std::forward<T>(
                                                                                    value),
                                                                                std::move(c));
}

/// Creates a [ts::constrained_type]() with the default verifier, [ts::assertion_verifier]().
/// \returns A [ts::constrained_type]() with the given `value` and `Constraint`.
/// \requires As it uses a `DEBUG_ASSERT` to check constrain,
/// the value must be valid.
/// \unique_name constrain
template <typename T, typename Constraint>
constexpr auto constrain(T&& value, Constraint c)
    -> constrained_type<typename std::decay<T>::type, Constraint>
{
    return constrained_type<typename std::decay<T>::type, Constraint>(std::forward<T>(value),
                                                                      std::move(c));
}

/// Creates a [ts::constrained_type]() using the [ts::throwing_verifier]().
/// \returns A [ts::constrained_type]() with the given `value` and `Constraint`.
/// \throws A [ts::constrain_error]() if the `value` isn't valid,
/// or anything else thrown by the constructor.
/// \notes This is meant for sanitizing user input,
/// using a recoverable error handling strategy.
template <typename T, typename Constraint>
constexpr auto sanitize(T&& value, Constraint c)
    -> constrained_type<typename std::decay<T>::type, Constraint, throwing_verifier>
{
    return constrained_type<typename std::decay<T>::type, Constraint,
                            throwing_verifier>(std::forward<T>(value), std::move(c));
}

/// With operation for [ts::constrained_type]().
/// \effects Calls `f` with a non-`const` reference to the stored value of the
/// [ts::constrained_type](). It checks that `f` does not change the validity of the object. \notes
/// The same behavior can be accomplished by using the `modify()` member function.
template <typename T, typename Constraint, class Verifier, typename Func, typename... Args>
void with(constrained_type<T, Constraint, Verifier>& value, Func&& f, Args&&... additional_args)
{
    auto modifier = value.modify();
    std::forward<Func>(f)(modifier.get(), std::forward<Args>(additional_args)...);
}

//=== tagged_type ===//
/// A `Verifier` for [ts::constrained_type]() that doesn't check the constraint.
///
/// It will simply return the value unchanged, without any checks.
/// \notes It does not impose any additional requirements on the `Predicate`.
/// \output_section Tagged type
struct null_verifier
{
    template <typename Value, typename Predicate>
    static constexpr Value&& verify(Value&& v, const Predicate&)
    {
        return std::forward<Value>(v);
    }
};

/// An alias for [ts::constrained_type]() that never checks the constraint.
///
/// It is useful for creating tagged types:
/// The `Constraint` - which does not need to be a predicate anymore - is a "tag" to differentiate a
/// type in different states. For example, you could have a "sanitized" value and a "non-sanitized"
/// value that have different types, so you cannot accidentally mix them. \notes It is only intended
/// if the `Constraint` cannot be formalized easily and/or is expensive. Otherwise
/// [ts::constrained_type]() is recommended as it does additional runtime checks in debug mode.
template <typename T, class Constraint>
using tagged_type = constrained_type<T, Constraint, null_verifier>;

/// An alias for [ts::tagged_type]() with reference.
template <typename T, class Constraint>
using tagged_ref = constrained_ref<T, Constraint, null_verifier>;

/// Creates a new [ts::tagged_type]().
/// \returns A [ts::tagged_type]() with the given `value` and `Constraint`.
template <typename T, typename Constraint>
constexpr auto tag(T&& value, Constraint c) -> tagged_type<typename std::decay<T>::type, Constraint>
{
    return tagged_type<typename std::decay<T>::type, Constraint>(std::forward<T>(value),
                                                                 std::move(c));
}

//=== constraints ===//
namespace constraints
{
    /// A `Constraint` for the [ts::constrained_type]().
    ///
    /// A value of a pointer type is valid if it is not equal to `nullptr`.
    /// This is borrowed from GSL's
    /// [non_null](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#a-namess-viewsagslview-views).
    struct non_null
    {
        template <typename T>
        struct is_valid : std::true_type
        {};

        template <typename T>
        constexpr bool operator()(const T& ptr) const noexcept
        {
            return ptr != nullptr;
        }
    };

    template <>
    struct non_null::is_valid<std::nullptr_t> : std::false_type
    {};

    /// A `Constraint` for the [ts::constrained_type]().
    ///
    /// A value of a container type is valid if it is not empty.
    /// Empty-ness is determined with either a member or non-member function.
    class non_empty
    {
        template <typename T>
        constexpr auto is_empty(int, const T& t) const noexcept(noexcept(t.empty()))
            -> decltype(t.empty())
        {
            return !t.empty();
        }

        template <typename T>
        constexpr bool is_empty(short, const T& t) const
        {
            return !empty(t);
        }

    public:
        template <typename T>
        constexpr bool operator()(const T& t) const
        {
            return is_empty(0, t);
        }
    };

    /// A `Constraint` for the [ts::constrained_type]().
    ///
    /// A value is valid if it not equal to the default constructed value.
    struct non_default
    {
        template <typename T>
        constexpr bool operator()(const T& t) const noexcept(noexcept(t == T()))
        {
            return !(t == T());
        }
    };

    /// A `Constraint` for the [ts::constrained_type]().
    ///
    /// A value of a pointer-like type is valid if the expression `!value` is `false`.
    struct non_invalid
    {
        template <typename T>
        constexpr bool operator()(const T& t) const noexcept(noexcept(!!t))
        {
            return !!t;
        }
    };

    /// A `Constraint` for the [ts::tagged_type]().
    ///
    /// It marks an owning pointer.
    /// It is borrowed from GSL's
    /// [non_null](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#a-namess-viewsagslview-views).
    /// \notes This is not actually a predicate.
    struct owner
    {};
} // namespace constraints
} // namespace type_safe

#endif // TYPE_SAFE_CONSTRAINED_TYPE_HPP_INCLUDED
