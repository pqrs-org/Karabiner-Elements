#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDUsageTables.h>
#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace osx {
namespace iokit_hid_usage {
struct value_t : type_safe::strong_typedef<value_t, int32_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}

//
// iokit_hid_usage_page::generic_desktop
//

namespace generic_desktop {
constexpr value_t pointer(kHIDUsage_GD_Pointer);
constexpr value_t mouse(kHIDUsage_GD_Mouse);
constexpr value_t keyboard(kHIDUsage_GD_Keyboard);
constexpr value_t keypad(kHIDUsage_GD_Keypad);
constexpr value_t x(kHIDUsage_GD_X);
constexpr value_t y(kHIDUsage_GD_Y);
constexpr value_t z(kHIDUsage_GD_Z);
constexpr value_t wheel(kHIDUsage_GD_Wheel);
} // namespace generic_desktop

//
// iokit_hid_usage_page::keyboard_or_keypad
//

namespace keyboard_or_keypad {
constexpr value_t error_rollover(kHIDUsage_KeyboardErrorRollOver);
constexpr value_t post_fail(kHIDUsage_KeyboardPOSTFail);
constexpr value_t error_undefined(kHIDUsage_KeyboardErrorUndefined);
constexpr value_t keyboard_a(kHIDUsage_KeyboardA);
constexpr value_t keyboard_b(kHIDUsage_KeyboardB);
constexpr value_t keyboard_c(kHIDUsage_KeyboardC);
constexpr value_t keyboard_d(kHIDUsage_KeyboardD);
constexpr value_t keyboard_e(kHIDUsage_KeyboardE);
constexpr value_t keyboard_f(kHIDUsage_KeyboardF);
constexpr value_t keyboard_g(kHIDUsage_KeyboardG);
constexpr value_t keyboard_h(kHIDUsage_KeyboardH);
constexpr value_t keyboard_i(kHIDUsage_KeyboardI);
constexpr value_t keyboard_j(kHIDUsage_KeyboardJ);
constexpr value_t keyboard_k(kHIDUsage_KeyboardK);
constexpr value_t keyboard_l(kHIDUsage_KeyboardL);
constexpr value_t keyboard_m(kHIDUsage_KeyboardM);
constexpr value_t keyboard_n(kHIDUsage_KeyboardN);
constexpr value_t keyboard_o(kHIDUsage_KeyboardO);
constexpr value_t keyboard_p(kHIDUsage_KeyboardP);
constexpr value_t keyboard_q(kHIDUsage_KeyboardQ);
constexpr value_t keyboard_r(kHIDUsage_KeyboardR);
constexpr value_t keyboard_s(kHIDUsage_KeyboardS);
constexpr value_t keyboard_t(kHIDUsage_KeyboardT);
constexpr value_t keyboard_u(kHIDUsage_KeyboardU);
constexpr value_t keyboard_v(kHIDUsage_KeyboardV);
constexpr value_t keyboard_w(kHIDUsage_KeyboardW);
constexpr value_t keyboard_x(kHIDUsage_KeyboardX);
constexpr value_t keyboard_y(kHIDUsage_KeyboardY);
constexpr value_t keyboard_z(kHIDUsage_KeyboardZ);
constexpr value_t keyboard_1(kHIDUsage_Keyboard1);
constexpr value_t keyboard_2(kHIDUsage_Keyboard2);
constexpr value_t keyboard_3(kHIDUsage_Keyboard3);
constexpr value_t keyboard_4(kHIDUsage_Keyboard4);
constexpr value_t keyboard_5(kHIDUsage_Keyboard5);
constexpr value_t keyboard_6(kHIDUsage_Keyboard6);
constexpr value_t keyboard_7(kHIDUsage_Keyboard7);
constexpr value_t keyboard_8(kHIDUsage_Keyboard8);
constexpr value_t keyboard_9(kHIDUsage_Keyboard9);
constexpr value_t keyboard_0(kHIDUsage_Keyboard0);
constexpr value_t keyboard_return_or_enter(kHIDUsage_KeyboardReturnOrEnter);
constexpr value_t keyboard_escape(kHIDUsage_KeyboardEscape);
constexpr value_t keyboard_delete_or_backspace(kHIDUsage_KeyboardDeleteOrBackspace);
constexpr value_t keyboard_tab(kHIDUsage_KeyboardTab);
constexpr value_t keyboard_spacebar(kHIDUsage_KeyboardSpacebar);
constexpr value_t keyboard_hyphen(kHIDUsage_KeyboardHyphen);
constexpr value_t keyboard_equal_sign(kHIDUsage_KeyboardEqualSign);
constexpr value_t keyboard_open_bracket(kHIDUsage_KeyboardOpenBracket);
constexpr value_t keyboard_close_bracket(kHIDUsage_KeyboardCloseBracket);
constexpr value_t keyboard_backslash(kHIDUsage_KeyboardBackslash);
constexpr value_t keyboard_non_us_pound(kHIDUsage_KeyboardNonUSPound);
constexpr value_t keyboard_semicolon(kHIDUsage_KeyboardSemicolon);
constexpr value_t keyboard_quote(kHIDUsage_KeyboardQuote);
constexpr value_t keyboard_grave_accent_and_tilde(kHIDUsage_KeyboardGraveAccentAndTilde);
constexpr value_t keyboard_comma(kHIDUsage_KeyboardComma);
constexpr value_t keyboard_period(kHIDUsage_KeyboardPeriod);
constexpr value_t keyboard_slash(kHIDUsage_KeyboardSlash);
constexpr value_t keyboard_caps_lock(kHIDUsage_KeyboardCapsLock);
constexpr value_t keyboard_f1(kHIDUsage_KeyboardF1);
constexpr value_t keyboard_f2(kHIDUsage_KeyboardF2);
constexpr value_t keyboard_f3(kHIDUsage_KeyboardF3);
constexpr value_t keyboard_f4(kHIDUsage_KeyboardF4);
constexpr value_t keyboard_f5(kHIDUsage_KeyboardF5);
constexpr value_t keyboard_f6(kHIDUsage_KeyboardF6);
constexpr value_t keyboard_f7(kHIDUsage_KeyboardF7);
constexpr value_t keyboard_f8(kHIDUsage_KeyboardF8);
constexpr value_t keyboard_f9(kHIDUsage_KeyboardF9);
constexpr value_t keyboard_f10(kHIDUsage_KeyboardF10);
constexpr value_t keyboard_f11(kHIDUsage_KeyboardF11);
constexpr value_t keyboard_f12(kHIDUsage_KeyboardF12);
constexpr value_t keyboard_print_screen(kHIDUsage_KeyboardPrintScreen);
constexpr value_t keyboard_scroll_lock(kHIDUsage_KeyboardScrollLock);
constexpr value_t keyboard_pause(kHIDUsage_KeyboardPause);
constexpr value_t keyboard_insert(kHIDUsage_KeyboardInsert);
constexpr value_t keyboard_home(kHIDUsage_KeyboardHome);
constexpr value_t keyboard_page_up(kHIDUsage_KeyboardPageUp);
constexpr value_t keyboard_delete_forward(kHIDUsage_KeyboardDeleteForward);
constexpr value_t keyboard_end(kHIDUsage_KeyboardEnd);
constexpr value_t keyboard_page_down(kHIDUsage_KeyboardPageDown);
constexpr value_t keyboard_right_arrow(kHIDUsage_KeyboardRightArrow);
constexpr value_t keyboard_left_arrow(kHIDUsage_KeyboardLeftArrow);
constexpr value_t keyboard_down_arrow(kHIDUsage_KeyboardDownArrow);
constexpr value_t keyboard_up_arrow(kHIDUsage_KeyboardUpArrow);
constexpr value_t keypad_num_lock(kHIDUsage_KeypadNumLock);
constexpr value_t keypad_slash(kHIDUsage_KeypadSlash);
constexpr value_t keypad_asterisk(kHIDUsage_KeypadAsterisk);
constexpr value_t keypad_hyphen(kHIDUsage_KeypadHyphen);
constexpr value_t keypad_plus(kHIDUsage_KeypadPlus);
constexpr value_t keypad_enter(kHIDUsage_KeypadEnter);
constexpr value_t keypad_1(kHIDUsage_Keypad1);
constexpr value_t keypad_2(kHIDUsage_Keypad2);
constexpr value_t keypad_3(kHIDUsage_Keypad3);
constexpr value_t keypad_4(kHIDUsage_Keypad4);
constexpr value_t keypad_5(kHIDUsage_Keypad5);
constexpr value_t keypad_6(kHIDUsage_Keypad6);
constexpr value_t keypad_7(kHIDUsage_Keypad7);
constexpr value_t keypad_8(kHIDUsage_Keypad8);
constexpr value_t keypad_9(kHIDUsage_Keypad9);
constexpr value_t keypad_0(kHIDUsage_Keypad0);
constexpr value_t keypad_period(kHIDUsage_KeypadPeriod);
constexpr value_t keyboard_non_us_backslash(kHIDUsage_KeyboardNonUSBackslash);
constexpr value_t keyboard_application(kHIDUsage_KeyboardApplication);
constexpr value_t keyboard_power(kHIDUsage_KeyboardPower);
constexpr value_t keypad_equal_sign(kHIDUsage_KeypadEqualSign);
constexpr value_t keyboard_f13(kHIDUsage_KeyboardF13);
constexpr value_t keyboard_f14(kHIDUsage_KeyboardF14);
constexpr value_t keyboard_f15(kHIDUsage_KeyboardF15);
constexpr value_t keyboard_f16(kHIDUsage_KeyboardF16);
constexpr value_t keyboard_f17(kHIDUsage_KeyboardF17);
constexpr value_t keyboard_f18(kHIDUsage_KeyboardF18);
constexpr value_t keyboard_f19(kHIDUsage_KeyboardF19);
constexpr value_t keyboard_f20(kHIDUsage_KeyboardF20);
constexpr value_t keyboard_f21(kHIDUsage_KeyboardF21);
constexpr value_t keyboard_f22(kHIDUsage_KeyboardF22);
constexpr value_t keyboard_f23(kHIDUsage_KeyboardF23);
constexpr value_t keyboard_f24(kHIDUsage_KeyboardF24);
constexpr value_t keyboard_execute(kHIDUsage_KeyboardExecute);
constexpr value_t keyboard_help(kHIDUsage_KeyboardHelp);
constexpr value_t keyboard_menu(kHIDUsage_KeyboardMenu);
constexpr value_t keyboard_select(kHIDUsage_KeyboardSelect);
constexpr value_t keyboard_stop(kHIDUsage_KeyboardStop);
constexpr value_t keyboard_again(kHIDUsage_KeyboardAgain);
constexpr value_t keyboard_undo(kHIDUsage_KeyboardUndo);
constexpr value_t keyboard_cut(kHIDUsage_KeyboardCut);
constexpr value_t keyboard_copy(kHIDUsage_KeyboardCopy);
constexpr value_t keyboard_paste(kHIDUsage_KeyboardPaste);
constexpr value_t keyboard_find(kHIDUsage_KeyboardFind);
constexpr value_t keyboard_mute(kHIDUsage_KeyboardMute);
constexpr value_t keyboard_volume_up(kHIDUsage_KeyboardVolumeUp);
constexpr value_t keyboard_volume_down(kHIDUsage_KeyboardVolumeDown);
constexpr value_t keyboard_locking_caps_lock(kHIDUsage_KeyboardLockingCapsLock);
constexpr value_t keyboard_locking_num_lock(kHIDUsage_KeyboardLockingNumLock);
constexpr value_t keyboard_locking_scroll_lock(kHIDUsage_KeyboardLockingScrollLock);
constexpr value_t keypad_comma(kHIDUsage_KeypadComma);
constexpr value_t keypad_equal_sign_as400(kHIDUsage_KeypadEqualSignAS400);
constexpr value_t keyboard_international1(kHIDUsage_KeyboardInternational1);
constexpr value_t keyboard_international2(kHIDUsage_KeyboardInternational2);
constexpr value_t keyboard_international3(kHIDUsage_KeyboardInternational3);
constexpr value_t keyboard_international4(kHIDUsage_KeyboardInternational4);
constexpr value_t keyboard_international5(kHIDUsage_KeyboardInternational5);
constexpr value_t keyboard_international6(kHIDUsage_KeyboardInternational6);
constexpr value_t keyboard_international7(kHIDUsage_KeyboardInternational7);
constexpr value_t keyboard_international8(kHIDUsage_KeyboardInternational8);
constexpr value_t keyboard_international9(kHIDUsage_KeyboardInternational9);
constexpr value_t keyboard_lang1(kHIDUsage_KeyboardLANG1);
constexpr value_t keyboard_lang2(kHIDUsage_KeyboardLANG2);
constexpr value_t keyboard_lang3(kHIDUsage_KeyboardLANG3);
constexpr value_t keyboard_lang4(kHIDUsage_KeyboardLANG4);
constexpr value_t keyboard_lang5(kHIDUsage_KeyboardLANG5);
constexpr value_t keyboard_lang6(kHIDUsage_KeyboardLANG6);
constexpr value_t keyboard_lang7(kHIDUsage_KeyboardLANG7);
constexpr value_t keyboard_lang8(kHIDUsage_KeyboardLANG8);
constexpr value_t keyboard_lang9(kHIDUsage_KeyboardLANG9);
constexpr value_t keyboard_alternate_erase(kHIDUsage_KeyboardAlternateErase);
constexpr value_t keyboard_sys_req_or_attention(kHIDUsage_KeyboardSysReqOrAttention);
constexpr value_t keyboard_cancel(kHIDUsage_KeyboardCancel);
constexpr value_t keyboard_clear(kHIDUsage_KeyboardClear);
constexpr value_t keyboard_prior(kHIDUsage_KeyboardPrior);
constexpr value_t keyboard_return(kHIDUsage_KeyboardReturn);
constexpr value_t keyboard_separator(kHIDUsage_KeyboardSeparator);
constexpr value_t keyboard_out(kHIDUsage_KeyboardOut);
constexpr value_t keyboard_oper(kHIDUsage_KeyboardOper);
constexpr value_t keyboard_clear_or_again(kHIDUsage_KeyboardClearOrAgain);
constexpr value_t keyboard_cr_sel_or_props(kHIDUsage_KeyboardCrSelOrProps);
constexpr value_t keyboard_ex_sel(kHIDUsage_KeyboardExSel);
/* 0xA5-0xDF Reserved */
constexpr value_t keyboard_left_control(kHIDUsage_KeyboardLeftControl);
constexpr value_t keyboard_left_shift(kHIDUsage_KeyboardLeftShift);
constexpr value_t keyboard_left_alt(kHIDUsage_KeyboardLeftAlt);
constexpr value_t keyboard_left_gui(kHIDUsage_KeyboardLeftGUI);
constexpr value_t keyboard_right_control(kHIDUsage_KeyboardRightControl);
constexpr value_t keyboard_right_shift(kHIDUsage_KeyboardRightShift);
constexpr value_t keyboard_right_alt(kHIDUsage_KeyboardRightAlt);
constexpr value_t keyboard_right_gui(kHIDUsage_KeyboardRightGUI);
/* 0xE8-0xFFFF Reserved */
constexpr value_t reserved(kHIDUsage_Keyboard_Reserved);
} // namespace keyboard_or_keypad

//
// iokit_hid_usage_page::leds
//

namespace leds {
constexpr value_t caps_lock(kHIDUsage_LED_CapsLock);
}

//
// iokit_hid_usage_page::button
//

namespace button {
constexpr value_t button_1(kHIDUsage_Button_1);
constexpr value_t button_2(kHIDUsage_Button_2);
constexpr value_t button_3(kHIDUsage_Button_3);
constexpr value_t button_4(kHIDUsage_Button_4);
constexpr value_t button_5(kHIDUsage_Button_5);
constexpr value_t button_6(kHIDUsage_Button_6);
constexpr value_t button_7(kHIDUsage_Button_7);
constexpr value_t button_8(kHIDUsage_Button_8);
constexpr value_t button_9(kHIDUsage_Button_9);
constexpr value_t button_10(kHIDUsage_Button_10);
constexpr value_t button_11(kHIDUsage_Button_11);
constexpr value_t button_12(kHIDUsage_Button_12);
constexpr value_t button_13(kHIDUsage_Button_13);
constexpr value_t button_14(kHIDUsage_Button_14);
constexpr value_t button_15(kHIDUsage_Button_15);
constexpr value_t button_16(kHIDUsage_Button_16);
constexpr value_t button_17(kHIDUsage_Button_17);
constexpr value_t button_18(kHIDUsage_Button_18);
constexpr value_t button_19(kHIDUsage_Button_19);
constexpr value_t button_20(kHIDUsage_Button_20);
constexpr value_t button_21(kHIDUsage_Button_21);
constexpr value_t button_22(kHIDUsage_Button_22);
constexpr value_t button_23(kHIDUsage_Button_23);
constexpr value_t button_24(kHIDUsage_Button_24);
constexpr value_t button_25(kHIDUsage_Button_25);
constexpr value_t button_26(kHIDUsage_Button_26);
constexpr value_t button_27(kHIDUsage_Button_27);
constexpr value_t button_28(kHIDUsage_Button_28);
constexpr value_t button_29(kHIDUsage_Button_29);
constexpr value_t button_30(kHIDUsage_Button_30);
constexpr value_t button_31(kHIDUsage_Button_31);
constexpr value_t button_32(kHIDUsage_Button_32);
} // namespace button

//
// iokit_hid_usage_page::consumer
//

namespace consumer {
constexpr value_t consumer_control(kHIDUsage_Csmr_ConsumerControl);
constexpr value_t power(kHIDUsage_Csmr_Power);
constexpr value_t display_brightness_increment(kHIDUsage_Csmr_DisplayBrightnessIncrement);
constexpr value_t display_brightness_decrement(kHIDUsage_Csmr_DisplayBrightnessDecrement);
constexpr value_t fast_forward(kHIDUsage_Csmr_FastForward);
constexpr value_t rewind(kHIDUsage_Csmr_Rewind);
constexpr value_t scan_next_track(kHIDUsage_Csmr_ScanNextTrack);
constexpr value_t scan_previous_track(kHIDUsage_Csmr_ScanPreviousTrack);
constexpr value_t eject(kHIDUsage_Csmr_Eject);
constexpr value_t play_or_pause(kHIDUsage_Csmr_PlayOrPause);
constexpr value_t mute(kHIDUsage_Csmr_Mute);
constexpr value_t volume_increment(kHIDUsage_Csmr_VolumeIncrement);
constexpr value_t volume_decrement(kHIDUsage_Csmr_VolumeDecrement);
constexpr value_t ac_pan(kHIDUsage_Csmr_ACPan);
} // namespace consumer

//
// iokit_hid_usage_page::apple_vendor
//

namespace apple_vendor {
constexpr value_t top_case(0x0001);
constexpr value_t display(0x0002);
constexpr value_t accelerometer(0x0003);
constexpr value_t ambient_light_sensor(0x0004);
constexpr value_t temperature_sensor(0x0005);
constexpr value_t keyboard(0x0006);
constexpr value_t headset(0x0007);
constexpr value_t proximity_sensor(0x0008);
constexpr value_t gyro(0x0009);
constexpr value_t compass(0x000A);
constexpr value_t device_management(0x000B);
constexpr value_t trackpad(0x000C);
constexpr value_t top_case_reserved(0x000D);
constexpr value_t motion(0x000E);
constexpr value_t keyboard_backlight(0x000F);
constexpr value_t device_motion_lite(0x0010);
constexpr value_t force(0x0011);
constexpr value_t bluetooth_radio(0x0012);
constexpr value_t orb(0x0013);
constexpr value_t accessory_battery(0x0014);
constexpr value_t humidity(0x0015);
constexpr value_t hid_event_relay(0x0016);
constexpr value_t nx_event(0x0017);
constexpr value_t nx_event_translated(0x0018);
constexpr value_t nx_event_diagnostic(0x0019);
constexpr value_t homer(0x0020);
constexpr value_t color(0x0021);
constexpr value_t accessibility(0x0022);
} // namespace apple_vendor

//
// iokit_hid_usage_page::apple_vendor_keyboard
//

namespace apple_vendor_keyboard {
constexpr value_t spotlight(0x0001);
constexpr value_t dashboard(0x0002);
constexpr value_t function(0x0003);
constexpr value_t launchpad(0x0004);
constexpr value_t reserved(0x000a);
constexpr value_t caps_lock_delay_enable(0x000b);
constexpr value_t power_state(0x000c);
constexpr value_t expose_all(0x0010);
constexpr value_t expose_desktop(0x0011);
constexpr value_t brightness_up(0x0020);
constexpr value_t brightness_down(0x0021);
constexpr value_t language(0x0030);
} // namespace apple_vendor_keyboard

//
// iokit_hid_usage_page::apple_vendor_multitouch
//

namespace apple_vendor_multitouch {
constexpr value_t power_off(0x0001);
constexpr value_t device_ready(0x0002);
constexpr value_t external_message(0x0003);
constexpr value_t will_power_on(0x0004);
constexpr value_t touch_cancel(0x0005);
} // namespace apple_vendor_multitouch

//
// iokit_hid_usage_page::apple_vendor_top_case
//

namespace apple_vendor_top_case {
constexpr value_t keyboard_fn(0x0003);
constexpr value_t brightness_up(0x0004);
constexpr value_t brightness_down(0x0005);
constexpr value_t video_mirror(0x0006);
constexpr value_t illumination_toggle(0x0007);
constexpr value_t illumination_up(0x0008);
constexpr value_t illumination_down(0x0009);
constexpr value_t clamshell_latched(0x000a);
constexpr value_t reserved_mouse_data(0x00c0);
} // namespace apple_vendor_top_case
} // namespace iokit_hid_usage
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_hid_usage::value_t> : type_safe::hashable<pqrs::osx::iokit_hid_usage::value_t> {
};
} // namespace std
