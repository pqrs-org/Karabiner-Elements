// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DEFERRED_CONSTRUCTION_HPP_INCLUDED
#define TYPE_SAFE_DEFERRED_CONSTRUCTION_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#include <new>
#include <type_traits>
#include <utility>
#endif

#include <type_safe/detail/assert.hpp>

namespace type_safe
{
/// A tiny wrapper to create an object without constructing it yet.
///
/// This is useful if you have a type that is default constructible,
/// but can't be initialized properly - yet.
/// It works especially well with [ts::output_parameter]().
///
/// It has two states:
/// Either it is *initialized* in which case you can get its value,
/// or it is *un-initialized* in which case you cannot get its value.
/// All objects start out un-initialized.
/// For consistency with [ts::basic_optional]() it provides a similar interface,
/// yet it is not as flexible and does not allow to reset it to the uninitialized state,
/// once initialized.
template <typename T>
class deferred_construction
{
public:
    using value_type = T;

    //=== constructors/assignment/destructor ===//
    /// Default constructor.
    /// \effects Creates it in the un-initialized state.
    deferred_construction() noexcept : initialized_(false) {}

    /// Copy constructor:
    /// \effects If `other` is un-initialized, it will be un-initialized as well.
    /// If `other` is initialized, it will copy the stored value.
    /// \throws Anything thrown by the copy constructor of `value_type` if `other` is initialized.
    deferred_construction(const deferred_construction& other) : initialized_(other.initialized_)
    {
        if (initialized_)
            ::new (as_void()) value_type(other.value());
    }

    /// Move constructor:
    /// \effects If `other` is un-initialized, it will be un-initialized as well.
    /// If `other` is initialized, it will copy the stored value.
    /// \throws Anything thrown by the move constructor of `value_type` if `other` is initialized.
    /// \notes `other` will still be initialized after the move operation,
    /// it is just in a moved-from state.
    deferred_construction(deferred_construction&& other) noexcept(
        std::is_nothrow_move_constructible<value_type>::value)
    : initialized_(other.initialized_)
    {
        if (initialized_)
            ::new (as_void()) value_type(std::move(other).value());
    }

    /// \notes You cannot construct it from the type directly.
    /// If you are able to do that, there is no need to use `defer_construction`!
    deferred_construction(value_type) = delete;

    /// \effects If it is initialized, it will destroy the value.
    /// Otherwise it has no effect.
    ~deferred_construction() noexcept
    {
        if (initialized_)
            value().~value_type();
    }

    /// \notes You cannot copy or move assign it.
    /// This is a deliberate design decision to guarantee,
    /// that an initialized object stays initialized, no matter what.
    deferred_construction& operator=(deferred_construction) = delete;

    /// \effects Same as `emplace(std::forward<U>(u))`.
    /// \requires `value_type` must be constructible from `U`.
    /// \notes You must not use this function to actually "assign" the value,
    /// like `emplace()`, the object must not be initialized.
    /// \synopsis_return deferred_construction&
    template <typename U>
    auto operator=(U&& u) ->
        typename std::enable_if<std::is_constructible<T, decltype(std::forward<U>(u))>::value,
                                deferred_construction&>::type
    {
        emplace(std::forward<U>(u));
        return *this;
    }

    //=== modifiers ===//
    /// \effects Initializes the object with the `value_type` constructed from `args`.
    /// \requires `has_value() == false`.
    /// \throws Anything thrown by the chosen constructor of `value_type`.
    /// \notes You must only call this function once,
    /// after the object has been initialized,
    /// you can use `value()` to assign to it.
    /// \output_section Modifiers
    template <typename... Args>
    void emplace(Args&&... args)
    {
        DEBUG_ASSERT(!has_value(), detail::precondition_error_handler{});
        ::new (as_void()) value_type(std::forward<Args>(args)...);
        initialized_ = true;
    }

    //=== observers ===//
    /// \returns `true` if the object is initialized, `false` otherwise.
    /// \output_section Observers
    bool has_value() const noexcept
    {
        return initialized_;
    }

    /// \returns The same as `has_value()`.
    explicit operator bool() const noexcept
    {
        return has_value();
    }

    /// Access the stored value.
    /// \returns A (`const`) (rvalue) reference to the stored value.
    /// \requires `has_value() == true`.
    /// \group value
    value_type& value() TYPE_SAFE_LVALUE_REF noexcept
    {
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{});
        return *static_cast<value_type*>(as_void());
    }

    /// \group value
    const value_type& value() const TYPE_SAFE_LVALUE_REF noexcept
    {
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{});
        return *static_cast<const value_type*>(as_void());
    }

#if TYPE_SAFE_USE_REF_QUALIFIERS
    /// \group value
    value_type&& value() && noexcept
    {
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{});
        return std::move(*static_cast<value_type*>(as_void()));
    }

    /// \group value
    const value_type&& value() const&& noexcept
    {
        DEBUG_ASSERT(has_value(), detail::precondition_error_handler{});
        return std::move(*static_cast<const value_type*>(as_void()));
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

    alignas(T) unsigned char storage_[sizeof(T)];
    bool      initialized_;
};

} // namespace type_safe

#endif // TYPE_SAFE_DEFER_CONSTRUCTION_HPP_INCLUDED
