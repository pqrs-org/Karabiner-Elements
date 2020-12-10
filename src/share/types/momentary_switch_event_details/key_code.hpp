#pragma once

#include "../modifier_flag.hpp"
#include <pqrs/osx/iokit_hid_value.hpp>

namespace krbn {
namespace momentary_switch_event_details {
namespace key_code {
constexpr std::pair<const mapbox::eternal::string, const pqrs::hid::usage::value_t> name_value_pairs[] = {
    // Aliases

    {"left_option", pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt},
    {"left_command", pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui},
    {"right_option", pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt},
    {"right_command", pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui},
    {"japanese_eisuu", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang2},
    {"japanese_kana", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang1},
    {"japanese_pc_nfer", pqrs::hid::usage::keyboard_or_keypad::keyboard_international5},
    {"japanese_pc_xfer", pqrs::hid::usage::keyboard_or_keypad::keyboard_international4},
    {"japanese_pc_katakana", pqrs::hid::usage::keyboard_or_keypad::keyboard_international2},
    {"volume_down", pqrs::hid::usage::keyboard_or_keypad::keyboard_volume_down},
    {"volume_up", pqrs::hid::usage::keyboard_or_keypad::keyboard_volume_up},

    // Extras

    {"vk_none", pqrs::hid::usage::undefined},

    // Usages

    {"a", pqrs::hid::usage::keyboard_or_keypad::keyboard_a},
    {"b", pqrs::hid::usage::keyboard_or_keypad::keyboard_b},
    {"c", pqrs::hid::usage::keyboard_or_keypad::keyboard_c},
    {"d", pqrs::hid::usage::keyboard_or_keypad::keyboard_d},
    {"e", pqrs::hid::usage::keyboard_or_keypad::keyboard_e},
    {"f", pqrs::hid::usage::keyboard_or_keypad::keyboard_f},
    {"g", pqrs::hid::usage::keyboard_or_keypad::keyboard_g},
    {"h", pqrs::hid::usage::keyboard_or_keypad::keyboard_h},
    {"i", pqrs::hid::usage::keyboard_or_keypad::keyboard_i},
    {"j", pqrs::hid::usage::keyboard_or_keypad::keyboard_j},
    {"k", pqrs::hid::usage::keyboard_or_keypad::keyboard_k},
    {"l", pqrs::hid::usage::keyboard_or_keypad::keyboard_l},
    {"m", pqrs::hid::usage::keyboard_or_keypad::keyboard_m},
    {"n", pqrs::hid::usage::keyboard_or_keypad::keyboard_n},
    {"o", pqrs::hid::usage::keyboard_or_keypad::keyboard_o},
    {"p", pqrs::hid::usage::keyboard_or_keypad::keyboard_p},
    {"q", pqrs::hid::usage::keyboard_or_keypad::keyboard_q},
    {"r", pqrs::hid::usage::keyboard_or_keypad::keyboard_r},
    {"s", pqrs::hid::usage::keyboard_or_keypad::keyboard_s},
    {"t", pqrs::hid::usage::keyboard_or_keypad::keyboard_t},
    {"u", pqrs::hid::usage::keyboard_or_keypad::keyboard_u},
    {"v", pqrs::hid::usage::keyboard_or_keypad::keyboard_v},
    {"w", pqrs::hid::usage::keyboard_or_keypad::keyboard_w},
    {"x", pqrs::hid::usage::keyboard_or_keypad::keyboard_x},
    {"y", pqrs::hid::usage::keyboard_or_keypad::keyboard_y},
    {"z", pqrs::hid::usage::keyboard_or_keypad::keyboard_z},
    {"1", pqrs::hid::usage::keyboard_or_keypad::keyboard_1},
    {"2", pqrs::hid::usage::keyboard_or_keypad::keyboard_2},
    {"3", pqrs::hid::usage::keyboard_or_keypad::keyboard_3},
    {"4", pqrs::hid::usage::keyboard_or_keypad::keyboard_4},
    {"5", pqrs::hid::usage::keyboard_or_keypad::keyboard_5},
    {"6", pqrs::hid::usage::keyboard_or_keypad::keyboard_6},
    {"7", pqrs::hid::usage::keyboard_or_keypad::keyboard_7},
    {"8", pqrs::hid::usage::keyboard_or_keypad::keyboard_8},
    {"9", pqrs::hid::usage::keyboard_or_keypad::keyboard_9},
    {"0", pqrs::hid::usage::keyboard_or_keypad::keyboard_0},
    {"return_or_enter", pqrs::hid::usage::keyboard_or_keypad::keyboard_return_or_enter},
    {"escape", pqrs::hid::usage::keyboard_or_keypad::keyboard_escape},
    {"delete_or_backspace", pqrs::hid::usage::keyboard_or_keypad::keyboard_delete_or_backspace},
    {"tab", pqrs::hid::usage::keyboard_or_keypad::keyboard_tab},
    {"spacebar", pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar},
    {"hyphen", pqrs::hid::usage::keyboard_or_keypad::keyboard_hyphen},
    {"equal_sign", pqrs::hid::usage::keyboard_or_keypad::keyboard_equal_sign},
    {"open_bracket", pqrs::hid::usage::keyboard_or_keypad::keyboard_open_bracket},
    {"close_bracket", pqrs::hid::usage::keyboard_or_keypad::keyboard_close_bracket},
    {"backslash", pqrs::hid::usage::keyboard_or_keypad::keyboard_backslash},
    {"non_us_pound", pqrs::hid::usage::keyboard_or_keypad::keyboard_non_us_pound},
    {"semicolon", pqrs::hid::usage::keyboard_or_keypad::keyboard_semicolon},
    {"quote", pqrs::hid::usage::keyboard_or_keypad::keyboard_quote},
    {"grave_accent_and_tilde", pqrs::hid::usage::keyboard_or_keypad::keyboard_grave_accent_and_tilde},
    {"comma", pqrs::hid::usage::keyboard_or_keypad::keyboard_comma},
    {"period", pqrs::hid::usage::keyboard_or_keypad::keyboard_period},
    {"slash", pqrs::hid::usage::keyboard_or_keypad::keyboard_slash},
    {"caps_lock", pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock},
    {"f1", pqrs::hid::usage::keyboard_or_keypad::keyboard_f1},
    {"f2", pqrs::hid::usage::keyboard_or_keypad::keyboard_f2},
    {"f3", pqrs::hid::usage::keyboard_or_keypad::keyboard_f3},
    {"f4", pqrs::hid::usage::keyboard_or_keypad::keyboard_f4},
    {"f5", pqrs::hid::usage::keyboard_or_keypad::keyboard_f5},
    {"f6", pqrs::hid::usage::keyboard_or_keypad::keyboard_f6},
    {"f7", pqrs::hid::usage::keyboard_or_keypad::keyboard_f7},
    {"f8", pqrs::hid::usage::keyboard_or_keypad::keyboard_f8},
    {"f9", pqrs::hid::usage::keyboard_or_keypad::keyboard_f9},
    {"f10", pqrs::hid::usage::keyboard_or_keypad::keyboard_f10},
    {"f11", pqrs::hid::usage::keyboard_or_keypad::keyboard_f11},
    {"f12", pqrs::hid::usage::keyboard_or_keypad::keyboard_f12},
    {"print_screen", pqrs::hid::usage::keyboard_or_keypad::keyboard_print_screen},
    {"scroll_lock", pqrs::hid::usage::keyboard_or_keypad::keyboard_scroll_lock},
    {"pause", pqrs::hid::usage::keyboard_or_keypad::keyboard_pause},
    {"insert", pqrs::hid::usage::keyboard_or_keypad::keyboard_insert},
    {"home", pqrs::hid::usage::keyboard_or_keypad::keyboard_home},
    {"page_up", pqrs::hid::usage::keyboard_or_keypad::keyboard_page_up},
    {"delete_forward", pqrs::hid::usage::keyboard_or_keypad::keyboard_delete_forward},
    {"end", pqrs::hid::usage::keyboard_or_keypad::keyboard_end},
    {"page_down", pqrs::hid::usage::keyboard_or_keypad::keyboard_page_down},
    {"right_arrow", pqrs::hid::usage::keyboard_or_keypad::keyboard_right_arrow},
    {"left_arrow", pqrs::hid::usage::keyboard_or_keypad::keyboard_left_arrow},
    {"down_arrow", pqrs::hid::usage::keyboard_or_keypad::keyboard_down_arrow},
    {"up_arrow", pqrs::hid::usage::keyboard_or_keypad::keyboard_up_arrow},
    {"keypad_num_lock", pqrs::hid::usage::keyboard_or_keypad::keypad_num_lock},
    {"keypad_slash", pqrs::hid::usage::keyboard_or_keypad::keypad_slash},
    {"keypad_asterisk", pqrs::hid::usage::keyboard_or_keypad::keypad_asterisk},
    {"keypad_hyphen", pqrs::hid::usage::keyboard_or_keypad::keypad_hyphen},
    {"keypad_plus", pqrs::hid::usage::keyboard_or_keypad::keypad_plus},
    {"keypad_enter", pqrs::hid::usage::keyboard_or_keypad::keypad_enter},
    {"keypad_1", pqrs::hid::usage::keyboard_or_keypad::keypad_1},
    {"keypad_2", pqrs::hid::usage::keyboard_or_keypad::keypad_2},
    {"keypad_3", pqrs::hid::usage::keyboard_or_keypad::keypad_3},
    {"keypad_4", pqrs::hid::usage::keyboard_or_keypad::keypad_4},
    {"keypad_5", pqrs::hid::usage::keyboard_or_keypad::keypad_5},
    {"keypad_6", pqrs::hid::usage::keyboard_or_keypad::keypad_6},
    {"keypad_7", pqrs::hid::usage::keyboard_or_keypad::keypad_7},
    {"keypad_8", pqrs::hid::usage::keyboard_or_keypad::keypad_8},
    {"keypad_9", pqrs::hid::usage::keyboard_or_keypad::keypad_9},
    {"keypad_0", pqrs::hid::usage::keyboard_or_keypad::keypad_0},
    {"keypad_period", pqrs::hid::usage::keyboard_or_keypad::keypad_period},
    {"non_us_backslash", pqrs::hid::usage::keyboard_or_keypad::keyboard_non_us_backslash},
    {"application", pqrs::hid::usage::keyboard_or_keypad::keyboard_application},
    {"power", pqrs::hid::usage::keyboard_or_keypad::keyboard_power},
    {"keypad_equal_sign", pqrs::hid::usage::keyboard_or_keypad::keypad_equal_sign},
    {"f13", pqrs::hid::usage::keyboard_or_keypad::keyboard_f13},
    {"f14", pqrs::hid::usage::keyboard_or_keypad::keyboard_f14},
    {"f15", pqrs::hid::usage::keyboard_or_keypad::keyboard_f15},
    {"f16", pqrs::hid::usage::keyboard_or_keypad::keyboard_f16},
    {"f17", pqrs::hid::usage::keyboard_or_keypad::keyboard_f17},
    {"f18", pqrs::hid::usage::keyboard_or_keypad::keyboard_f18},
    {"f19", pqrs::hid::usage::keyboard_or_keypad::keyboard_f19},
    {"f20", pqrs::hid::usage::keyboard_or_keypad::keyboard_f20},
    {"f21", pqrs::hid::usage::keyboard_or_keypad::keyboard_f21},
    {"f22", pqrs::hid::usage::keyboard_or_keypad::keyboard_f22},
    {"f23", pqrs::hid::usage::keyboard_or_keypad::keyboard_f23},
    {"f24", pqrs::hid::usage::keyboard_or_keypad::keyboard_f24},
    {"execute", pqrs::hid::usage::keyboard_or_keypad::keyboard_execute},
    {"help", pqrs::hid::usage::keyboard_or_keypad::keyboard_help},
    {"menu", pqrs::hid::usage::keyboard_or_keypad::keyboard_menu},
    {"select", pqrs::hid::usage::keyboard_or_keypad::keyboard_select},
    {"stop", pqrs::hid::usage::keyboard_or_keypad::keyboard_stop},
    {"again", pqrs::hid::usage::keyboard_or_keypad::keyboard_again},
    {"undo", pqrs::hid::usage::keyboard_or_keypad::keyboard_undo},
    {"cut", pqrs::hid::usage::keyboard_or_keypad::keyboard_cut},
    {"copy", pqrs::hid::usage::keyboard_or_keypad::keyboard_copy},
    {"paste", pqrs::hid::usage::keyboard_or_keypad::keyboard_paste},
    {"find", pqrs::hid::usage::keyboard_or_keypad::keyboard_find},
    // keyboard_mute
    // keyboard_volume_down
    // keyboard_volume_up
    {"locking_caps_lock", pqrs::hid::usage::keyboard_or_keypad::keyboard_locking_caps_lock},
    {"locking_num_lock", pqrs::hid::usage::keyboard_or_keypad::keyboard_locking_num_lock},
    {"locking_scroll_lock", pqrs::hid::usage::keyboard_or_keypad::keyboard_locking_scroll_lock},
    {"keypad_comma", pqrs::hid::usage::keyboard_or_keypad::keypad_comma},
    {"keypad_equal_sign_as400", pqrs::hid::usage::keyboard_or_keypad::keypad_equal_sign_as400},
    {"international1", pqrs::hid::usage::keyboard_or_keypad::keyboard_international1},
    {"international2", pqrs::hid::usage::keyboard_or_keypad::keyboard_international2},
    {"international3", pqrs::hid::usage::keyboard_or_keypad::keyboard_international3},
    {"international4", pqrs::hid::usage::keyboard_or_keypad::keyboard_international4},
    {"international5", pqrs::hid::usage::keyboard_or_keypad::keyboard_international5},
    {"international6", pqrs::hid::usage::keyboard_or_keypad::keyboard_international6},
    {"international7", pqrs::hid::usage::keyboard_or_keypad::keyboard_international7},
    {"international8", pqrs::hid::usage::keyboard_or_keypad::keyboard_international8},
    {"international9", pqrs::hid::usage::keyboard_or_keypad::keyboard_international9},
    {"lang1", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang1},
    {"lang2", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang2},
    {"lang3", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang3},
    {"lang4", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang4},
    {"lang5", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang5},
    {"lang6", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang6},
    {"lang7", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang7},
    {"lang8", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang8},
    {"lang9", pqrs::hid::usage::keyboard_or_keypad::keyboard_lang9},
    {"alternate_erase", pqrs::hid::usage::keyboard_or_keypad::keyboard_alternate_erase},
    {"sys_req_or_attention", pqrs::hid::usage::keyboard_or_keypad::keyboard_sys_req_or_attention},
    {"cancel", pqrs::hid::usage::keyboard_or_keypad::keyboard_cancel},
    {"clear", pqrs::hid::usage::keyboard_or_keypad::keyboard_clear},
    {"prior", pqrs::hid::usage::keyboard_or_keypad::keyboard_prior},
    {"return", pqrs::hid::usage::keyboard_or_keypad::keyboard_return},
    {"separator", pqrs::hid::usage::keyboard_or_keypad::keyboard_separator},
    {"out", pqrs::hid::usage::keyboard_or_keypad::keyboard_out},
    {"oper", pqrs::hid::usage::keyboard_or_keypad::keyboard_oper},
    {"clear_or_again", pqrs::hid::usage::keyboard_or_keypad::keyboard_clear_or_again},
    {"cr_sel_or_props", pqrs::hid::usage::keyboard_or_keypad::keyboard_cr_sel_or_props},
    {"ex_sel", pqrs::hid::usage::keyboard_or_keypad::keyboard_ex_sel},
    {"left_control", pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control},
    {"left_shift", pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift},
    {"left_alt", pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt},
    {"left_gui", pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui},
    {"right_control", pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control},
    {"right_shift", pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift},
    {"right_alt", pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt},
    {"right_gui", pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, pqrs::hid::usage::value_t>(name_value_pairs);

constexpr std::pair<const mapbox::eternal::string, const pqrs::hid::usage_pair> other_usage_page_pairs[] = {
    // consumer

    {"mute", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::mute)},
    {"volume_decrement", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::volume_decrement)},
    {"volume_increment", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::volume_increment)},
    {"display_brightness_decrement", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::display_brightness_decrement)},
    {"display_brightness_increment", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::display_brightness_increment)},
    {"eject", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::eject)},
    {"fastforward", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::fast_forward)},
    {"play_or_pause", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::play_or_pause)},
    {"rewind", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::rewind)},
    {"vk_consumer_brightness_down", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::display_brightness_decrement)},
    {"vk_consumer_brightness_up", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::display_brightness_increment)},
    {"vk_consumer_next", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::fast_forward)},
    {"vk_consumer_play", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::play_or_pause)},
    {"vk_consumer_previous", pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer, pqrs::hid::usage::consumer::rewind)},

    // apple_vendor_keyboard

    {"apple_display_brightness_decrement", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_keyboard, pqrs::hid::usage::apple_vendor_keyboard::brightness_down)},
    {"apple_display_brightness_increment", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_keyboard, pqrs::hid::usage::apple_vendor_keyboard::brightness_up)},
    {"dashboard", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_keyboard, pqrs::hid::usage::apple_vendor_keyboard::dashboard)},
    {"launchpad", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_keyboard, pqrs::hid::usage::apple_vendor_keyboard::launchpad)},
    {"mission_control", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_keyboard, pqrs::hid::usage::apple_vendor_keyboard::expose_all)},
    {"vk_dashboard", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_keyboard, pqrs::hid::usage::apple_vendor_keyboard::dashboard)},
    {"vk_launchpad", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_keyboard, pqrs::hid::usage::apple_vendor_keyboard::launchpad)},
    {"vk_mission_control", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_keyboard, pqrs::hid::usage::apple_vendor_keyboard::expose_all)},

    // apple_vendor_top_case

    {"apple_top_case_display_brightness_decrement", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case, pqrs::hid::usage::apple_vendor_top_case::brightness_down)},
    {"apple_top_case_display_brightness_increment", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case, pqrs::hid::usage::apple_vendor_top_case::brightness_up)},
    {"fn", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case, pqrs::hid::usage::apple_vendor_top_case::keyboard_fn)},
    {"illumination_decrement", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case, pqrs::hid::usage::apple_vendor_top_case::illumination_down)},
    {"illumination_increment", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case, pqrs::hid::usage::apple_vendor_top_case::illumination_up)},
    {"vk_consumer_illumination_down", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case, pqrs::hid::usage::apple_vendor_top_case::illumination_down)},
    {"vk_consumer_illumination_up", pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case, pqrs::hid::usage::apple_vendor_top_case::illumination_up)},
};

constexpr auto other_usage_page_map = mapbox::eternal::hash_map<mapbox::eternal::string, pqrs::hid::usage_pair>(other_usage_page_pairs);

inline bool target(pqrs::hid::usage::value_t usage) {
  return pqrs::hid::usage::keyboard_or_keypad::keyboard_a <= usage &&
         usage < pqrs::hid::usage::keyboard_or_keypad::reserved;
}

inline std::string make_name(pqrs::hid::usage::value_t usage) {
  return impl::make_name(name_value_pairs, usage);
}

inline pqrs::hid::usage_pair make_usage_pair(const std::string& key,
                                             const nlohmann::json& json) {
  // Extra keys and aliases
  if (json.is_string()) {
    auto name = json.get<std::string>();
    auto it = other_usage_page_map.find(name.c_str());
    if (it != std::end(other_usage_page_map)) {
      return it->second;
    }
  }

  return impl::make_usage_pair(name_value_map,
                               pqrs::hid::usage_page::keyboard_or_keypad,
                               key,
                               json);
}
} // namespace key_code
} // namespace momentary_switch_event_details
} // namespace krbn
