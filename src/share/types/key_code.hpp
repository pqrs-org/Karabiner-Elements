#pragma once

#include "modifier_flag.hpp"
#include <pqrs/osx/iokit_hid_value.hpp>

namespace krbn {
namespace key_code {
struct value_t : type_safe::strong_typedef<value_t, uint32_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t>,
                 type_safe::strong_typedef_op::integer_arithmetic<value_t> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}

//
// Values
//

// pqrs::hid::usage::keyboard_or_keypad

constexpr value_t keyboard_a(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
constexpr value_t keyboard_b(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_b));
constexpr value_t keyboard_c(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_c));
constexpr value_t keyboard_d(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_d));
constexpr value_t keyboard_e(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_e));
constexpr value_t keyboard_f(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f));
constexpr value_t keyboard_g(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_g));
constexpr value_t keyboard_h(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_h));
constexpr value_t keyboard_i(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_i));
constexpr value_t keyboard_j(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_j));
constexpr value_t keyboard_k(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_k));
constexpr value_t keyboard_l(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_l));
constexpr value_t keyboard_m(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_m));
constexpr value_t keyboard_n(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_n));
constexpr value_t keyboard_o(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_o));
constexpr value_t keyboard_p(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_p));
constexpr value_t keyboard_q(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_q));
constexpr value_t keyboard_r(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_r));
constexpr value_t keyboard_s(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_s));
constexpr value_t keyboard_t(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_t));
constexpr value_t keyboard_u(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_u));
constexpr value_t keyboard_v(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_v));
constexpr value_t keyboard_w(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_w));
constexpr value_t keyboard_x(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_x));
constexpr value_t keyboard_y(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_y));
constexpr value_t keyboard_z(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_z));
constexpr value_t keyboard_1(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_1));
constexpr value_t keyboard_2(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_2));
constexpr value_t keyboard_3(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_3));
constexpr value_t keyboard_4(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_4));
constexpr value_t keyboard_5(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_5));
constexpr value_t keyboard_6(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_6));
constexpr value_t keyboard_7(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_7));
constexpr value_t keyboard_8(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_8));
constexpr value_t keyboard_9(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_9));
constexpr value_t keyboard_0(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_0));
constexpr value_t keyboard_return_or_enter(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_return_or_enter));
constexpr value_t keyboard_escape(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_escape));
constexpr value_t keyboard_delete_or_backspace(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_delete_or_backspace));
constexpr value_t keyboard_tab(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
constexpr value_t keyboard_spacebar(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
constexpr value_t keyboard_hyphen(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_hyphen));
constexpr value_t keyboard_equal_sign(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_equal_sign));
constexpr value_t keyboard_open_bracket(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_open_bracket));
constexpr value_t keyboard_close_bracket(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_close_bracket));
constexpr value_t keyboard_backslash(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_backslash));
constexpr value_t keyboard_non_us_pound(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_non_us_pound));
constexpr value_t keyboard_semicolon(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_semicolon));
constexpr value_t keyboard_quote(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_quote));
constexpr value_t keyboard_grave_accent_and_tilde(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_grave_accent_and_tilde));
constexpr value_t keyboard_comma(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_comma));
constexpr value_t keyboard_period(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_period));
constexpr value_t keyboard_slash(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_slash));
constexpr value_t keyboard_caps_lock(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock));
constexpr value_t keyboard_f1(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f1));
constexpr value_t keyboard_f2(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f2));
constexpr value_t keyboard_f3(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f3));
constexpr value_t keyboard_f4(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f4));
constexpr value_t keyboard_f5(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f5));
constexpr value_t keyboard_f6(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f6));
constexpr value_t keyboard_f7(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f7));
constexpr value_t keyboard_f8(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f8));
constexpr value_t keyboard_f9(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f9));
constexpr value_t keyboard_f10(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f10));
constexpr value_t keyboard_f11(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f11));
constexpr value_t keyboard_f12(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f12));
constexpr value_t keyboard_print_screen(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_print_screen));
constexpr value_t keyboard_scroll_lock(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_scroll_lock));
constexpr value_t keyboard_pause(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_pause));
constexpr value_t keyboard_insert(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_insert));
constexpr value_t keyboard_home(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_home));
constexpr value_t keyboard_page_up(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_page_up));
constexpr value_t keyboard_delete_forward(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_delete_forward));
constexpr value_t keyboard_end(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_end));
constexpr value_t keyboard_page_down(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_page_down));
constexpr value_t keyboard_right_arrow(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_right_arrow));
constexpr value_t keyboard_left_arrow(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_left_arrow));
constexpr value_t keyboard_down_arrow(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_down_arrow));
constexpr value_t keyboard_up_arrow(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_up_arrow));
constexpr value_t keypad_num_lock(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_num_lock));
constexpr value_t keypad_slash(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_slash));
constexpr value_t keypad_asterisk(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_asterisk));
constexpr value_t keypad_hyphen(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_hyphen));
constexpr value_t keypad_plus(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_plus));
constexpr value_t keypad_enter(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_enter));
constexpr value_t keypad_1(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_1));
constexpr value_t keypad_2(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_2));
constexpr value_t keypad_3(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_3));
constexpr value_t keypad_4(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_4));
constexpr value_t keypad_5(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_5));
constexpr value_t keypad_6(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_6));
constexpr value_t keypad_7(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_7));
constexpr value_t keypad_8(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_8));
constexpr value_t keypad_9(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_9));
constexpr value_t keypad_0(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_0));
constexpr value_t keypad_period(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_period));
constexpr value_t keyboard_non_us_backslash(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_non_us_backslash));
constexpr value_t keyboard_application(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_application));
constexpr value_t keyboard_power(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_power));
constexpr value_t keypad_equal_sign(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_equal_sign));
constexpr value_t keyboard_f13(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f13));
constexpr value_t keyboard_f14(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f14));
constexpr value_t keyboard_f15(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f15));
constexpr value_t keyboard_f16(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f16));
constexpr value_t keyboard_f17(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f17));
constexpr value_t keyboard_f18(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f18));
constexpr value_t keyboard_f19(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f19));
constexpr value_t keyboard_f20(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f20));
constexpr value_t keyboard_f21(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f21));
constexpr value_t keyboard_f22(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f22));
constexpr value_t keyboard_f23(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f23));
constexpr value_t keyboard_f24(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_f24));
constexpr value_t keyboard_execute(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_execute));
constexpr value_t keyboard_help(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_help));
constexpr value_t keyboard_menu(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_menu));
constexpr value_t keyboard_select(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_select));
constexpr value_t keyboard_stop(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_stop));
constexpr value_t keyboard_again(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_again));
constexpr value_t keyboard_undo(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_undo));
constexpr value_t keyboard_cut(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_cut));
constexpr value_t keyboard_copy(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_copy));
constexpr value_t keyboard_paste(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_paste));
constexpr value_t keyboard_find(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_find));
constexpr value_t keyboard_mute(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_mute));
constexpr value_t keyboard_volume_up(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_volume_up));
constexpr value_t keyboard_volume_down(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_volume_down));
constexpr value_t keyboard_locking_caps_lock(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_locking_caps_lock));
constexpr value_t keyboard_locking_num_lock(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_locking_num_lock));
constexpr value_t keyboard_locking_scroll_lock(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_locking_scroll_lock));
constexpr value_t keypad_comma(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_comma));
constexpr value_t keypad_equal_sign_as400(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keypad_equal_sign_as400));
constexpr value_t keyboard_international1(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international1));
constexpr value_t keyboard_international2(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international2));
constexpr value_t keyboard_international3(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international3));
constexpr value_t keyboard_international4(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international4));
constexpr value_t keyboard_international5(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international5));
constexpr value_t keyboard_international6(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international6));
constexpr value_t keyboard_international7(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international7));
constexpr value_t keyboard_international8(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international8));
constexpr value_t keyboard_international9(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_international9));
constexpr value_t keyboard_lang1(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang1));
constexpr value_t keyboard_lang2(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang2));
constexpr value_t keyboard_lang3(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang3));
constexpr value_t keyboard_lang4(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang4));
constexpr value_t keyboard_lang5(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang5));
constexpr value_t keyboard_lang6(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang6));
constexpr value_t keyboard_lang7(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang7));
constexpr value_t keyboard_lang8(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang8));
constexpr value_t keyboard_lang9(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_lang9));
constexpr value_t keyboard_alternate_erase(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_alternate_erase));
constexpr value_t keyboard_sys_req_or_attention(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_sys_req_or_attention));
constexpr value_t keyboard_cancel(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_cancel));
constexpr value_t keyboard_clear(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_clear));
constexpr value_t keyboard_prior(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_prior));
constexpr value_t keyboard_return(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_return));
constexpr value_t keyboard_separator(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_separator));
constexpr value_t keyboard_out(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_out));
constexpr value_t keyboard_oper(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_oper));
constexpr value_t keyboard_clear_or_again(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_clear_or_again));
constexpr value_t keyboard_cr_sel_or_props(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_cr_sel_or_props));
constexpr value_t keyboard_ex_sel(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_ex_sel));

constexpr value_t keyboard_left_control(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control));
constexpr value_t keyboard_left_shift(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift));
constexpr value_t keyboard_left_option(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt));
constexpr value_t keyboard_left_command(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui));
constexpr value_t keyboard_right_control(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control));
constexpr value_t keyboard_right_shift(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift));
constexpr value_t keyboard_right_option(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt));
constexpr value_t keyboard_right_command(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui));

// usage in keyboard_or_keypad usage page is reserved until 0xffff.

// 0x10000 - are karabiner own virtual key codes or keys not in keyboard_or_keypad usage page.
constexpr value_t extra_(0x10000);
// A pseudo key that does not send any event.
constexpr value_t vk_none(0x10001);

// Keys that are not in generic keyboard_or_keypad usage_page.
constexpr value_t fn(0x10002);
constexpr value_t display_brightness_decrement(0x10003);
constexpr value_t display_brightness_increment(0x10004);
constexpr value_t dashboard(0x10005);
constexpr value_t launchpad(0x10006);
constexpr value_t mission_control(0x10007);
constexpr value_t illumination_decrement(0x10008);
constexpr value_t illumination_increment(0x10009);
constexpr value_t rewind(0x1000a);
constexpr value_t play_or_pause(0x1000b);
constexpr value_t fastforward(0x1000c);
constexpr value_t eject(0x1000d);
constexpr value_t apple_display_brightness_decrement(0x1000e);
constexpr value_t apple_display_brightness_increment(0x1000f);
constexpr value_t apple_top_case_display_brightness_decrement(0x10010);
constexpr value_t apple_top_case_display_brightness_increment(0x10011);

namespace impl {
constexpr std::pair<const mapbox::eternal::string, const value_t> name_value_pairs[] = {
    // Aliases
    {"left_option", keyboard_left_option},
    {"left_command", keyboard_left_command},
    {"right_option", keyboard_right_option},
    {"right_command", keyboard_right_command},
    {"japanese_eisuu", keyboard_lang2},
    {"japanese_kana", keyboard_lang1},
    {"japanese_pc_nfer", keyboard_international5},
    {"japanese_pc_xfer", keyboard_international4},
    {"japanese_pc_katakana", keyboard_international2},

    {"a", keyboard_a},
    {"b", keyboard_b},
    {"c", keyboard_c},
    {"d", keyboard_d},
    {"e", keyboard_e},
    {"f", keyboard_f},
    {"g", keyboard_g},
    {"h", keyboard_h},
    {"i", keyboard_i},
    {"j", keyboard_j},
    {"k", keyboard_k},
    {"l", keyboard_l},
    {"m", keyboard_m},
    {"n", keyboard_n},
    {"o", keyboard_o},
    {"p", keyboard_p},
    {"q", keyboard_q},
    {"r", keyboard_r},
    {"s", keyboard_s},
    {"t", keyboard_t},
    {"u", keyboard_u},
    {"v", keyboard_v},
    {"w", keyboard_w},
    {"x", keyboard_x},
    {"y", keyboard_y},
    {"z", keyboard_z},
    {"1", keyboard_1},
    {"2", keyboard_2},
    {"3", keyboard_3},
    {"4", keyboard_4},
    {"5", keyboard_5},
    {"6", keyboard_6},
    {"7", keyboard_7},
    {"8", keyboard_8},
    {"9", keyboard_9},
    {"0", keyboard_0},
    {"return_or_enter", keyboard_return_or_enter},
    {"escape", keyboard_escape},
    {"delete_or_backspace", keyboard_delete_or_backspace},
    {"tab", keyboard_tab},
    {"spacebar", keyboard_spacebar},
    {"hyphen", keyboard_hyphen},
    {"equal_sign", keyboard_equal_sign},
    {"open_bracket", keyboard_open_bracket},
    {"close_bracket", keyboard_close_bracket},
    {"backslash", keyboard_backslash},
    {"non_us_pound", keyboard_non_us_pound},
    {"semicolon", keyboard_semicolon},
    {"quote", keyboard_quote},
    {"grave_accent_and_tilde", keyboard_grave_accent_and_tilde},
    {"comma", keyboard_comma},
    {"period", keyboard_period},
    {"slash", keyboard_slash},
    {"caps_lock", keyboard_caps_lock},
    {"f1", keyboard_f1},
    {"f2", keyboard_f2},
    {"f3", keyboard_f3},
    {"f4", keyboard_f4},
    {"f5", keyboard_f5},
    {"f6", keyboard_f6},
    {"f7", keyboard_f7},
    {"f8", keyboard_f8},
    {"f9", keyboard_f9},
    {"f10", keyboard_f10},
    {"f11", keyboard_f11},
    {"f12", keyboard_f12},
    {"print_screen", keyboard_print_screen},
    {"scroll_lock", keyboard_scroll_lock},
    {"pause", keyboard_pause},
    {"insert", keyboard_insert},
    {"home", keyboard_home},
    {"page_up", keyboard_page_up},
    {"delete_forward", keyboard_delete_forward},
    {"end", keyboard_end},
    {"page_down", keyboard_page_down},
    {"right_arrow", keyboard_right_arrow},
    {"left_arrow", keyboard_left_arrow},
    {"down_arrow", keyboard_down_arrow},
    {"up_arrow", keyboard_up_arrow},
    {"keypad_num_lock", keypad_num_lock},
    {"keypad_slash", keypad_slash},
    {"keypad_asterisk", keypad_asterisk},
    {"keypad_hyphen", keypad_hyphen},
    {"keypad_plus", keypad_plus},
    {"keypad_enter", keypad_enter},
    {"keypad_1", keypad_1},
    {"keypad_2", keypad_2},
    {"keypad_3", keypad_3},
    {"keypad_4", keypad_4},
    {"keypad_5", keypad_5},
    {"keypad_6", keypad_6},
    {"keypad_7", keypad_7},
    {"keypad_8", keypad_8},
    {"keypad_9", keypad_9},
    {"keypad_0", keypad_0},
    {"keypad_period", keypad_period},
    {"non_us_backslash", keyboard_non_us_backslash},
    {"application", keyboard_application},
    {"power", keyboard_power},
    {"keypad_equal_sign", keypad_equal_sign},
    {"f13", keyboard_f13},
    {"f14", keyboard_f14},
    {"f15", keyboard_f15},
    {"f16", keyboard_f16},
    {"f17", keyboard_f17},
    {"f18", keyboard_f18},
    {"f19", keyboard_f19},
    {"f20", keyboard_f20},
    {"f21", keyboard_f21},
    {"f22", keyboard_f22},
    {"f23", keyboard_f23},
    {"f24", keyboard_f24},
    {"execute", keyboard_execute},
    {"help", keyboard_help},
    {"menu", keyboard_menu},
    {"select", keyboard_select},
    {"stop", keyboard_stop},
    {"again", keyboard_again},
    {"undo", keyboard_undo},
    {"cut", keyboard_cut},
    {"copy", keyboard_copy},
    {"paste", keyboard_paste},
    {"find", keyboard_find},
    {"mute", keyboard_mute},
    {"volume_decrement", keyboard_volume_down},
    {"volume_increment", keyboard_volume_up},
    {"locking_caps_lock", keyboard_locking_caps_lock},
    {"locking_num_lock", keyboard_locking_num_lock},
    {"locking_scroll_lock", keyboard_locking_scroll_lock},
    {"keypad_comma", keypad_comma},
    {"keypad_equal_sign_as400", keypad_equal_sign_as400},
    {"international1", keyboard_international1},
    {"international2", keyboard_international2},
    {"international3", keyboard_international3},
    {"international4", keyboard_international4},
    {"international5", keyboard_international5},
    {"international6", keyboard_international6},
    {"international7", keyboard_international7},
    {"international8", keyboard_international8},
    {"international9", keyboard_international9},
    {"lang1", keyboard_lang1},
    {"lang2", keyboard_lang2},
    {"lang3", keyboard_lang3},
    {"lang4", keyboard_lang4},
    {"lang5", keyboard_lang5},
    {"lang6", keyboard_lang6},
    {"lang7", keyboard_lang7},
    {"lang8", keyboard_lang8},
    {"lang9", keyboard_lang9},
    {"alternate_erase", keyboard_alternate_erase},
    {"sys_req_or_attention", keyboard_sys_req_or_attention},
    {"cancel", keyboard_cancel},
    {"clear", keyboard_clear},
    {"prior", keyboard_prior},
    {"return", keyboard_return},
    {"separator", keyboard_separator},
    {"out", keyboard_out},
    {"oper", keyboard_oper},
    {"clear_or_again", keyboard_clear_or_again},
    {"cr_sel_or_props", keyboard_cr_sel_or_props},
    {"ex_sel", keyboard_ex_sel},
    {"left_control", keyboard_left_control},
    {"left_shift", keyboard_left_shift},
    {"left_alt", keyboard_left_option},
    {"left_gui", keyboard_left_command},
    {"right_control", keyboard_right_control},
    {"right_shift", keyboard_right_shift},
    {"right_alt", keyboard_right_option},
    {"right_gui", keyboard_right_command},

    // Extra
    {"vk_none", vk_none},

    {"fn", fn},
    {"display_brightness_decrement", display_brightness_decrement},
    {"display_brightness_increment", display_brightness_increment},
    {"mission_control", mission_control},
    {"launchpad", launchpad},
    {"dashboard", dashboard},
    {"illumination_decrement", illumination_decrement},
    {"illumination_increment", illumination_increment},
    {"rewind", rewind},
    {"play_or_pause", play_or_pause},
    {"fastforward", fastforward},
    {"eject", eject},
    {"apple_display_brightness_decrement", apple_display_brightness_decrement},
    {"apple_display_brightness_increment", apple_display_brightness_increment},
    {"apple_top_case_display_brightness_decrement", apple_top_case_display_brightness_decrement},
    {"apple_top_case_display_brightness_increment", apple_top_case_display_brightness_increment},

    // Aliases
    {"vk_consumer_brightness_down", display_brightness_decrement},
    {"vk_consumer_brightness_up", display_brightness_increment},
    {"vk_mission_control", mission_control},
    {"vk_launchpad", launchpad},
    {"vk_dashboard", dashboard},
    {"vk_consumer_illumination_down", illumination_decrement},
    {"vk_consumer_illumination_up", illumination_increment},
    {"vk_consumer_previous", rewind},
    {"vk_consumer_play", play_or_pause},
    {"vk_consumer_next", fastforward},
    {"volume_down", keyboard_volume_down},
    {"volume_up", keyboard_volume_up},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, value_t>(name_value_pairs);

constexpr auto value_usage_page_map = mapbox::eternal::map<value_t, pqrs::hid::usage_page::value_t>({
    // pqrs::hid::usage_page::apple_vendor_top_case
    {fn, pqrs::hid::usage_page::apple_vendor_top_case},
    {illumination_decrement, pqrs::hid::usage_page::apple_vendor_top_case},
    {illumination_increment, pqrs::hid::usage_page::apple_vendor_top_case},
    {apple_top_case_display_brightness_decrement, pqrs::hid::usage_page::apple_vendor_top_case},
    {apple_top_case_display_brightness_increment, pqrs::hid::usage_page::apple_vendor_top_case},

    // pqrs::hid::usage_page::apple_vendor_keyboard
    {dashboard, pqrs::hid::usage_page::apple_vendor_keyboard},
    {launchpad, pqrs::hid::usage_page::apple_vendor_keyboard},
    {mission_control, pqrs::hid::usage_page::apple_vendor_keyboard},
    {apple_display_brightness_decrement, pqrs::hid::usage_page::apple_vendor_keyboard},
    {apple_display_brightness_increment, pqrs::hid::usage_page::apple_vendor_keyboard},

    // pqrs::hid::usage_page::consumer
    {keyboard_mute, pqrs::hid::usage_page::consumer},
    {keyboard_volume_down, pqrs::hid::usage_page::consumer},
    {keyboard_volume_up, pqrs::hid::usage_page::consumer},
    {display_brightness_decrement, pqrs::hid::usage_page::consumer},
    {display_brightness_increment, pqrs::hid::usage_page::consumer},
    {rewind, pqrs::hid::usage_page::consumer},
    {play_or_pause, pqrs::hid::usage_page::consumer},
    {fastforward, pqrs::hid::usage_page::consumer},
    {eject, pqrs::hid::usage_page::consumer},
});

constexpr auto value_usage_map = mapbox::eternal::map<value_t, pqrs::hid::usage::value_t>({
    // pqrs::hid::usage::apple_vendor_top_case
    {fn, pqrs::hid::usage::apple_vendor_top_case::keyboard_fn},
    {illumination_decrement, pqrs::hid::usage::apple_vendor_top_case::illumination_down},
    {illumination_increment, pqrs::hid::usage::apple_vendor_top_case::illumination_up},
    {apple_top_case_display_brightness_decrement, pqrs::hid::usage::apple_vendor_top_case::brightness_down},
    {apple_top_case_display_brightness_increment, pqrs::hid::usage::apple_vendor_top_case::brightness_up},

    // pqrs::hid::usage::apple_vendor_keyboard
    {dashboard, pqrs::hid::usage::apple_vendor_keyboard::dashboard},
    {launchpad, pqrs::hid::usage::apple_vendor_keyboard::launchpad},
    {mission_control, pqrs::hid::usage::apple_vendor_keyboard::expose_all},
    {apple_display_brightness_decrement, pqrs::hid::usage::apple_vendor_keyboard::brightness_down},
    {apple_display_brightness_increment, pqrs::hid::usage::apple_vendor_keyboard::brightness_up},

    // pqrs::hid::usage::consumer

    {keyboard_mute, pqrs::hid::usage::consumer::mute},
    {keyboard_volume_down, pqrs::hid::usage::consumer::volume_decrement},
    {keyboard_volume_up, pqrs::hid::usage::consumer::volume_increment},
    {display_brightness_decrement, pqrs::hid::usage::consumer::display_brightness_decrement},
    {display_brightness_increment, pqrs::hid::usage::consumer::display_brightness_increment},
    {rewind, pqrs::hid::usage::consumer::rewind},
    {play_or_pause, pqrs::hid::usage::consumer::play_or_pause},
    {fastforward, pqrs::hid::usage::consumer::fast_forward},
    {eject, pqrs::hid::usage::consumer::eject},
});

constexpr auto value_modifier_flag_map = mapbox::eternal::map<value_t, modifier_flag>({
    // keyboard_caps_lock == std::nullopt
    {keyboard_left_control, modifier_flag::left_control},
    {keyboard_left_shift, modifier_flag::left_shift},
    {keyboard_left_option, modifier_flag::left_option},
    {keyboard_left_command, modifier_flag::left_command},
    {keyboard_right_control, modifier_flag::right_control},
    {keyboard_right_shift, modifier_flag::right_shift},
    {keyboard_right_option, modifier_flag::right_option},
    {keyboard_right_command, modifier_flag::right_command},
    {fn, modifier_flag::fn},
});
} // namespace impl
} // namespace key_code

inline std::string make_key_code_name(key_code::value_t key_code) {
  for (const auto& pair : key_code::impl::name_value_pairs) {
    if (pair.second == key_code) {
      return pair.first.c_str();
    }
  }
  return fmt::format("(number:{0})", type_safe::get(key_code));
}

inline std::optional<key_code::value_t> make_key_code(const std::string& name) {
  auto& map = key_code::impl::name_value_map;
  auto it = map.find(name.c_str());
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::optional<key_code::value_t> make_key_code(pqrs::hid::usage_page::value_t usage_page,
                                                      pqrs::hid::usage::value_t usage) {
  if (usage_page == pqrs::hid::usage_page::keyboard_or_keypad) {
    if (pqrs::hid::usage::keyboard_or_keypad::keyboard_a <= usage && usage < pqrs::hid::usage::keyboard_or_keypad::reserved) {
      return key_code::value_t(type_safe::get(usage));
    }

  } else if (usage_page == pqrs::hid::usage_page::apple_vendor_top_case) {
    if (usage == pqrs::hid::usage::apple_vendor_top_case::keyboard_fn) {
      return key_code::fn;
    } else if (usage == pqrs::hid::usage::apple_vendor_top_case::brightness_up) {
      return key_code::apple_top_case_display_brightness_increment;
    } else if (usage == pqrs::hid::usage::apple_vendor_top_case::brightness_down) {
      return key_code::apple_top_case_display_brightness_decrement;
    } else if (usage == pqrs::hid::usage::apple_vendor_top_case::illumination_up) {
      return key_code::illumination_increment;
    } else if (usage == pqrs::hid::usage::apple_vendor_top_case::illumination_down) {
      return key_code::illumination_decrement;
    }

  } else if (usage_page == pqrs::hid::usage_page::apple_vendor_keyboard) {
    if (usage == pqrs::hid::usage::apple_vendor_keyboard::dashboard) {
      return key_code::dashboard;
    } else if (usage == pqrs::hid::usage::apple_vendor_keyboard::function) {
      return key_code::fn;
    } else if (usage == pqrs::hid::usage::apple_vendor_keyboard::launchpad) {
      return key_code::launchpad;
    } else if (usage == pqrs::hid::usage::apple_vendor_keyboard::expose_all) {
      return key_code::mission_control;
    } else if (usage == pqrs::hid::usage::apple_vendor_keyboard::brightness_up) {
      return key_code::apple_display_brightness_increment;
    } else if (usage == pqrs::hid::usage::apple_vendor_keyboard::brightness_down) {
      return key_code::apple_display_brightness_decrement;
    }
  }

  return std::nullopt;
}

inline std::optional<key_code::value_t> make_key_code(const pqrs::osx::iokit_hid_value& hid_value) {
  if (auto usage_page = hid_value.get_usage_page()) {
    if (auto usage = hid_value.get_usage()) {
      return make_key_code(*usage_page,
                           *usage);
    }
  }
  return std::nullopt;
}

inline std::optional<pqrs::hid::usage_page::value_t> make_hid_usage_page(key_code::value_t key_code) {
  if (key_code < key_code::extra_) {
    return pqrs::hid::usage_page::keyboard_or_keypad;
  }

  auto& map = key_code::impl::value_usage_page_map;
  auto it = map.find(key_code);
  if (it != map.end()) {
    return it->second;
  }

  return std::nullopt;
}

inline std::optional<pqrs::hid::usage::value_t> make_hid_usage(key_code::value_t key_code) {
  if (key_code < key_code::extra_) {
    return pqrs::hid::usage::value_t(type_safe::get(key_code));
  }

  auto& map = key_code::impl::value_usage_map;
  auto it = map.find(key_code);
  if (it != map.end()) {
    return it->second;
  }

  return std::nullopt;
}

inline std::optional<key_code::value_t> make_key_code(modifier_flag modifier_flag) {
  switch (modifier_flag) {
    case modifier_flag::zero:
      return std::nullopt;

    case modifier_flag::caps_lock:
      return key_code::keyboard_caps_lock;

    case modifier_flag::left_control:
      return key_code::keyboard_left_control;

    case modifier_flag::left_shift:
      return key_code::keyboard_left_shift;

    case modifier_flag::left_option:
      return key_code::keyboard_left_option;

    case modifier_flag::left_command:
      return key_code::keyboard_left_command;

    case modifier_flag::right_control:
      return key_code::keyboard_right_control;

    case modifier_flag::right_shift:
      return key_code::keyboard_right_shift;

    case modifier_flag::right_option:
      return key_code::keyboard_right_option;

    case modifier_flag::right_command:
      return key_code::keyboard_right_command;

    case modifier_flag::fn:
      return key_code::fn;

    case modifier_flag::end_:
      return std::nullopt;
  }
}

inline std::optional<modifier_flag> make_modifier_flag(key_code::value_t key_code) {
  auto& map = key_code::impl::value_modifier_flag_map;
  auto it = map.find(key_code);
  if (it != map.end()) {
    return it->second;
  }

  return std::nullopt;
}

inline std::optional<modifier_flag> make_modifier_flag(pqrs::hid::usage_page::value_t usage_page,
                                                       pqrs::hid::usage::value_t usage) {
  if (auto key_code = make_key_code(usage_page, usage)) {
    return make_modifier_flag(*key_code);
  }
  return std::nullopt;
}

inline std::optional<modifier_flag> make_modifier_flag(const pqrs::osx::iokit_hid_value& hid_value) {
  if (auto usage_page = hid_value.get_usage_page()) {
    if (auto usage = hid_value.get_usage()) {
      return make_modifier_flag(*usage_page,
                                *usage);
    }
  }
  return std::nullopt;
}

namespace key_code {
inline void from_json(const nlohmann::json& json, key_code::value_t& value) {
  if (json.is_string()) {
    if (auto v = make_key_code(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key_code: `{0}`", json.dump()));
    }
  } else if (json.is_number()) {
    value = key_code::value_t(json.get<type_safe::underlying_type<value_t>>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", json.dump()));
  }
}
} // namespace key_code
} // namespace krbn

namespace std {
template <>
struct hash<krbn::key_code::value_t> : type_safe::hashable<krbn::key_code::value_t> {
};
} // namespace std
