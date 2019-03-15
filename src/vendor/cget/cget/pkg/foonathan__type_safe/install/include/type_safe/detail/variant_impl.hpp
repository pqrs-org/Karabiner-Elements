// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_VARIANT_IMPL_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_VARIANT_IMPL_HPP_INCLUDED

#include <type_safe/detail/all_of.hpp>
#include <type_safe/detail/assign_or_construct.hpp>
#include <type_safe/detail/copy_move_control.hpp>
#include <type_safe/detail/is_nothrow_swappable.hpp>
#include <type_safe/optional.hpp>
#include <type_safe/tagged_union.hpp>

namespace type_safe
{
template <class VariantPolicy, typename Head, typename... Types>
class basic_variant;

namespace detail
{
    //=== variant traits ===//
    template <typename T>
    struct is_variant_impl : std::false_type
    {};

    template <class VariantPolicy, typename Head, typename... Types>
    struct is_variant_impl<basic_variant<VariantPolicy, Head, Types...>> : std::true_type
    {};

    template <typename T>
    using is_variant = is_variant_impl<typename std::decay<T>::type>;

    template <typename... Types>
    struct traits_impl
    {
        using copy_constructible = all_of<std::is_copy_constructible<Types>::value...>;
        using move_constructible = all_of<std::is_move_constructible<Types>::value...>;
        using nothrow_move_constructible
            = all_of<std::is_nothrow_move_constructible<Types>::value...>;
        using nothrow_move_assignable = all_of<nothrow_move_constructible::value,
                                               (std::is_move_assignable<Types>::value
                                                    ? std::is_nothrow_move_assignable<Types>::value
                                                    : true)...>;
        using nothrow_swappable
            = all_of<nothrow_move_constructible::value, is_nothrow_swappable<Types>::value...>;
    };

    template <typename... Types>
    using traits = traits_impl<typename std::decay<Types>::type...>;

    //=== copy_assign_union_value ===//
    template <class VariantPolicy, class Union>
    struct copy_assign_union_value
    {
        struct visitor
        {
            template <typename T,
                      typename = typename std::enable_if<std::is_copy_assignable<T>::value>::type>
            void do_assign(Union& dest, const T& value)
            {
                dest.value(union_type<T>{}) = value;
            }

            template <typename T,
                      typename std::enable_if<!std::is_copy_assignable<T>::value, int>::type = 0>
            void do_assign(Union& dest, const T& value)
            {
                VariantPolicy::change_value(union_type<T>{}, dest, value);
            }

            template <typename T>
            void operator()(const T& value, Union& dest)
            {
                constexpr auto id = typename Union::type_id(union_type<T>{});
                if (dest.type() == id)
                    do_assign(dest, value);
                else
                    VariantPolicy::change_value(union_type<T>{}, dest, value);
            }
        };

        static void assign(Union& dest, const Union& org)
        {
            with(org, visitor{}, dest);
        }
    };

    //==== move_assign_union_value ===//
    template <class VariantPolicy, class Union>
    struct move_assign_union_value
    {
        struct visitor
        {
            template <typename T,
                      typename = typename std::enable_if<std::is_move_assignable<T>::value>::type>
            void do_assign(Union& dest, T&& value)
            {
                dest.value(union_type<T>{}) = std::move(value);
            }

            template <typename T,
                      typename std::enable_if<!std::is_move_assignable<T>::value, int>::type = 0>
            void do_assign(Union& dest, T&& value)
            {
                VariantPolicy::change_value(union_type<typename std::decay<T>::type>{}, dest,
                                            std::move(value));
            }

            template <typename T>
            void operator()(T&& value, Union& dest)
            {
                constexpr auto id =
                    typename Union::type_id(union_type<typename std::decay<T>::type>{});
                if (dest.type() == id)
                    do_assign(dest, std::move(value));
                else
                    VariantPolicy::change_value(union_type<typename std::decay<T>::type>{}, dest,
                                                std::move(value));
            }
        };

        static void assign(Union& dest, Union&& org)
        {
            with(std::move(org), visitor{}, dest);
        }
    };

    //=== swap_union ===//
    template <class VariantPolicy, class Union>
    struct swap_union
    {
        struct visitor
        {
            template <typename T>
            void operator()(T&, Union& a, Union& b)
            {
                constexpr auto id = typename Union::type_id(union_type<T>{});
                DEBUG_ASSERT(a.type() == id, detail::assert_handler{});

                if (b.type() == id)
                {
                    using std::swap;
                    swap(a.value(union_type<T>{}), b.value(union_type<T>{}));
                }
                else
                {
                    T tmp(std::move(a).value(union_type<T>{})); // save old value from a
                    // assign a to value in b
                    move_assign_union_value<VariantPolicy, Union>::assign(a, std::move(b));
                    // change value in b to tmp
                    VariantPolicy::change_value(union_type<T>{}, b, std::move(tmp));
                }
            }
        };

        static void swap(Union& a, Union& b)
        {
            with(a, visitor{}, a, b);
        }
    };

    //=== map_union ===//
    template <typename Functor, class Union>
    struct map_union
    {
        struct visitor
        {
            template <typename T, typename... Args>
            auto call(int, Union& res, Functor&& f, T&& value, Args&&... args)
                -> decltype((void)map_invoke(std::forward<Functor>(f), std::forward<T>(value),
                                             std::forward<Args>(args)...))
            {
                using result = decltype(map_invoke(std::forward<Functor>(f), std::forward<T>(value),
                                                   std::forward<Args>(args)...));
                res.emplace(union_type<typename std::decay<result>::type>{},
                            map_invoke(std::forward<Functor>(f), std::forward<T>(value),
                                       std::forward<Args>(args)...));
            }
            template <typename T, typename... Args>
            void call(short, Union& res, Functor&&, T&& value, Args&&...)
            {
                res.emplace(union_type<typename std::decay<T>::type>{}, std::forward<T>(value));
            }

            template <typename T, typename... Args>
            void operator()(T&& value, Union& res, Functor&& f, Args&&... args)
            {
                call(0, res, std::forward<Functor>(f), std::forward<T>(value),
                     std::forward<Args>(args)...);
            }
        };

        template <typename... Args>
        static void map(Union& res, const Union& u, Functor&& f, Args&&... args)
        {
            DEBUG_ASSERT(!res.has_value(), precondition_error_handler{});
            with(u, visitor{}, res, std::forward<Functor>(f), std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void map(Union& res, Union&& u, Functor&& f, Args&&... args)
        {
            DEBUG_ASSERT(!res.has_value(), precondition_error_handler{});
            with(std::move(u), visitor{}, res, std::forward<Functor>(f),
                 std::forward<Args>(args)...);
        }
    };

    //=== compare_variant ===//
    template <class Variant>
    struct compare_variant
    {
        struct equal_visitor
        {
            bool result = false;

            template <typename T>
            void operator()(const T& value, const Variant& other)
            {
                result = (value == other);
            }
        };

        struct less_visitor
        {
            bool result = false;

            template <typename T>
            void operator()(const T& value, const Variant& other)
            {
                result = (value < other);
            }
        };

        static bool compare_equal(const Variant& a, const Variant& b)
        {
            if (!a.has_value())
                // to be equal, b must not have value as well
                return !b.has_value();

            equal_visitor v;
            with(a, v, b);
            return v.result;
        }

        static bool compare_less(const Variant& a, const Variant& b)
        {
            if (!a.has_value())
                // for a to be less than b,
                // b must have a value
                return b.has_value();

            less_visitor v;
            with(a, v, b);
            return v.result;
        }
    };

    //=== variant_storage ===//
    template <class VariantPolicy, typename... Types>
    class variant_storage
    {
        using traits = detail::traits<Types...>;

    public:
        variant_storage() noexcept = default;

        variant_storage(const variant_storage& other)
        {
            copy(storage_, other.storage_);
        }

        variant_storage(variant_storage&& other) noexcept(traits::nothrow_move_constructible::value)
        {
            move(storage_, std::move(other.storage_));
        }

        ~variant_storage() noexcept
        {
            destroy(storage_);
        }

        variant_storage& operator=(const variant_storage& other)
        {
            if (storage_.has_value() && other.storage_.has_value())
                copy_assign_union_value<VariantPolicy,
                                        tagged_union<Types...>>::assign(storage_, other.storage_);
            else if (storage_.has_value() && !other.storage_.has_value())
                destroy(storage_);
            else if (!storage_.has_value() && other.storage_.has_value())
                copy(storage_, other.storage_);

            return *this;
        }

        variant_storage& operator=(variant_storage&& other) noexcept(
            traits::nothrow_move_assignable::value)
        {
            if (storage_.has_value() && other.storage_.has_value())
                move_assign_union_value<VariantPolicy,
                                        tagged_union<Types...>>::assign(storage_,
                                                                        std::move(other.storage_));
            else if (storage_.has_value() && !other.storage_.has_value())
                destroy(storage_);
            else if (!storage_.has_value() && other.storage_.has_value())
                move(storage_, std::move(other.storage_));

            return *this;
        }

        tagged_union<Types...>& get_union() noexcept
        {
            return storage_;
        }

        const tagged_union<Types...>& get_union() const noexcept
        {
            return storage_;
        }

    private:
        tagged_union<Types...> storage_;
    };

    struct storage_access
    {
        template <class Variant>
        static auto get(Variant& var) -> decltype(var.storage_)&
        {
            return var.storage_;
        }

        template <class Variant>
        static auto get(const Variant& var) -> const decltype(var.storage_)&
        {
            return var.storage_;
        }
    };

    template <typename... Types>
    using variant_copy = copy_control<traits<Types...>::copy_constructible::value>;

    template <typename... Types>
    using variant_move = move_control<traits<Types...>::move_constructible::value>;

    template <class Union, typename T, typename... Args>
    using enable_variant_type_impl =
        typename std::enable_if<Union::type_id::template is_valid<T>()
                                && std::is_constructible<T, Args...>::value>::type;

    template <class Union, typename T, typename... Args>
    using enable_variant_type
        = enable_variant_type_impl<Union, typename std::decay<T>::type, Args...>;
} // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_DETAIL_VARIANT_IMPL_HPP_INCLUDED
