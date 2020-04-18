#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hidsystem/IOLLEvent.h>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace osx {
namespace iokit_hid_system {
namespace event_type {
struct value_t : type_safe::strong_typedef<value_t, uint32_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;
};

//
// values from IOKit/hidsystem/IOLLEvent.h
//

constexpr value_t key_down(NX_KEYDOWN);
constexpr value_t key_up(NX_KEYUP);
constexpr value_t flags_changed(NX_FLAGSCHANGED);
constexpr value_t system_defined(NX_SYSDEFINED);
} // namespace event_type
} // namespace iokit_hid_system
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_hid_system::event_type::value_t> : type_safe::hashable<pqrs::osx::iokit_hid_system::event_type::value_t> {
};
} // namespace std
