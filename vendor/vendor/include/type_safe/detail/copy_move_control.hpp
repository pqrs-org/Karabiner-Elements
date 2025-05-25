// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_COPY_MOVE_CONTROL_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_COPY_MOVE_CONTROL_HPP_INCLUDED

namespace type_safe
{
namespace detail
{
    template <bool AllowCopy>
    struct copy_control;

    template <>
    struct copy_control<true>
    {
        copy_control() noexcept = default;

        copy_control(const copy_control&) noexcept = default;
        copy_control& operator=(const copy_control&) noexcept = default;

        copy_control(copy_control&&) noexcept = default;
        copy_control& operator=(copy_control&&) noexcept = default;
    };

    template <>
    struct copy_control<false>
    {
        copy_control() noexcept = default;

        copy_control(const copy_control&) noexcept = delete;
        copy_control& operator=(const copy_control&) noexcept = delete;

        copy_control(copy_control&&) noexcept = default;
        copy_control& operator=(copy_control&&) noexcept = default;
    };

    template <bool AllowCopy>
    struct move_control;

    template <>
    struct move_control<true>
    {
        move_control() noexcept = default;

        move_control(const move_control&) noexcept = default;
        move_control& operator=(const move_control&) noexcept = default;

        move_control(move_control&&) noexcept = default;
        move_control& operator=(move_control&&) noexcept = default;
    };

    template <>
    struct move_control<false>
    {
        move_control() noexcept = default;

        move_control(const move_control&) noexcept = default;
        move_control& operator=(const move_control&) noexcept = default;

        move_control(move_control&&) noexcept = delete;
        move_control& operator=(move_control&&) noexcept = delete;
    };
} // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_DETAIL_COPY_MOVE_CONTROL_HPP_INCLUDED
