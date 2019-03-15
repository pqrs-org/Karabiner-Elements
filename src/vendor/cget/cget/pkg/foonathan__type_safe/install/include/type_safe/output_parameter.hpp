// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_OUTPUT_PARAMETER_HPP_INCLUDED
#define TYPE_SAFE_OUTPUT_PARAMETER_HPP_INCLUDED

#include <new>

#include <type_safe/deferred_construction.hpp>
#include <type_safe/detail/assert.hpp>

namespace type_safe
{
/// A tiny wrapper modelling an output parameter of a function.
///
/// An output parameter is a parameter that will be used to transport output of a function to its
/// caller, like a return value does. Usually they are implemented with lvalue references. They have
/// a couple of disadvantages though:
/// * They require an already created object, i.e. a default constructor or similar.
/// * It is not obvious when just looking at the call that the argument will be changed.
/// * They make it easy to accidentally use them as regular parameters, i.e. reading their value,
/// even though that was not intended.
///
/// If you use this class as your output parameter type,
/// you do not have these disadvantages.
/// The creation is explicit, you cannot read the value,
/// and it works with [ts::deferred_construction]().
///
/// \notes While you could use this class in other locations besides parameters,
/// this is not recommended.
template <typename T>
class output_parameter
{
public:
    using parameter_type = T;

    //=== constructors/assignment ===//
    /// \effects Creates it from an lvalue reference.
    /// All output will be assigned to the object referred by the reference.
    /// \requires The referred object must live as long as the function has not returned.
    explicit output_parameter(T& obj) noexcept : ptr_(&obj), is_normal_ptr_(true) {}

    /// \group delete_val
    output_parameter(const T&) = delete;
    /// \group delete_val
    output_parameter(T&&) = delete;
    /// \group delete_val
    output_parameter(const T&&) = delete;

    /// \effects Creates it from a [ts::deferred_construction]() object.
    /// All output will be assigned or created in the storage of the defer construction object,
    /// depending on wheter it is initialized.
    /// \requires The referred object must live as long as the function has not returned.
    explicit output_parameter(deferred_construction<T>& out) noexcept
    {
        if (out.has_value())
        {
            is_normal_ptr_ = true;
            ptr_           = &out.value();
        }
        else
        {
            is_normal_ptr_ = false;
            ptr_           = &out;
        }
    }

    /// \group delete_deferred
    output_parameter(const deferred_construction<T>&) = delete;
    /// \group delete_deferred
    output_parameter(deferred_construction<T>&&) = delete;
    /// \group delete_deferred
    output_parameter(const deferred_construction<T>&&) = delete;

    /// \effects Moves an output parameter.
    /// This will put `other` in an invalid state, it must not be used afterwards.
    /// \notes This constructor is only there because guaranteed copy elision isn't available
    /// and otherwise the `out()` function could not be implemented.
    /// It is not intended to use it otherwise.
    output_parameter(output_parameter&& other) noexcept
    : ptr_(other.ptr_), is_normal_ptr_(other.is_normal_ptr_)
    {
        other.ptr_ = nullptr;
    }

    ~output_parameter() noexcept = default;

    /// \group delete_assign
    output_parameter& operator=(const output_parameter&) = delete;
    /// \group delete_assign
    output_parameter& operator=(output_parameter&&) = delete;

    //=== modifiers ===//
    /// \effects Same as `assign(std::forward<U>(u))`.
    /// \returns A reference to the value that was assigned, *not* `*this` as normal "assignment"
    /// operators. \requires `value_type` must be constructible from `U`. \synopsis_return
    /// parameter_type&
    template <typename U>
    auto operator=(U&& u) ->
        typename std::enable_if<std::is_constructible<T, decltype(std::forward<U>(u))>::value,
                                parameter_type&>::type
    {
        return assign(std::forward<U>(u));
    }

    /// \effects If the output object is not already created, forwards `args` to the constructor.
    /// Otherwise if `args` is a single type and can be assigned to `T`, assigns it.
    /// Otherwise if `args` cannot be assigned directly, creates a temporary object and move assigns
    /// that. \returns A reference to the value that was assigned. \throws Anything thrown by the
    /// constructor for `T` or the chosen assignment operator. \requires `T` must be constructible
    /// from the arguments,
    // and `T` must be move-assignable.
    template <typename... Args>
    T& assign(Args&&... args)
    {
        if (is_normal_ptr_)
            assign_impl(std::forward<Args>(args)...);
        else
        {
            auto defer = static_cast<deferred_construction<T>*>(ptr_);
            defer->emplace(std::forward<Args>(args)...);
            ptr_           = &defer->value();
            is_normal_ptr_ = true;
        }

        DEBUG_ASSERT(is_normal_ptr_, detail::assert_handler{});
        return *static_cast<T*>(ptr_);
    }

private:
    template <typename U>
    auto assign_impl(U&& u) ->
        typename std::enable_if<std::is_assignable<T&, decltype(std::forward<U>(u))>::value>::type
    {
        *static_cast<T*>(ptr_) = std::forward<U>(u);
    }

    template <typename... Args>
    void assign_impl(Args&&... args)
    {
        *static_cast<T*>(ptr_) = T(std::forward<Args>(args)...);
    }

    void* ptr_;
    bool  is_normal_ptr_;
};

/// \returns A new [ts::output_parameter]() using the reference `obj` as output.
template <typename T>
output_parameter<T> out(T& obj) noexcept
{
    return output_parameter<T>(obj);
}

/// \returns A new [ts::output_parameter]() using the [ts::deferred_construction]() as output.
template <typename T>
output_parameter<T> out(deferred_construction<T>& o) noexcept
{
    return output_parameter<T>(o);
}
} // namespace type_safe

#endif // TYPE_SAFE_OUTPUT_PARAMETER_HPP_INCLUDED
