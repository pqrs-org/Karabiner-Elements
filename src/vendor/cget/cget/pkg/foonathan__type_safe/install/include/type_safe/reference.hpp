// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_REFERENCE_HPP_INCLUDED
#define TYPE_SAFE_REFERENCE_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#    include <new>
#    include <type_traits>
#    include <utility>
#endif

#include <type_safe/detail/aligned_union.hpp>
#include <type_safe/detail/assert.hpp>
#include <type_safe/detail/map_invoke.hpp>
#include <type_safe/index.hpp>

namespace type_safe
{
template <typename T, bool XValue = false>
class object_ref;

/// \exclude
namespace detail
{
    template <typename T, bool XValue>
    struct rebind_object_ref_impl
    {
        using type = object_ref<T, XValue>;
    };

    template <typename T, bool XValue1, bool XValue2>
    struct rebind_object_ref_impl<object_ref<T, XValue1>, XValue2>
    {
        using type = object_ref<T, XValue2>;
    };

    template <typename T, bool XValue>
    using rebind_object_ref =
        typename rebind_object_ref_impl<typename std::remove_reference<T>::type, XValue>::type;
} // namespace detail

/// A reference to an object of some type `T`.
///
/// Unlike [std::reference_wrapper]() it does not try to model reference semantics,
/// instead it is basically a non-null pointer to a single object.
/// This allows rebinding on assignment.
/// Apart from the different access syntax it can be safely used instead of a reference,
/// and is safe for all kinds of containers.
///
/// If the given type is `const`, it will only return a `const` reference,
/// but then `XValue` must be `false`.
///
/// If `XValue` is `true`, dereferencing will [std::move()]() the object,
/// modelling a reference to an expiring lvalue.
/// \notes `T` is the type without the reference, ie. `object_ref<int>`.
template <typename T, bool XValue /* = false*/>
class object_ref
{
    static_assert(!std::is_void<T>::value, "must not be void");
    static_assert(!std::is_reference<T>::value, "pass the type without reference");
    static_assert(!XValue || !std::is_const<T>::value, "must not be const if xvalue reference");

    T* ptr_;

public:
    using value_type     = T;
    using reference_type = typename std::conditional<XValue, T&&, T&>::type;

    /// \effects Binds the reference to the given object.
    /// \notes This constructor will only participate in overload resolution
    /// if `U` is a compatible type (i.e. non-const variant or derived type).
    /// \group ctor_assign
    /// \param 1
    /// \exclude
    template <typename U, typename = decltype(std::declval<T*&>() = std::declval<U*>())>
    explicit constexpr object_ref(U& obj) noexcept : ptr_(&obj)
    {}

    /// \group ctor_assign
    /// \param 1
    /// \exclude
    template <typename U, typename = decltype(std::declval<T*&>() = std::declval<U*>())>
    constexpr object_ref(const object_ref<U>& obj) noexcept : ptr_(&*obj)
    {}

    /// \group ctor_assign
    /// \param 1
    /// \exclude
    template <typename U, typename = decltype(std::declval<T*&>() = std::declval<U*>())>
    object_ref& operator=(const object_ref<U>& obj) noexcept
    {
        ptr_ = &*obj;
        return *this;
    }

    /// \returns A native reference to the referenced object.
    /// if `XValue` is true, this will be an rvalue reference,
    /// else an lvalue reference.
    /// \group deref
    constexpr reference_type get() const noexcept
    {
        return **this;
    }

    /// \group deref
    constexpr reference_type operator*() const noexcept
    {
        return static_cast<reference_type>(*ptr_);
    }

    /// Member access operator.
    constexpr T* operator->() const noexcept
    {
        return ptr_;
    }

    /// \effects Invokes the function with the referred object followed by the arguments.
    /// \returns A [ts::object_ref]() to the result of the function,
    /// if `*this` is an xvalue reference, the result is as well.
    /// \requires The function must return an lvalue or another [ts::object_ref]() object.
    template <typename Func, typename... Args>
    auto map(Func&& f, Args&&... args)
        -> detail::rebind_object_ref<decltype(detail::map_invoke(std::forward<Func>(f),
                                                                 std::declval<object_ref&>().get(),
                                                                 std::forward<Args>(args)...)),
                                     XValue>
    {
        using result = decltype(detail::map_invoke(std::forward<Func>(f), get(),
                                                   std::forward<Args>(args)...));
        return detail::rebind_object_ref<result, XValue>(
            detail::map_invoke(std::forward<Func>(f), get(), std::forward<Args>(args)...));
    }
};

template <typename T, bool XValue>
class reference_optional_storage;

template <typename T>
struct optional_storage_policy_for;

/// Sets the [ts::basic_optional]() storage policy for [ts::object_ref]() to
/// [ts::reference_optional_storage]().
///
/// It will be used when the optional is rebound.
/// \module optional
template <typename T, bool XValue>
struct optional_storage_policy_for<object_ref<T, XValue>>
{
    using type = reference_optional_storage<T, XValue>;
};

/// Comparison operator for [ts::object_ref]().
///
/// Two references are equal if both refer to the same object.
/// A reference is equal to an object if the reference refers to that object.
/// \notes These functions do not participate in overload resolution
/// if the types are not compatible (i.e. const/non-const or derived).
/// \group ref_compare Object reference comparison
/// \param 3
/// \exclude
template <typename T, typename U, bool XValue,
          typename = decltype(std::declval<T*>() == std::declval<U*>())>
constexpr bool operator==(const object_ref<T, XValue>& a, const object_ref<U, XValue>& b) noexcept
{
    return a.operator->() == b.operator->();
}

/// \group ref_compare
/// \param 3
/// \exclude
template <typename T, typename U, bool XValue,
          typename = decltype(std::declval<T*>() == std::declval<U*>())>
constexpr bool operator==(const object_ref<T, XValue>& a, U& b) noexcept
{
    return a.operator->() == &b;
}

/// \group ref_compare
/// \param 3
/// \exclude
template <typename T, typename U, bool XValue,
          typename = decltype(std::declval<T*>() == std::declval<U*>())>
constexpr bool operator==(const T& a, const object_ref<U, XValue>& b) noexcept
{
    return &a == b.operator->();
}

/// \group ref_compare
/// \param 3
/// \exclude
template <typename T, typename U, bool XValue,
          typename = decltype(std::declval<T*>() == std::declval<U*>())>
constexpr bool operator!=(const object_ref<T, XValue>& a, const object_ref<U, XValue>& b) noexcept
{
    return !(a == b);
}

/// \group ref_compare
/// \param 3
/// \exclude
template <typename T, typename U, bool XValue,
          typename = decltype(std::declval<T*>() == std::declval<U*>())>
constexpr bool operator!=(const object_ref<T, XValue>& a, U& b) noexcept
{
    return !(a == b);
}

/// \group ref_compare
/// \param 3
/// \exclude
template <typename T, typename U, bool XValue,
          typename = decltype(std::declval<T*>() == std::declval<U*>())>
constexpr bool operator!=(const T& a, const object_ref<U, XValue>& b) noexcept
{
    return !(a == b);
}

/// With operation for [ts::object_ref]().
/// \effects Calls the `operator()` of `f` passing it `*ref` and the additional arguments.
template <typename T, bool XValue, typename Func, typename... Args>
void with(const object_ref<T, XValue>& ref, Func&& f, Args&&... additional_args)
{
    std::forward<Func>(f)(*ref, std::forward<Args>(additional_args)...);
}

/// Creates a (const) [ts::object_ref]().
/// \returns A [ts::object_ref]() to the given object.
/// \group object_ref_ref
template <typename T>
constexpr object_ref<T> ref(T& obj) noexcept
{
    return object_ref<T>(obj);
}

/// \group object_ref_ref
template <typename T>
constexpr object_ref<const T> cref(const T& obj) noexcept
{
    return object_ref<const T>(obj);
}

/// Convenience alias of [ts::object_ref]() where `XValue` is `true`.
template <typename T>
using xvalue_ref = object_ref<T, true>;

/// Creates a [ts::xvalue_ref]().
/// \returns A [ts::xvalue_ref]() to the given object.
template <typename T>
constexpr xvalue_ref<T> xref(T& obj) noexcept
{
    return xvalue_ref<T>(obj);
}

/// \returns A new object containing a copy of the referenced object.
/// It will use the copy (1)/move constructor (2).
/// \throws Anything thrown by the copy (1)/move (2) constructor.
/// \group object_ref_copy
template <typename T>
constexpr typename std::remove_const<T>::type copy(const object_ref<T>& obj)
{
    return *obj;
}

/// \group object_ref_copy
template <typename T>
constexpr T move(const xvalue_ref<T>& obj) noexcept(std::is_nothrow_move_constructible<T>::value)
{
    return *obj;
}

/// A reference to an array of objects of type `T`.
///
/// It is a simple pointer + size pair that allows reference access to each element in the array.
/// An "array" here is any contiguous storage (so C arrays, [std::vector](), etc.).
/// It does not allow changing the size of the array, only the individual elements.
/// Like [ts::object_ref]() it can be safely used in containers.
///
/// If the given type is `const`, it will only return a `const` reference to each element,
/// but then `XValue` must be `false`.
///
/// If `XValue` is `true`, dereferencing will [std::move()]() the object,
/// modelling a reference to an expiring lvalue.
/// \notes `T` is the type stored in the array, so `array_ref<int>` to reference a contiguous
/// storage of `int`s. \notes Unlike the other types it isn't technically non-null, as it may
/// contain an empty array. But the range `[data(), data() + size)` will always be valid.
template <typename T, bool XValue = false>
class array_ref
{
    static_assert(!std::is_void<T>::value, "must not be void");
    static_assert(!std::is_reference<T>::value, "pass the type without reference");
    static_assert(!XValue || !std::is_const<T>::value, "must not be const if xvalue reference");

public:
    using value_type     = T;
    using reference_type = typename std::conditional<XValue, T&&, T&>::type;
    using iterator       = T*;

    /// \effects Sets the reference to an empty array.
    /// \group empty
    array_ref(std::nullptr_t) : begin_(nullptr), size_(0u) {}

    /// \effects Sets the reference to the memory range `[begin, end)`.
    /// \requires `begin <= end`.
    /// \group range
    array_ref(T* begin, T* end) noexcept : size_(0u)
    {
        assign(begin, end);
    }

    /// \effects Sets the reference to the memory range `[array, array + size)`.
    /// \requires `array` must not be `nullptr` unless `size` is `0`.
    /// \group ptr_size
    array_ref(T* array, size_t size) noexcept : size_(size)
    {
        assign(array, size);
    }

    /// \effects Sets the reference to the C array.
    /// \group c_array
    template <std::size_t Size>
    explicit array_ref(T (&arr)[Size]) : begin_(arr), size_(Size)
    {}

    /// \group empty
    void assign(std::nullptr_t) noexcept
    {
        begin_ = nullptr;
        size_  = 0u;
    }

    /// \group range
    void assign(T* begin, T* end) noexcept
    {
        DEBUG_ASSERT(begin <= end, detail::precondition_error_handler{}, "invalid array bounds");
        begin_ = begin;
        size_  = static_cast<size_t>(make_unsigned(end - begin));
    }

    /// \group ptr_size
    void assign(T* array, size_t size) noexcept
    {
        DEBUG_ASSERT(size == 0u || array, detail::precondition_error_handler{},
                     "invalid array bounds");
        begin_ = array;
        size_  = size;
    }

    /// \group c_array
    template <std::size_t Size>
    void assign(T (&arr)[Size]) noexcept
    {
        begin_ = arr;
        size_  = Size;
    }

    /// \returns An iterator to the beginning of the array.
    iterator begin() const noexcept
    {
        return begin_;
    }

    /// \returns An iterator one past the last element of the array.
    iterator end() const noexcept
    {
        return begin_ + static_cast<std::size_t>(size_);
    }

    /// \returns A pointer to the beginning of the array.
    /// If `size()` isn't zero, the pointer is guaranteed to be non-null.
    T* data() const noexcept
    {
        return begin_;
    }

    /// \returns The number of elements in the array.
    size_t size() const noexcept
    {
        return size_;
    }

    /// \returns A (`rvalue` if `Xvalue` is `true`) reference to the `i`th element of the array.
    /// \requires `i < size()`.
    reference_type operator[](index_t i) const noexcept
    {
        DEBUG_ASSERT(static_cast<size_t&>(i) < size_, detail::precondition_error_handler{},
                     "out of bounds array access");
        return static_cast<reference_type>(at(begin_, i));
    }

private:
    T*     begin_;
    size_t size_;
};

/// With operation for [ts::array_ref]().
/// \effects For every element of the array in order, it will invoke `f`, passing it the current
/// element and the additional arguments.
//// If `XValue` is `true`, it will pass an rvalue reference to the element, allowing it to be moved
/// from.
template <typename T, bool XValue, typename Func, typename... Args>
void with(const array_ref<T, XValue>& ref, Func&& f, Args&&... additional_args)
{
    for (auto&& elem : ref)
        f(std::forward<decltype(elem)>(elem), additional_args...);
}

/// Creates a [ts::array_ref]().
/// \returns The reference created by forwarding the parameter(s) to the constructor.
/// \group array_ref_ref
template <typename T, std::size_t Size>
array_ref<T> ref(T (&arr)[Size]) noexcept
{
    return array_ref<T>(arr);
}

/// \group array_ref_ref
template <typename T>
array_ref<T> ref(T* begin, T* end) noexcept
{
    return array_ref<T>(begin, end);
}

/// \group array_ref_ref
template <typename T>
array_ref<T> ref(T* array, size_t size) noexcept
{
    return array_ref<T>(array, size);
}

/// Creates a [ts::array_ref]() to `const`.
/// \returns The reference created by forwarding the parameter(s) to the constructor.
/// \group array_ref_cref
template <typename T, std::size_t Size>
array_ref<const T> cref(const T (&arr)[Size]) noexcept
{
    return array_ref<const T>(arr);
}

/// \group array_ref_cref
template <typename T>
array_ref<const T> cref(const T* begin, const T* end) noexcept
{
    return array_ref<const T>(begin, end);
}

/// \group array_ref_cref
template <typename T>
array_ref<const T> cref(const T* array, size_t size) noexcept
{
    return array_ref<const T>(array, size);
}

/// Convenience alias for [ts::array_ref]() where `XValue` is `true`.
template <typename T>
using array_xvalue_ref = array_ref<T, true>;

/// Creates a [ts::array_xvalue_ref]().
/// \returns The reference created by forwarding the parameter(s) to the constructor.
/// \group array_xvalue_ref_ref
template <typename T, std::size_t Size>
array_xvalue_ref<T> xref(T (&arr)[Size]) noexcept
{
    return array_xvalue_ref<T>(arr);
}

/// \group array_xvalue_ref_ref
template <typename T>
array_xvalue_ref<T> xref(T* begin, T* end) noexcept
{
    return array_xvalue_ref<T>(begin, end);
}

/// \group array_xvalue_ref_ref
template <typename T>
array_xvalue_ref<T> xref(T* array, size_t size) noexcept
{
    return array_xvalue_ref<T>(array, size);
}

/// \exclude
namespace detail
{
    template <typename Returned, typename Required>
    struct compatible_return_type
    : std::integral_constant<bool, std::is_void<Required>::value
                                       || std::is_convertible<Returned, Required>::value>
    {};

    struct matching_function_pointer_tag
    {};
    struct matching_functor_tag
    {};
    struct invalid_functor_tag
    {};

    template <typename Func, typename Return, typename... Args>
    struct get_callable_tag
    {
        // use unary + to convert to function pointer
        template <typename T,
                  typename Result = decltype((+std::declval<T&>())(std::declval<Args>()...))>
        static matching_function_pointer_tag test(
            int, T& obj,
            typename std::enable_if<compatible_return_type<Result, Return>::value, int>::type = 0);

        template <typename T,
                  typename Result = decltype(std::declval<T&>()(std::declval<Args>()...))>
        static matching_functor_tag test(
            short, T& obj,
            typename std::enable_if<compatible_return_type<Result, Return>::value, int>::type = 0);

        static invalid_functor_tag test(...);

        using type = decltype(test(0, std::declval<Func&>()));
    };

    template <typename Result, typename Func, typename Return, typename... Args>
    using enable_function_tag = typename std::enable_if<
        std::is_same<typename get_callable_tag<Func, Return, Args...>::type, Result>::value,
        int>::type;
} // namespace detail

template <typename Signature>
class function_ref;

namespace detail
{
    template<typename>
    struct function_ref_trait { using type = void; };

    template<typename Signature>
    struct function_ref_trait<function_ref<Signature>>
    {
        using type = function_ref<Signature>;

        using return_type = typename type::return_type;
    };
} // namespace detail

/// A reference to a function.
///
/// This is a lightweight reference to a function.
/// It can refer to any function that is compatible with given signature.
///
/// A function is compatible if it is callable with regular function call syntax from the given
/// argument types, and its return type is either implicitly convertible to the specified return
/// type or the specified return type is `void`.
///
/// In general it will store a pointer to the functor,
/// requiring an lvalue.
/// But if it is created with a function pointer or something convertible to a function pointer,
/// it will store the function pointer itself.
/// This allows creating it from stateless lambdas.
/// \notes Due to implementation reasons,
/// it does not support member function pointers,
/// as it requires regular function call syntax.
/// Create a reference to the object returned by [std::mem_fn](), if that is required.
template <typename Return, typename... Args>
class function_ref<Return(Args...)>
{
public:
    using return_type = Return;

    using signature = Return(Args...);

    /// \effects Creates a reference to the function specified by the function pointer.
    /// \requires `fptr` must not be `nullptr`.
    /// \notes (2) only participates in overload resolution if the type of the function is
    /// compatible with the specified signature. \group function_ptr_ctor
    function_ref(Return (*fptr)(Args...))
    : function_ref(detail::matching_function_pointer_tag{}, fptr)
    {}

    /// \group function_ptr_ctor
    /// \param 1
    /// \exclude
    template <typename Return2, typename... Args2>
    function_ref(
        Return2 (*fptr)(Args2...),
        typename std::enable_if<detail::compatible_return_type<Return2, Return>::value, int>::type
        = 0)
    : function_ref(detail::matching_function_pointer_tag{}, fptr)
    {}

    /// \effects Creates a reference to the function created by the stateless lambda.
    /// \notes This constructor is intended for stateless lambdas,
    /// which are implicitly convertible to function pointers.
    /// It does not participate in overload resolution,
    /// unless the type is implicitly convertible to a function pointer
    /// that is compatible with the specified signature.
    /// \notes Due to to implementation reasons,
    /// it does not work for polymorphic lambdas,
    /// it needs an explicit cast to the desired function pointer type.
    /// A polymorphic lambda convertible to a direct match function pointer,
    /// works however.
    /// \param 1
    /// \exclude
    template <typename Functor,
              typename = detail::enable_function_tag<detail::matching_function_pointer_tag, Functor,
                                                     Return, Args...>>
    function_ref(const Functor& f) : function_ref(detail::matching_function_pointer_tag{}, +f)
    {}

    /// \effects Creates a reference to the specified functor.
    /// It will store a pointer to the function object,
    /// so it must live as long as the reference.
    /// \notes This constructor does not participate in overload resolution,
    /// unless the functor is compatible with the specified signature.
    /// \param 1
    /// \exclude
    template <
        typename Functor,
        typename = detail::enable_function_tag<detail::matching_functor_tag, Functor, Return, Args...>,
        // This overload restricts us to not directly referencing another function_ref.
        typename std::enable_if<std::is_same<typename detail::function_ref_trait<Functor>::type, void>::value, int>::type = 0
    >
    explicit function_ref(Functor& f) : cb_(&invoke_functor<Functor>)
    {
        // Ref to this functor
        ::new (storage_.get()) void*(&f);
    }

    /// Converting copy constructor.
    /// \effects Creates a reference to the same function referred by `other`.
    /// \notes This constructor does not participate in overload resolution,
    /// unless the signature of `other` is compatible with the specified signature.
    /// \notes This constructor may create a bigger conversion chain.
    /// For example, if `other` has signature `void(const char*)` it can refer to a function taking
    /// `std::string`. If this signature than accepts a type `T` implicitly convertible to `const
    /// char*`, calling this will call the function taking `std::string`, converting `T ->
    /// std::string`, even though such a conversion would be ill-formed otherwise. \param 1 \exclude
    template <
        typename Functor,
        // This overloading allows us to directly referencing another function_ref.
        typename std::enable_if<!std::is_same<typename detail::function_ref_trait<Functor>::type, void>::value, int>::type = 0,
        // Requires that the signature not be consistent (if it is then the copy construct should be called).
        typename std::enable_if<!std::is_same<typename detail::function_ref_trait<Functor>::type, function_ref>::value, int>::type = 0,
        // Of course, the return type and parameter types must be compatible.
        typename = detail::enable_function_tag<detail::matching_functor_tag, Functor, Return, Args...>
    >
    explicit function_ref(Functor& f) : cb_(&invoke_functor<Functor>)
    {
        // Ref to this function_ref
        ::new (storage_.get()) void*(&f);
    }

    /// \effects Rebinds the reference to the specified functor.
    /// \notes This assignment operator only participates in overload resolution,
    /// if the argument can also be a valid constructor argument.
    /// \param 1
    /// \exclude
    template <typename Functor,
              typename = typename std::enable_if<
                  !std::is_same<typename std::decay<Functor>::type, function_ref>::value,
                  decltype(function_ref(std::declval<Functor&&>()))>::type>
    void assign(Functor&& f) noexcept
    {
        auto ref = function_ref(std::forward<Functor>(f));
        storage_ = ref.storage_;
        cb_      = ref.cb_;
    }

    /// \effects Invokes the stored function with the specified arguments and returns the result.
    Return operator()(Args... args) const
    {
        return cb_(storage_.get(), static_cast<Args>(args)...);
    }

private:
    template <typename Functor>
    static Return invoke_functor(const void* memory, Args... args)
    {
        using ptr_t   = void*;
        ptr_t    ptr  = *static_cast<const ptr_t*>(memory);
        Functor& func = *static_cast<Functor*>(ptr);
        return static_cast<Return>(func(static_cast<Args>(args)...));
    }

    template <typename PointerT, typename StoredT>
    static Return invoke_function_pointer(const void* memory, Args... args)
    {
        auto ptr  = *static_cast<const StoredT*>(memory);
        auto func = reinterpret_cast<PointerT>(ptr);
        return static_cast<Return>(func(static_cast<Args>(args)...));
    }

    template <typename Return2, typename... Args2>
    function_ref(detail::matching_function_pointer_tag, Return2 (*fptr)(Args2...))
    {
        using pointer_type        = Return2 (*)(Args2...);
        using stored_pointer_type = void (*)();

        DEBUG_ASSERT(fptr, detail::precondition_error_handler{},
                     "function pointer must not be null");
        ::new (storage_.get()) stored_pointer_type(reinterpret_cast<stored_pointer_type>(fptr));

        cb_ = &invoke_function_pointer<pointer_type, stored_pointer_type>;
    }

    using storage  = detail::aligned_union<void*, Return (*)(Args...)>;
    using callback = Return (*)(const void*, Args...);

    storage  storage_;
    callback cb_;
};
} // namespace type_safe

#endif // TYPE_SAFE_REFERENCE_HPP_INCLUDED
