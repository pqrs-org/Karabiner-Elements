#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <compare>
#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace hid {
namespace country_code {
struct value_t : type_safe::strong_typedef<value_t, uint64_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;

  constexpr auto operator<=>(const value_t& other) const {
    return type_safe::get(*this) <=> type_safe::get(other);
  }
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}

//
// Values from Device Class Definition for Human Interface Devices (HID) Version 1.11.
// https://www.usb.org/sites/default/files/documents/hid1_11.pdf
//

constexpr value_t not_supported(0);
constexpr value_t arabic(1);
constexpr value_t belgian(2);
constexpr value_t canadian_bilingual(3);
constexpr value_t canadian_french(4);
constexpr value_t czech_republic(5);
constexpr value_t danish(6);
constexpr value_t finnish(7);
constexpr value_t french(8);
constexpr value_t german(9);
constexpr value_t greek(10);
constexpr value_t hebrew(11);
constexpr value_t hungary(12);
constexpr value_t international(13);
constexpr value_t italian(14);
constexpr value_t japan(15);
constexpr value_t korean(16);
constexpr value_t latin_american(17);
constexpr value_t netherlands_dutch(18);
constexpr value_t norwegian(19);
constexpr value_t persian(20);
constexpr value_t poland(21);
constexpr value_t portuguese(22);
constexpr value_t russia(23);
constexpr value_t slovakia(24);
constexpr value_t spanish(25);
constexpr value_t swedish(26);
constexpr value_t swiss_french(27);
constexpr value_t swiss_german(28);
constexpr value_t switzerland(29);
constexpr value_t taiwan(30);
constexpr value_t turkish_q(31);
constexpr value_t uk(32);
constexpr value_t us(33);
constexpr value_t yugoslavia(34);
constexpr value_t turkish_f(35);
} // namespace country_code
} // namespace hid
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::hid::country_code::value_t> : type_safe::hashable<pqrs::hid::country_code::value_t> {
};
} // namespace std
