// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_VISITOR_HPP_INCLUDED
#define TYPE_SAFE_VISITOR_HPP_INCLUDED

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;
#else
#include <utility>
#endif

#include <type_safe/optional.hpp>
#include <type_safe/variant.hpp>

namespace type_safe
{
/// \exclude
namespace detail
{
    // common type where void is wildcard
    template <typename A, typename B>
    struct common_type
    {
        using type = typename std::conditional<std::is_same<A, B>::value, A,
                                               typename std::common_type<A, B>::type>::type;
    };

    template <typename A>
    struct common_type<A, void>
    {
        using type = A;
    };

    template <typename A>
    struct common_type<void, A>
    {
        using type = A;
    };

    template <>
    struct common_type<void, void>
    {
        using type = void;
    };

    template <typename A, typename B>
    using common_type_t = typename common_type<A, B>::type;

    //=== visit optional ===//
    template <bool Save, typename Visitor, typename... Optional>
    struct visit_optional;

    template <bool Save, typename Visitor, typename Optional>
    struct visit_optional<Save, Visitor, Optional>
    {
        template <typename... Args>
        static auto call_visitor(int, Visitor&& visitor, Args&&... args)
            -> decltype(std::forward<Visitor>(visitor)(std::forward<Args>(args)...))
        {
            return std::forward<Visitor>(visitor)(std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void call_visitor(short, Visitor&&, Args&&...)
        {
            static_assert(!Save, "call to visitor does not cover all possible combinations");
        }

        template <typename... Args>
        static auto call(Visitor&& visitor, Optional&& opt, Args&&... args) ->
            typename common_type<decltype(call_visitor(0, std::forward<Visitor>(visitor),
                                                       std::forward<Args>(args)..., nullopt)),
                                 decltype(call_visitor(0, std::forward<Visitor>(visitor),
                                                       std::forward<Args>(args)...,
                                                       std::forward<Optional>(opt).value()))>::type
        {
            return opt.has_value() ? call_visitor(0, std::forward<Visitor>(visitor),
                                                  std::forward<Args>(args)...,
                                                  std::forward<Optional>(opt).value())
                                   : call_visitor(0, std::forward<Visitor>(visitor),
                                                  std::forward<Args>(args)..., nullopt);
        }
    };

    template <bool Save, typename Visitor, typename Optional, typename... Rest>
    struct visit_optional<Save, Visitor, Optional, Rest...>
    {
        template <typename... Args>
        static auto call(Visitor&& visitor, Optional&& opt, Rest&&... rest, Args&&... args) ->
            typename common_type<decltype(visit_optional<Save, Visitor, Rest...>::call(
                                     std::forward<Visitor>(visitor), std::forward<Rest>(rest)...,
                                     std::forward<Args>(args)...,
                                     std::forward<Optional>(opt).value())),
                                 decltype(visit_optional<Save, Visitor, Rest...>::call(
                                     std::forward<Visitor>(visitor), std::forward<Rest>(rest)...,
                                     std::forward<Args>(args)..., nullopt))>::type
        {
            return opt.has_value()
                       ? visit_optional<Save, Visitor,
                                        Rest...>::call(std::forward<Visitor>(visitor),
                                                       std::forward<Rest>(rest)...,
                                                       std::forward<Args>(args)...,
                                                       std::forward<Optional>(opt).value())
                       : visit_optional<Save, Visitor, Rest...>::call(std::forward<Visitor>(
                                                                          visitor),
                                                                      std::forward<Rest>(rest)...,
                                                                      std::forward<Args>(args)...,
                                                                      nullopt);
        }
    };

    template <typename T>
    struct visitor_allow_incomplete
    {
        template <typename U, typename = typename U::incomplete_visitor>
        static std::true_type test(int);

        template <typename U>
        static std::true_type test(int, decltype(U::incomplete_visitor));

        template <typename U>
        static std::false_type test(short);

        static const bool value = decltype(test<typename std::decay<T>::type>(0))::value;
    };

    template <typename Visitor, class... Optionals>
    auto visit_optional_impl(Visitor&& visitor, Optionals&&... optionals) -> decltype(
        detail::visit_optional<
            !visitor_allow_incomplete<Visitor>::value, decltype(std::forward<Visitor>(visitor)),
            decltype(std::forward<Optionals>(optionals))...>::call(std::forward<Visitor>(visitor),
                                                                   std::forward<Optionals>(
                                                                       optionals)...))
    {
        return detail::visit_optional<
            !visitor_allow_incomplete<Visitor>::value, decltype(std::forward<Visitor>(visitor)),
            decltype(std::forward<Optionals>(optionals))...>::call(std::forward<Visitor>(visitor),
                                                                   std::forward<Optionals>(
                                                                       optionals)...);
    }
} // namespace detail

/// Visits a [ts::basic_optional]().
/// \effects Effectively calls `visitor((optionals.has_value() ? optionals.value() : nullopt)...)`,
/// i.e. the `operator()` of `visitor` passing it `sizeof...(Optionals)` arguments,
/// where the `i`th argument is the `value()` of the `i`th optional or `nullopt`, if it has none.
/// If the particular combination of types is not overloaded,
/// the program is ill-formed,
/// unless the `Visitor` provides a member named `incomplete_visitor`,
/// then `visit()` does not do anything instead of the error.
/// \returns The result of the chosen `operator()`,
/// its the type is the common type of all possible combinations.
/// \module optional
/// \exclude return
/// \param 2
/// \exclude
template <typename Visitor, class... Optionals,
          typename std::enable_if<detail::all_of<detail::is_optional<Optionals>::value...>::value,
                                  int>::type
          = 0>
auto visit(Visitor&& visitor, Optionals&&... optionals)
    -> decltype(detail::visit_optional_impl(std::forward<Visitor>(visitor),
                                            std::forward<Optionals>(optionals)...))
{
    return detail::visit_optional_impl(std::forward<Visitor>(visitor),
                                       std::forward<Optionals>(optionals)...);
}

/// \exclude
namespace detail
{
    template <bool AllowIncomplete, typename Visitor, class... Variants>
    class visit_variant_impl;

    template <bool AllowIncomplete, typename Visitor>
    class visit_variant_impl<AllowIncomplete, Visitor>
    {
        template <typename... Args>
        static auto call_impl(int, Visitor&& visitor, Args&&... args)
            -> decltype(std::forward<Visitor>(visitor)(std::forward<Args>(args)...))
        {
            return std::forward<Visitor>(visitor)(std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void call_impl(short, Visitor&&, Args&&...)
        {
            static_assert(AllowIncomplete, "visitor does not cover all possible combinations");
        }

    public:
        template <typename... Args>
        static auto call(Visitor&& visitor, Args&&... args)
            -> decltype(call_impl(0, std::forward<Visitor>(visitor), std::forward<Args>(args)...))
        {
            return call_impl(0, std::forward<Visitor>(visitor), std::forward<Args>(args)...);
        }
    };

    // should not be called
    template <typename Variant, typename Head, typename... Types>
    Head get_dummy_type(const Variant& var, variant_types<Head, Types...>)
    {
        return var.value(variant_type<Head>{});
    }

    template <typename Variant>
    auto get_dummy_type(const Variant& var)
        -> decltype(get_dummy_type(var, typename Variant::types{}))
    {
        return get_dummy_type(var, typename Variant::types{});
    }

    template <bool AllowIncomplete, typename Visitor, class Variant>
    class visit_variant_impl<AllowIncomplete, Visitor, Variant>
    {
        template <typename, typename... Args,
                  typename Variant2 = typename std::decay<Variant>::type>
        static auto call_type(Visitor&& visitor, variant_types<>, Variant&& variant, Args&&... args)
            -> typename std::enable_if<Variant2::allow_empty::value,
                                       decltype(visit_variant_impl<AllowIncomplete, Visitor>::call(
                                           std::forward<Visitor>(visitor),
                                           std::forward<Args>(args)..., nullvar))>::type
        {
            DEBUG_ASSERT(!variant.has_value(), assert_handler{},
                         "it has a value but we are in this overload?!");
            return visit_variant_impl<AllowIncomplete, Visitor>::call(std::forward<Visitor>(
                                                                          visitor),
                                                                      std::forward<Args>(args)...,
                                                                      nullvar);
        }

        template <typename, typename... Args,
                  typename Variant2 = typename std::decay<Variant>::type>
        static auto call_type(Visitor&& visitor, variant_types<>, Variant&& variant, Args&&... args)
            ->
            typename std::enable_if<!Variant2::allow_empty::value,
                                    decltype(visit_variant_impl<AllowIncomplete, Visitor>::call(
                                        std::forward<Visitor>(visitor), std::forward<Args>(args)...,
                                        get_dummy_type(variant)))>::type
        {
            DEBUG_ASSERT(!variant.has_value(), assert_handler{},
                         "it has a value but we are in this overload?!");
            DEBUG_UNREACHABLE(precondition_error_handler{}, "variant in invalid state for visit");
            return visit_variant_impl<AllowIncomplete, Visitor>::call(std::forward<Visitor>(
                                                                          visitor),
                                                                      std::forward<Args>(args)...,
                                                                      get_dummy_type(variant));
        }

        template <typename RecursiveDecltype, typename Head, typename... Tail, typename... Args>
        static auto call_type(Visitor&& visitor, variant_types<Head, Tail...>, Variant&& variant,
                              Args&&... args)
            -> common_type_t<decltype(visit_variant_impl<AllowIncomplete, Visitor>::call(
                                 std::forward<Visitor>(visitor), std::forward<Args>(args)...,
                                 std::forward<Variant>(variant).value(variant_type<Head>{}))),
                             decltype(RecursiveDecltype::template call_type<RecursiveDecltype>(
                                 std::forward<Visitor>(visitor), variant_types<Tail...>{},
                                 std::forward<Variant>(variant), std::forward<Args>(args)...))>
        {
            if (variant.has_value(variant_type<Head>{}))
                return visit_variant_impl<AllowIncomplete,
                                          Visitor>::call(std::forward<Visitor>(visitor),
                                                         std::forward<Args>(args)...,
                                                         std::forward<Variant>(variant).value(
                                                             variant_type<Head>{}));
            else
                return RecursiveDecltype::template call_type<
                    RecursiveDecltype>(std::forward<Visitor>(visitor), variant_types<Tail...>{},
                                       std::forward<Variant>(variant), std::forward<Args>(args)...);
        }

    public:
        template <typename... Args>
        static auto call(Visitor&& visitor, Variant&& variant, Args&&... args)
            -> decltype(call_type<visit_variant_impl>(std::forward<Visitor>(visitor),
                                                      typename std::decay<Variant>::type::types{},
                                                      std::forward<Variant>(variant),
                                                      std::forward<Args>(args)...))
        {
            return visit_variant_impl::call_type<
                visit_variant_impl>(std::forward<Visitor>(visitor),
                                    typename std::decay<Variant>::type::types{},
                                    std::forward<Variant>(variant), std::forward<Args>(args)...);
        }
    };

    template <bool AllowIncomplete, typename Visitor, class Variant, class... Rest>
    class visit_variant_impl<AllowIncomplete, Visitor, Variant, Rest...>
    {
        template <typename, typename... Args,
                  typename Variant2 = typename std::decay<Variant>::type>
        static auto call_type(Visitor&& visitor, variant_types<>, Variant&& variant, Rest&&... rest,
                              Args&&... args) ->
            typename std::enable_if<
                Variant2::allow_empty::value,
                decltype(visit_variant_impl<AllowIncomplete, Visitor, Rest...>::call(
                    std::forward<Visitor>(visitor), std::forward<Rest>(rest)...,
                    std::forward<Args>(args)..., nullvar))>::type
        {
            DEBUG_ASSERT(std::decay<Variant>::type::allow_empty::value && !variant.has_value(),
                         precondition_error_handler{}, "variant in invalid state for visitor");
            return visit_variant_impl<AllowIncomplete, Visitor,
                                      Rest...>::call(std::forward<Visitor>(visitor),
                                                     std::forward<Rest>(rest)...,
                                                     std::forward<Args>(args)..., nullvar);
        }

        template <typename, typename... Args,
                  typename Variant2 = typename std::decay<Variant>::type>
        static auto call_type(Visitor&& visitor, variant_types<>, Variant&& variant, Rest&&... rest,
                              Args&&... args) ->
            typename std::enable_if<
                !Variant2::allow_empty::value,
                decltype(visit_variant_impl<AllowIncomplete, Visitor, Rest...>::call(
                    std::forward<Visitor>(visitor), std::forward<Rest>(rest)...,
                    std::forward<Args>(args)..., get_dummy_type(variant)))>::type
        {
            DEBUG_ASSERT(!variant.has_value(), assert_handler{},
                         "it has a value but we are in this overload?!");
            DEBUG_UNREACHABLE(precondition_error_handler{}, "variant in invalid state for visit");
            return visit_variant_impl<AllowIncomplete, Visitor,
                                      Rest...>::call(std::forward<Visitor>(visitor),
                                                     std::forward<Rest>(rest)...,
                                                     std::forward<Args>(args)...,
                                                     get_dummy_type(variant));
        }

        template <typename RecursiveDecltype, typename Head, typename... Tail, typename... Args>
        static auto call_type(Visitor&& visitor, variant_types<Head, Tail...>, Variant&& variant,
                              Rest&&... rest, Args&&... args)
            -> common_type_t<decltype(visit_variant_impl<AllowIncomplete, Visitor, Rest...>::call(
                                 std::forward<Visitor>(visitor), std::forward<Rest>(rest)...,
                                 std::forward<Args>(args)...,
                                 std::forward<Variant>(variant).value(variant_type<Head>{}))),
                             decltype(RecursiveDecltype::template call_type<RecursiveDecltype>(
                                 std::forward<Visitor>(visitor), variant_types<Tail...>{},
                                 std::forward<Variant>(variant), std::forward<Rest>(rest)...,
                                 std::forward<Args>(args)...))>
        {
            if (variant.has_value(variant_type<Head>{}))
                return visit_variant_impl<AllowIncomplete, Visitor,
                                          Rest...>::call(std::forward<Visitor>(visitor),
                                                         std::forward<Rest>(rest)...,
                                                         std::forward<Args>(args)...,
                                                         std::forward<Variant>(variant).value(
                                                             variant_type<Head>{}));
            else
                return RecursiveDecltype::template call_type<
                    RecursiveDecltype>(std::forward<Visitor>(visitor), variant_types<Tail...>{},
                                       std::forward<Variant>(variant), std::forward<Rest>(rest)...,
                                       std::forward<Args>(args)...);
        }

    public:
        template <typename... Args>
        static auto call(Visitor&& visitor, Variant&& variant, Rest&&... rest, Args&&... args)
            -> decltype(call_type<visit_variant_impl>(std::forward<Visitor>(visitor),
                                                      typename std::decay<Variant>::type::types{},
                                                      std::forward<Variant>(variant),
                                                      std::forward<Rest>(rest)...,
                                                      std::forward<Args>(args)...))
        {
            return visit_variant_impl::call_type<
                visit_variant_impl>(std::forward<Visitor>(visitor),
                                    typename std::decay<Variant>::type::types{},
                                    std::forward<Variant>(variant), std::forward<Rest>(rest)...,
                                    std::forward<Args>(args)...);
        }
    };

    template <class Visitor, class... Variants>
    auto visit_variant(Visitor&& visitor, Variants&&... variants)
        -> decltype(visit_variant_impl<visitor_allow_incomplete<Visitor>::value, Visitor&&,
                                       Variants&&...>::call(std::forward<Visitor>(visitor),
                                                            std::forward<Variants>(variants)...))
    {
        return visit_variant_impl<visitor_allow_incomplete<Visitor>::value, Visitor&&,
                                  Variants&&...>::call(std::forward<Visitor>(visitor),
                                                       std::forward<Variants>(variants)...);
    }
} // namespace detail

/// Visits a [ts::basic_variant]().
/// \effects Effectively calls `visitor(variants.value(variant_type<Ts>{})...)`,
/// where `Ts...` are the types of the currently active element in the variant,
/// i.e. it calls the `operator()` of the `visitor` where the `i`th argument is the currently stored
/// value in the `i`th variant, perfectly forwarded. If the `i`th variant is empty and it allows the
/// empty state, it passes `nullvar` as parameter, otherwise the behavior is undefined. If the
/// particular combination of types is not overloaded, the program is ill-formed, unless the
/// `Visitor` provides a member named `incomplete_visitor`, then `visit()` does not do anything
/// instead of the error. \returns The result of the chosen `operator()`, its the type is the common
/// type of all possible combinations. \exclude return \module variant
template <class Visitor, class... Variants,
          typename = typename std::enable_if<
              detail::all_of<detail::is_variant<Variants>::value...>::value>::type>
auto visit(Visitor&& visitor, Variants&&... variants)
    -> decltype(detail::visit_variant(std::forward<Visitor>(visitor),
                                      std::forward<Variants>(variants)...))
{
    return detail::visit_variant(std::forward<Visitor>(visitor),
                                 std::forward<Variants>(variants)...);
}
} // namespace type_safe

#endif // TYPE_SAFE_VISITOR_HPP_INCLUDED
