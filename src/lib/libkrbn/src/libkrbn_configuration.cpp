#include "complex_modifications_utility.hpp"
#include "libkrbn/impl/libkrbn_configuration_monitor.hpp"
#include "libkrbn/impl/libkrbn_cpp.hpp"

namespace {
std::string save_error_message;

krbn::core_configuration::details::simple_modifications* find_simple_modifications(libkrbn_core_configuration* p,
                                                                                   const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      return c->get_core_configuration().get_selected_profile().find_simple_modifications(libkrbn_cpp::make_device_identifiers(*device_identifiers));
    } else {
      return &(c->get_core_configuration().get_selected_profile().get_simple_modifications());
    }
  }
  return nullptr;
}

krbn::core_configuration::details::simple_modifications* find_fn_function_keys(libkrbn_core_configuration* p,
                                                                               const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      return c->get_core_configuration().get_selected_profile().find_fn_function_keys(libkrbn_cpp::make_device_identifiers(*device_identifiers));
    } else {
      return &(c->get_core_configuration().get_selected_profile().get_fn_function_keys());
    }
  }
  return nullptr;
}
} // namespace

void libkrbn_core_configuration_terminate(libkrbn_core_configuration** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_core_configuration_class*>(*p);
    *p = nullptr;
  }
}

void libkrbn_core_configuration_save(libkrbn_core_configuration* p) {
  try {
    if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
      c->get_core_configuration().sync_save_to_file();
    }
    save_error_message.clear();
  } catch (const std::exception& e) {
    save_error_message = e.what();
  }
}

const char* libkrbn_core_configuration_get_save_error_message(void) {
  return save_error_message.c_str();
}

bool libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().get_check_for_updates_on_startup();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_global_configuration().set_check_for_updates_on_startup(value);
  }
}

bool libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().get_show_in_menu_bar();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().set_show_in_menu_bar(value);
  }
}

bool libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().get_show_profile_name_in_menu_bar();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().set_show_profile_name_in_menu_bar(value);
  }
}

bool libkrbn_core_configuration_get_global_configuration_ask_for_confirmation_before_quitting(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().get_ask_for_confirmation_before_quitting();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_ask_for_confirmation_before_quitting(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().set_ask_for_confirmation_before_quitting(value);
  }
}

bool libkrbn_core_configuration_get_global_configuration_unsafe_ui(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().get_unsafe_ui();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_unsafe_ui(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().set_unsafe_ui(value);
  }
}

size_t libkrbn_core_configuration_get_profiles_size(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_profiles().size();
  }
  return 0;
}

const char* libkrbn_core_configuration_get_profile_name(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    const auto& profiles = c->get_core_configuration().get_profiles();
    if (index < profiles.size()) {
      return profiles[index].get_name().c_str();
    }
  }
  return nullptr;
}

void libkrbn_core_configuration_set_profile_name(libkrbn_core_configuration* p, size_t index, const char* value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (value) {
      c->get_core_configuration().set_profile_name(index, value);
    }
  }
}

bool libkrbn_core_configuration_get_profile_selected(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    const auto& profiles = c->get_core_configuration().get_profiles();
    if (index < profiles.size()) {
      return profiles[index].get_selected();
    }
  }
  return false;
}

void libkrbn_core_configuration_select_profile(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().select_profile(index);
  }
}

const char* libkrbn_core_configuration_get_selected_profile_name(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_selected_profile().get_name().c_str();
  }
  return nullptr;
}

void libkrbn_core_configuration_push_back_profile(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().push_back_profile();
  }
}

void libkrbn_core_configuration_erase_profile(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().erase_profile(index);
  }
}

int libkrbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    auto count = c->get_core_configuration().get_selected_profile().get_parameters().get_delay_milliseconds_before_open_device().count();
    return static_cast<int>(count);
  }
  return 0;
}

void libkrbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(libkrbn_core_configuration* p, int value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().get_parameters().set_delay_milliseconds_before_open_device(
        std::chrono::milliseconds(value));
  }
}

size_t libkrbn_core_configuration_get_selected_profile_simple_modifications_size(libkrbn_core_configuration* p,
                                                                                 const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    return m->get_pairs().size();
  }
  return 0;
}

const char* libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(libkrbn_core_configuration* p,
                                                                                                 size_t index,
                                                                                                 const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    const auto& pairs = m->get_pairs();
    if (index < pairs.size()) {
      return pairs[index].first.c_str();
    }
  }
  return nullptr;
}

const char* libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(libkrbn_core_configuration* p,
                                                                                               size_t index,
                                                                                               const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    const auto& pairs = m->get_pairs();
    if (index < pairs.size()) {
      return pairs[index].second.c_str();
    }
  }
  return nullptr;
}

void libkrbn_core_configuration_replace_selected_profile_simple_modification(libkrbn_core_configuration* p,
                                                                             size_t index,
                                                                             const char* from_json_string,
                                                                             const char* to_json_string,
                                                                             const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    if (from_json_string &&
        to_json_string) {
      m->replace_pair(index, from_json_string, to_json_string);
    }
  }
}

void libkrbn_core_configuration_push_back_selected_profile_simple_modification(libkrbn_core_configuration* p,
                                                                               const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    m->push_back_pair();
  }
}

void libkrbn_core_configuration_erase_selected_profile_simple_modification(libkrbn_core_configuration* p,
                                                                           size_t index,
                                                                           const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    m->erase_pair(index);
  }
}

size_t libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(libkrbn_core_configuration* p,
                                                                             const libkrbn_device_identifiers* device_identifiers) {
  if (auto k = find_fn_function_keys(p, device_identifiers)) {
    return k->get_pairs().size();
  }
  return 0;
}

const char* libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(libkrbn_core_configuration* p,
                                                                                             size_t index,
                                                                                             const libkrbn_device_identifiers* device_identifiers) {
  if (auto k = find_fn_function_keys(p, device_identifiers)) {
    const auto& pairs = k->get_pairs();
    if (index < pairs.size()) {
      return pairs[index].first.c_str();
    }
  }
  return nullptr;
}

const char* libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(libkrbn_core_configuration* p,
                                                                                           size_t index,
                                                                                           const libkrbn_device_identifiers* device_identifiers) {
  if (auto k = find_fn_function_keys(p, device_identifiers)) {
    const auto& pairs = k->get_pairs();
    if (index < pairs.size()) {
      return pairs[index].second.c_str();
    }
  }
  return nullptr;
}

void libkrbn_core_configuration_replace_selected_profile_fn_function_key(libkrbn_core_configuration* p,
                                                                         const char* from_json_string,
                                                                         const char* to_json_string,
                                                                         const libkrbn_device_identifiers* device_identifiers) {
  if (auto k = find_fn_function_keys(p, device_identifiers)) {
    if (from_json_string &&
        to_json_string) {
      k->replace_second(from_json_string, to_json_string);
    }
  }
}

size_t libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_selected_profile().get_complex_modifications().get_rules().size();
  }
  return 0;
}

void libkrbn_core_configuration_push_back_complex_modifications_rule_to_selected_profile(libkrbn_core_configuration* p,
                                                                                         const krbn::core_configuration::details::complex_modifications_rule& rule) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().push_back_complex_modifications_rule(rule);
  }
}

void libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().erase_complex_modifications_rule(index);
  }
}

void libkrbn_core_configuration_swap_selected_profile_complex_modifications_rules(libkrbn_core_configuration* p, size_t index1, size_t index2) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().swap_complex_modifications_rules(index1, index2);
  }
}

void libkrbn_core_configuration_move_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p, size_t source_index, size_t destination_index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().move_complex_modifications_rule(source_index, destination_index);
  }
}

const char* _Nullable libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    const auto& rules = c->get_core_configuration().get_selected_profile().get_complex_modifications().get_rules();
    if (index < rules.size()) {
      return rules[index].get_description().c_str();
    }
  }
  return 0;
}

bool libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_json_string(libkrbn_core_configuration* p,
                                                                                            size_t index,
                                                                                            char* buffer,
                                                                                            size_t length) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    const auto& rules = c->get_core_configuration().get_selected_profile().get_complex_modifications().get_rules();
    if (index < rules.size()) {
      auto json_string = krbn::json_utility::dump(rules[index].get_json());
      // Return false if no enough space.
      if (json_string.length() < length) {
        strlcpy(buffer, json_string.c_str(), length);
        return true;
      }
    }
  }
  return false;
}

void libkrbn_core_configuration_replace_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p,
                                                                                    size_t index,
                                                                                    const char* json_string,
                                                                                    char* error_message_buffer,
                                                                                    size_t error_message_buffer_length) {
  if (error_message_buffer_length > 0) {
    error_message_buffer[0] = '\0';
  }

  try {
    if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
      auto complex_modifications = c->get_core_configuration().get_selected_profile().get_complex_modifications();
      krbn::core_configuration::details::complex_modifications_rule rule(
          krbn::json_utility::parse_jsonc(std::string(json_string)),
          complex_modifications.get_parameters());

      auto error_messages = krbn::complex_modifications_utility::lint_rule(rule);
      if (error_messages.size() > 0) {
        std::ostringstream os;
        std::copy(std::begin(error_messages),
                  std::end(error_messages),
                  std::ostream_iterator<std::string>(os, "\n"));
        strlcpy(error_message_buffer,
                pqrs::string::trim_copy(os.str()).c_str(),
                error_message_buffer_length);
        return;
      }

      complex_modifications.replace_rule(index, rule);
    }
  } catch (const std::exception& e) {
    auto message = fmt::format("error: {0}", e.what());
    strlcpy(error_message_buffer, message.c_str(), error_message_buffer_length);
  }
}

int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* p,
                                                                                    const char* name) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (name) {
      if (auto value = c->get_core_configuration().get_selected_profile().get_complex_modifications().get_parameters().get_value(name)) {
        return *value;
      }
    }
  }
  return 0;
}

void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* p,
                                                                                     const char* name,
                                                                                     int value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (name) {
      c->get_core_configuration().get_selected_profile().set_complex_modifications_parameter(name, value);
    }
  }
}

uint8_t libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return static_cast<uint8_t>(type_safe::get(c->get_core_configuration()
                                                   .get_selected_profile()
                                                   .get_virtual_hid_keyboard()
                                                   .get_country_code()));
  }
  return 0;
}

void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_country_code(libkrbn_core_configuration* p, uint8_t value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration()
        .get_selected_profile()
        .get_virtual_hid_keyboard()
        .set_country_code(pqrs::hid::country_code::value_t(value));
  }
}

int libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration()
        .get_selected_profile()
        .get_virtual_hid_keyboard()
        .get_mouse_key_xy_scale();
  }
  return 0;
}

void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(libkrbn_core_configuration* p, int value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration()
        .get_selected_profile()
        .get_virtual_hid_keyboard()
        .set_mouse_key_xy_scale(value);
  }
}

bool libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration()
        .get_selected_profile()
        .get_virtual_hid_keyboard()
        .get_indicate_sticky_modifier_keys_state();
  }
  return true;
}

void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration()
        .get_selected_profile()
        .get_virtual_hid_keyboard()
        .set_indicate_sticky_modifier_keys_state(value);
  }
}

bool libkrbn_core_configuration_get_selected_profile_device_ignore(libkrbn_core_configuration* p,
                                                                   const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      return c->get_core_configuration().get_selected_profile().get_device_ignore(identifiers);
    }
  }
  return false;
}

void libkrbn_core_configuration_set_selected_profile_device_ignore(libkrbn_core_configuration* p,
                                                                   const libkrbn_device_identifiers* device_identifiers,
                                                                   bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      c->get_core_configuration().get_selected_profile().set_device_ignore(identifiers, value);
    }
  }
}

bool libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(libkrbn_core_configuration* p,
                                                                                     const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      return c->get_core_configuration().get_selected_profile().get_device_manipulate_caps_lock_led(identifiers);
    }
  }
  return false;
}

void libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(libkrbn_core_configuration* p,
                                                                                     const libkrbn_device_identifiers* device_identifiers,
                                                                                     bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      c->get_core_configuration().get_selected_profile().set_device_manipulate_caps_lock_led(identifiers, value);
    }
  }
}

bool libkrbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(libkrbn_core_configuration* p,
                                                                                       const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      return c->get_core_configuration().get_selected_profile().get_device_treat_as_built_in_keyboard(identifiers);
    }
  }
  return false;
}

void libkrbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(libkrbn_core_configuration* p,
                                                                                       const libkrbn_device_identifiers* device_identifiers,
                                                                                       bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      c->get_core_configuration().get_selected_profile().set_device_treat_as_built_in_keyboard(identifiers, value);
    }
  }
}

bool libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* p,
                                                                                                const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      return c->get_core_configuration().get_selected_profile().get_device_disable_built_in_keyboard_if_exists(identifiers);
    }
  }
  return false;
}

void libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* p,
                                                                                                const libkrbn_device_identifiers* device_identifiers,
                                                                                                bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      c->get_core_configuration().get_selected_profile().set_device_disable_built_in_keyboard_if_exists(identifiers, value);
    }
  }
}
