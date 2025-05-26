#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hidsystem/ev_keymap.h>
#include <mapbox/eternal.hpp>
#include <optional>
#include <pqrs/hid.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs {
namespace osx {
namespace iokit_hid_system {
namespace aux_control_button {
struct value_t : type_safe::strong_typedef<value_t, uint16_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;
};

//
// values from NX_SPECIALKEY_POST_MASK in IOKit/hidsystem/ev_keymap.h
//

constexpr value_t sound_up(NX_KEYTYPE_SOUND_UP);
constexpr value_t sound_down(NX_KEYTYPE_SOUND_DOWN);
constexpr value_t power(NX_POWER_KEY);
constexpr value_t mute(NX_KEYTYPE_MUTE);
constexpr value_t brightness_up(NX_KEYTYPE_BRIGHTNESS_UP);
constexpr value_t brightness_down(NX_KEYTYPE_BRIGHTNESS_DOWN);
constexpr value_t contrast_up(NX_KEYTYPE_CONTRAST_UP);
constexpr value_t contrast_down(NX_KEYTYPE_CONTRAST_DOWN);
constexpr value_t launch_panel(NX_KEYTYPE_LAUNCH_PANEL);
constexpr value_t eject(NX_KEYTYPE_EJECT);
constexpr value_t vidmirror(NX_KEYTYPE_VIDMIRROR);
constexpr value_t play(NX_KEYTYPE_PLAY);
constexpr value_t next(NX_KEYTYPE_NEXT);
constexpr value_t previous(NX_KEYTYPE_PREVIOUS);
constexpr value_t fast(NX_KEYTYPE_FAST);
constexpr value_t rewind(NX_KEYTYPE_REWIND);
constexpr value_t illumination_up(NX_KEYTYPE_ILLUMINATION_UP);
constexpr value_t illumination_down(NX_KEYTYPE_ILLUMINATION_DOWN);
constexpr value_t illumination_toggle(NX_KEYTYPE_ILLUMINATION_TOGGLE);

//
// make_aux_control_button
//

namespace impl {
constexpr auto usage_page_keyboard_or_keypad_map = mapbox::eternal::map<hid::usage::value_t, value_t>({
    {hid::usage::keyboard_or_keypad::keyboard_power, power},
    {hid::usage::keyboard_or_keypad::keyboard_mute, mute},
    {hid::usage::keyboard_or_keypad::keyboard_volume_up, sound_up},
    {hid::usage::keyboard_or_keypad::keyboard_volume_down, sound_down},
});

constexpr auto usage_page_consumer_map = mapbox::eternal::map<hid::usage::value_t, value_t>({
    // hid::usage_page::consumer::consumer_control
    {hid::usage::consumer::power, power},
    {hid::usage::consumer::display_brightness_increment, brightness_up},
    {hid::usage::consumer::display_brightness_decrement, brightness_down},
    {hid::usage::consumer::fast_forward, fast},
    {hid::usage::consumer::rewind, rewind},
    {hid::usage::consumer::scan_next_track, next},
    {hid::usage::consumer::scan_previous_track, previous},
    {hid::usage::consumer::eject, eject},
    {hid::usage::consumer::play_or_pause, play},
    {hid::usage::consumer::mute, mute},
    {hid::usage::consumer::volume_increment, sound_up},
    {hid::usage::consumer::volume_decrement, sound_down},
    // hid::usage_page::consumer::ac_pan
});

constexpr auto usage_page_apple_vendor_keyboard_map = mapbox::eternal::map<hid::usage::value_t, value_t>({
    {hid::usage::apple_vendor_keyboard::brightness_up, brightness_up},
    {hid::usage::apple_vendor_keyboard::brightness_down, brightness_down},
});

constexpr auto usage_page_apple_vendor_top_case_map = mapbox::eternal::map<hid::usage::value_t, value_t>({
    {hid::usage::apple_vendor_top_case::brightness_up, brightness_up},
    {hid::usage::apple_vendor_top_case::brightness_down, brightness_down},
    {hid::usage::apple_vendor_top_case::video_mirror, vidmirror},
    {hid::usage::apple_vendor_top_case::illumination_toggle, illumination_toggle},
    {hid::usage::apple_vendor_top_case::illumination_up, illumination_up},
    {hid::usage::apple_vendor_top_case::illumination_down, illumination_down},
});

template <typename T>
inline std::optional<value_t> find(T& map, hid::usage::value_t usage) {
  auto it = map.find(usage);
  if (it != std::end(map)) {
    return it->second;
  }
  return std::nullopt;
}
} // namespace impl
} // namespace aux_control_button

inline std::optional<aux_control_button::value_t> make_aux_control_button(hid::usage_page::value_t usage_page, hid::usage::value_t usage) {
  if (usage_page == hid::usage_page::keyboard_or_keypad) {
    return aux_control_button::impl::find(aux_control_button::impl::usage_page_keyboard_or_keypad_map, usage);
  } else if (usage_page == hid::usage_page::consumer) {
    return aux_control_button::impl::find(aux_control_button::impl::usage_page_consumer_map, usage);
  } else if (usage_page == hid::usage_page::apple_vendor_keyboard) {
    return aux_control_button::impl::find(aux_control_button::impl::usage_page_apple_vendor_keyboard_map, usage);
  } else if (usage_page == hid::usage_page::apple_vendor_top_case) {
    return aux_control_button::impl::find(aux_control_button::impl::usage_page_apple_vendor_top_case_map, usage);
  }

  return std::nullopt;
}
} // namespace iokit_hid_system
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_hid_system::aux_control_button::value_t> : type_safe::hashable<pqrs::osx::iokit_hid_system::aux_control_button::value_t> {
};
} // namespace std
