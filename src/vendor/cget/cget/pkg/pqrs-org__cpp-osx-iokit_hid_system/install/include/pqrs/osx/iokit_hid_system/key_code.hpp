#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <frozen/unordered_map.h>
#include <optional>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs {
namespace osx {
namespace iokit_hid_system {
namespace key_code {
// NXEventData.key.keyCode is uint16_t.
struct value_t : type_safe::strong_typedef<value_t, uint16_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;
};

//
// values
//

constexpr value_t keyboard_a(0x0);
constexpr value_t keyboard_b(0xb);
constexpr value_t keyboard_c(0x8);
constexpr value_t keyboard_d(0x2);
constexpr value_t keyboard_e(0xe);
constexpr value_t keyboard_f(0x3);
constexpr value_t keyboard_g(0x5);
constexpr value_t keyboard_h(0x4);
constexpr value_t keyboard_i(0x22);
constexpr value_t keyboard_j(0x26);
constexpr value_t keyboard_k(0x28);
constexpr value_t keyboard_l(0x25);
constexpr value_t keyboard_m(0x2e);
constexpr value_t keyboard_n(0x2d);
constexpr value_t keyboard_o(0x1f);
constexpr value_t keyboard_p(0x23);
constexpr value_t keyboard_q(0xc);
constexpr value_t keyboard_r(0xf);
constexpr value_t keyboard_s(0x1);
constexpr value_t keyboard_t(0x11);
constexpr value_t keyboard_u(0x20);
constexpr value_t keyboard_v(0x9);
constexpr value_t keyboard_w(0xd);
constexpr value_t keyboard_x(0x7);
constexpr value_t keyboard_y(0x10);
constexpr value_t keyboard_z(0x6);
constexpr value_t keyboard_1(0x12);
constexpr value_t keyboard_2(0x13);
constexpr value_t keyboard_3(0x14);
constexpr value_t keyboard_4(0x15);
constexpr value_t keyboard_5(0x17);
constexpr value_t keyboard_6(0x16);
constexpr value_t keyboard_7(0x1a);
constexpr value_t keyboard_8(0x1c);
constexpr value_t keyboard_9(0x19);
constexpr value_t keyboard_0(0x1d);
constexpr value_t keyboard_return_or_enter(0x24);
constexpr value_t keyboard_escape(0x35);
constexpr value_t keyboard_delete_or_backspace(0x33);
constexpr value_t keyboard_tab(0x30);
constexpr value_t keyboard_spacebar(0x31);
constexpr value_t keyboard_hyphen(0x1b);
constexpr value_t keyboard_equal_sign(0x18);
constexpr value_t keyboard_open_bracket(0x21);
constexpr value_t keyboard_close_bracket(0x1e);
constexpr value_t keyboard_backslash(0x2a);
constexpr value_t keyboard_non_us_pound(0x2a); // non_us_pound == backslash
constexpr value_t keyboard_semicolon(0x29);
constexpr value_t keyboard_quote(0x27);
constexpr value_t keyboard_grave_accent_and_tilde(0x32);
constexpr value_t keyboard_comma(0x2b);
constexpr value_t keyboard_period(0x2f);
constexpr value_t keyboard_slash(0x2c);
constexpr value_t keyboard_caps_lock(0x39);
constexpr value_t keyboard_f1(0x7a);
constexpr value_t keyboard_f2(0x78);
constexpr value_t keyboard_f3(0x63);
constexpr value_t keyboard_f4(0x76);
constexpr value_t keyboard_f5(0x60);
constexpr value_t keyboard_f6(0x61);
constexpr value_t keyboard_f7(0x62);
constexpr value_t keyboard_f8(0x64);
constexpr value_t keyboard_f9(0x65);
constexpr value_t keyboard_f10(0x6d);
constexpr value_t keyboard_f11(0x67);
constexpr value_t keyboard_f12(0x6f);
constexpr value_t keyboard_print_screen(0x69);
constexpr value_t keyboard_scroll_lock(0x6b);
constexpr value_t keyboard_pause(0x71);
constexpr value_t keyboard_insert(0x72);
constexpr value_t keyboard_home(0x73);
constexpr value_t keyboard_page_up(0x74);
constexpr value_t keyboard_delete_forward(0x75);
constexpr value_t keyboard_end(0x77);
constexpr value_t keyboard_page_down(0x79);
constexpr value_t keyboard_right_arrow(0x7c);
constexpr value_t keyboard_left_arrow(0x7b);
constexpr value_t keyboard_down_arrow(0x7d);
constexpr value_t keyboard_up_arrow(0x7e);
constexpr value_t keypad_num_lock(0x47);
constexpr value_t keypad_slash(0x4b);
constexpr value_t keypad_asterisk(0x43);
constexpr value_t keypad_hyphen(0x4e);
constexpr value_t keypad_plus(0x45);
constexpr value_t keypad_enter(0x4c);
constexpr value_t keypad_1(0x53);
constexpr value_t keypad_2(0x54);
constexpr value_t keypad_3(0x55);
constexpr value_t keypad_4(0x56);
constexpr value_t keypad_5(0x57);
constexpr value_t keypad_6(0x58);
constexpr value_t keypad_7(0x59);
constexpr value_t keypad_8(0x5b);
constexpr value_t keypad_9(0x5c);
constexpr value_t keypad_0(0x52);
constexpr value_t keypad_period(0x41);
constexpr value_t keyboard_non_us_backslash(0xa);
constexpr value_t keyboard_application(0x6e);
// keyboard_power => aux_control_button
constexpr value_t keypad_equal_sign(0x51);
constexpr value_t keyboard_f13(0x69);
constexpr value_t keyboard_f14(0x6b);
constexpr value_t keyboard_f15(0x71);
constexpr value_t keyboard_f16(0x6a);
constexpr value_t keyboard_f17(0x40);
constexpr value_t keyboard_f18(0x4f);
constexpr value_t keyboard_f19(0x50);
constexpr value_t keyboard_f20(0x5a);
// keyboard_f21
// keyboard_f22
// keyboard_f23
// keyboard_f24
// keyboard_execute
constexpr value_t keyboard_help(0x72);
// keyboard_menu
// keyboard_select
// keyboard_stop
// keyboard_again
// keyboard_undo
// keyboard_cut
// keyboard_copy
// keyboard_paste
// keyboard_find
// keyboard_mute => aux_control_button
// keyboard_volume_up => aux_control_button
// keyboard_volume_down => aux_control_button
// keyboard_locking_caps_lock
// keyboard_locking_num_lock
// keyboard_locking_scroll_lock
constexpr value_t keypad_comma(0x5f);
// keypad_equal_sign_as400
constexpr value_t keyboard_international1(0x5e);
// keyboard_international2
constexpr value_t keyboard_international3(0x5d);
// keyboard_international4
// keyboard_international5
// keyboard_international6
// keyboard_international7
// keyboard_international8
// keyboard_international9
constexpr value_t keyboard_lang1(0x68);
constexpr value_t keyboard_lang2(0x66);
// keyboard_lang3
// keyboard_lang4
// keyboard_lang5
// keyboard_lang6
// keyboard_lang7
// keyboard_lang8
// keyboard_lang9
// keyboard_alternate_erase
// keyboard_sys_req_or_attention
// keyboard_cancel
// keyboard_clear
// keyboard_prior
// keyboard_return
// keyboard_separator
// keyboard_out
// keyboard_oper
// keyboard_clear_or_again
// keyboard_cr_sel_or_props
// keyboard_ex_sel

constexpr value_t keyboard_left_control(0x3b);
constexpr value_t keyboard_left_shift(0x38);
constexpr value_t keyboard_left_alt(0x3a);
constexpr value_t keyboard_left_gui(0x37);
constexpr value_t keyboard_right_control(0x3e);
constexpr value_t keyboard_right_shift(0x3c);
constexpr value_t keyboard_right_alt(0x3d);
constexpr value_t keyboard_right_gui(0x36);

constexpr value_t apple_vendor_keyboard_dashboard(0x82);
constexpr value_t apple_vendor_keyboard_function(0x3f);
constexpr value_t apple_vendor_keyboard_launchpad(0x83);
constexpr value_t apple_vendor_keyboard_expose_all(0xa0);

constexpr value_t apple_vendor_top_case_keyboard_fn(0x3f); // apple_vendor_top_case_keyboard_fn == apple_vendor_keyboard_function

//
// make_key_code
//

namespace impl {
#define PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(name) \
  { type_safe::get(hid::usage::keyboard_or_keypad::name), name }

constexpr std::pair<type_safe::underlying_type<hid::usage::value_t>, value_t> usage_page_keyboard_or_keypad_pairs[] = {
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_a),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_b),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_c),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_d),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_e),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_g),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_h),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_i),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_j),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_k),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_l),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_m),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_n),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_o),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_p),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_q),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_r),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_s),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_t),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_u),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_v),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_w),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_x),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_y),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_z),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_1),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_2),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_3),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_4),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_5),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_6),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_7),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_8),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_9),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_0),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_return_or_enter),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_escape),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_delete_or_backspace),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_tab),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_spacebar),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_hyphen),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_equal_sign),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_open_bracket),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_close_bracket),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_backslash),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_non_us_pound),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_semicolon),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_quote),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_grave_accent_and_tilde),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_comma),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_period),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_slash),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_caps_lock),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f1),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f2),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f3),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f4),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f5),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f6),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f7),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f8),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f9),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f10),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f11),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f12),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_print_screen),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_scroll_lock),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_pause),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_insert),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_home),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_page_up),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_delete_forward),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_end),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_page_down),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_right_arrow),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_left_arrow),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_down_arrow),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_up_arrow),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_num_lock),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_slash),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_asterisk),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_hyphen),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_plus),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_enter),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_1),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_2),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_3),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_4),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_5),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_6),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_7),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_8),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_9),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_0),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_period),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_non_us_backslash),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_application),
    // keyboard_power
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_equal_sign),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f13),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f14),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f15),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f16),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f17),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f18),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f19),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_f20),
    // keyboard_f21
    // keyboard_f22
    // keyboard_f23
    // keyboard_f24
    // keyboard_execute
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_help),
    // keyboard_menu
    // keyboard_select
    // keyboard_stop
    // keyboard_again
    // keyboard_undo
    // keyboard_cut
    // keyboard_copy
    // keyboard_paste
    // keyboard_find
    // keyboard_mute
    // keyboard_volume_up
    // keyboard_volume_down
    // keyboard_locking_caps_lock
    // keyboard_locking_num_lock
    // keyboard_locking_scroll_lock
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keypad_comma),
    // keypad_equal_sign_as400
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_international1),
    // keyboard_international2
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_international3),
    // keyboard_international4
    // keyboard_international5
    // keyboard_international6
    // keyboard_international7
    // keyboard_international8
    // keyboard_international9
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_lang1),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_lang2),
    // keyboard_lang3
    // keyboard_lang4
    // keyboard_lang5
    // keyboard_lang6
    // keyboard_lang7
    // keyboard_lang8
    // keyboard_lang9
    // keyboard_alternate_erase
    // keyboard_sys_req_or_attention
    // keyboard_cancel
    // keyboard_clear
    // keyboard_prior
    // keyboard_return
    // keyboard_separator
    // keyboard_out
    // keyboard_oper
    // keyboard_clear_or_again
    // keyboard_cr_sel_or_props
    // keyboard_ex_sel

    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_left_control),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_left_shift),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_left_alt),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_left_gui),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_right_control),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_right_shift),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_right_alt),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_right_gui),
};

#undef PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR

#define PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(name) \
  { type_safe::get(hid::usage::apple_vendor_keyboard::name), apple_vendor_keyboard_##name }

constexpr std::pair<type_safe::underlying_type<hid::usage::value_t>, value_t> usage_page_apple_vendor_keyboard_pairs[] = {
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(dashboard),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(function),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(launchpad),
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(expose_all),
};

#undef PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR

#define PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(name) \
  { type_safe::get(hid::usage::apple_vendor_top_case::name), apple_vendor_top_case_##name }

constexpr std::pair<type_safe::underlying_type<hid::usage::value_t>, value_t> usage_page_apple_vendor_top_case_pairs[] = {
    PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR(keyboard_fn),
};

#undef PQRS_OSX_IOKIT_HID_SYSTEM_KEY_CODE_PAIR

constexpr auto usage_page_keyboard_or_keypad_map = frozen::make_unordered_map(usage_page_keyboard_or_keypad_pairs);
constexpr auto usage_page_apple_vendor_keyboard_map = frozen::make_unordered_map(usage_page_apple_vendor_keyboard_pairs);
constexpr auto usage_page_apple_vendor_top_case_map = frozen::make_unordered_map(usage_page_apple_vendor_top_case_pairs);

template <typename T>
inline std::optional<key_code::value_t> find(T& map, hid::usage::value_t usage) {
  auto it = map.find(type_safe::get(usage));
  if (it != std::end(map)) {
    return it->second;
  }
  return std::nullopt;
}
} // namespace impl
} // namespace key_code

inline std::optional<key_code::value_t> make_key_code(hid::usage_page::value_t usage_page, hid::usage::value_t usage) {
  if (usage_page == hid::usage_page::keyboard_or_keypad) {
    return key_code::impl::find(key_code::impl::usage_page_keyboard_or_keypad_map, usage);
  } else if (usage_page == hid::usage_page::apple_vendor_keyboard) {
    return key_code::impl::find(key_code::impl::usage_page_apple_vendor_keyboard_map, usage);
  } else if (usage_page == hid::usage_page::apple_vendor_top_case) {
    return key_code::impl::find(key_code::impl::usage_page_apple_vendor_top_case_map, usage);
  }

  return std::nullopt;
}
} // namespace iokit_hid_system
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_hid_system::key_code::value_t> : type_safe::hashable<pqrs::osx::iokit_hid_system::key_code::value_t> {
};
} // namespace std
