#include "complex_modifications_utility.hpp"
#include "connected_devices.hpp"
#include "duktape_utility.hpp"
#include "settings_components_manager.hpp"
#include "settings_configuration_monitor.hpp"
#include "settings_cpp.hpp"

namespace {
auto empty_core_configuration = gsl::make_not_null(std::make_shared<krbn::core_configuration::core_configuration>());
auto empty_device = gsl::make_not_null(std::make_shared<krbn::core_configuration::details::device>());

[[nodiscard]] pqrs::not_null_shared_ptr_t<krbn::core_configuration::core_configuration> get_current_core_configuration() {
  if (auto manager = settings_cpp::get_components_manager()) {
    if (auto c = manager->get_current_core_configuration()) {
      return c;
    }
  }

  return empty_core_configuration;
}

[[nodiscard]] pqrs::not_null_shared_ptr_t<krbn::core_configuration::details::simple_modifications> find_simple_modifications(const krbn_device_identifiers* device_identifiers) {
  auto di = settings_cpp::make_device_identifiers(device_identifiers);
  auto c = get_current_core_configuration();
  if (di.empty()) {
    return c->get_selected_profile().get_simple_modifications();
  } else {
    auto d = c->get_selected_profile().get_device(di);
    return d->get_simple_modifications();
  }
}

[[nodiscard]] pqrs::not_null_shared_ptr_t<krbn::core_configuration::details::simple_modifications> find_fn_function_keys(const krbn_device_identifiers* device_identifiers) {
  auto di = settings_cpp::make_device_identifiers(device_identifiers);
  auto c = get_current_core_configuration();
  if (di.empty()) {
    return c->get_selected_profile().get_fn_function_keys();
  } else {
    auto d = c->get_selected_profile().get_device(di);
    return d->get_fn_function_keys();
  }
}
} // namespace

bool krbn_core_configuration_save(char* error_message_buffer,
                                  size_t error_message_buffer_length) {
  if (error_message_buffer && error_message_buffer_length > 0) {
    error_message_buffer[0] = '\0';
  }

  if (auto manager = settings_cpp::get_components_manager()) {
    if (auto m = manager->get_settings_configuration_monitor()) {
      if (auto c = m->get_weak_core_configuration().lock()) {
        try {
          c->sync_save_to_file();
          return true;
        } catch (const std::exception& e) {
          strlcpy(error_message_buffer, e.what(), error_message_buffer_length);
          return false;
        }
      }
    }
  }

  strlcpy(error_message_buffer, "core_configuration is not ready", error_message_buffer_length);
  return false;
}

bool krbn_core_configuration_get_global_configuration_check_for_updates() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_check_for_updates();
}

void krbn_core_configuration_set_global_configuration_check_for_updates(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_check_for_updates(value);
}

bool krbn_core_configuration_get_global_configuration_show_in_menu_bar() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_show_in_menu_bar();
}

void krbn_core_configuration_set_global_configuration_show_in_menu_bar(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_show_in_menu_bar(value);
}

bool krbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_show_profile_name_in_menu_bar();
}

void krbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_show_profile_name_in_menu_bar(value);
}

bool krbn_core_configuration_get_global_configuration_show_additional_menu_items() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_show_additional_menu_items();
}

void krbn_core_configuration_set_global_configuration_show_additional_menu_items(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_show_additional_menu_items(value);
}

bool krbn_core_configuration_get_global_configuration_enable_notification_window() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_enable_notification_window();
}

void krbn_core_configuration_set_global_configuration_enable_notification_window(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_enable_notification_window(value);
}

bool krbn_core_configuration_get_global_configuration_unsafe_ui() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_unsafe_ui();
}

void krbn_core_configuration_set_global_configuration_unsafe_ui(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_unsafe_ui(value);
}

bool krbn_core_configuration_get_global_configuration_filter_useless_events_from_specific_devices() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_filter_useless_events_from_specific_devices();
}

void krbn_core_configuration_set_global_configuration_filter_useless_events_from_specific_devices(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_filter_useless_events_from_specific_devices(value);
}

bool krbn_core_configuration_get_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_reorder_same_timestamp_input_events_to_prioritize_modifiers();
}

void krbn_core_configuration_set_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_reorder_same_timestamp_input_events_to_prioritize_modifiers(value);
}

bool krbn_core_configuration_get_global_configuration_enable_cgeventtap_fallback() {
  auto c = get_current_core_configuration();
  return c->get_global_configuration().get_enable_cgeventtap_fallback();
}

void krbn_core_configuration_set_global_configuration_enable_cgeventtap_fallback(bool value) {
  auto c = get_current_core_configuration();
  c->get_global_configuration().set_enable_cgeventtap_fallback(value);
}

bool krbn_core_configuration_get_machine_specific_enable_multitouch_extension() {
  auto c = get_current_core_configuration();
  return c->get_machine_specific().get_entry().get_enable_multitouch_extension();
}

void krbn_core_configuration_set_machine_specific_enable_multitouch_extension(bool value) {
  auto c = get_current_core_configuration();
  c->get_machine_specific().get_entry().set_enable_multitouch_extension(value);
}

void krbn_core_configuration_get_machine_specific_external_editor_path(char* buffer,
                                                                       size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  const auto& value = c->get_machine_specific().get_entry().get_external_editor_path();
  strlcpy(buffer, value.c_str(), length);
}

void krbn_core_configuration_set_machine_specific_external_editor_path(const char* value) {
  if (value) {
    auto c = get_current_core_configuration();
    c->get_machine_specific().get_entry().set_external_editor_path(value);
  }
}

size_t krbn_core_configuration_get_profiles_size() {
  auto c = get_current_core_configuration();
  return c->get_profiles().size();
}

bool krbn_core_configuration_get_profile_name(size_t index,
                                              char* buffer,
                                              size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  const auto& profiles = c->get_profiles();
  if (index < profiles.size()) {
    strlcpy(buffer, profiles[index]->get_name().c_str(), length);
    return true;
  }

  return false;
}

void krbn_core_configuration_set_profile_name(size_t index, const char* value) {
  if (value) {
    auto c = get_current_core_configuration();
    c->set_profile_name(index, value);
  }
}

bool krbn_core_configuration_get_profile_selected(size_t index) {
  auto c = get_current_core_configuration();
  const auto& profiles = c->get_profiles();
  if (index < profiles.size()) {
    return profiles[index]->get_selected();
  }
  return false;
}

void krbn_core_configuration_select_profile(size_t index) {
  auto c = get_current_core_configuration();
  c->select_profile(index);
}

bool krbn_core_configuration_get_selected_profile_name(char* buffer,
                                                       size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  strlcpy(buffer, c->get_selected_profile().get_name().c_str(), length);
  return true;
}

void krbn_core_configuration_push_back_profile() {
  auto c = get_current_core_configuration();
  c->push_back_profile();
}

void krbn_core_configuration_duplicate_profile(size_t source_index) {
  auto c = get_current_core_configuration();
  const auto& profiles = c->get_profiles();
  if (source_index < profiles.size()) {
    c->duplicate_profile(*(profiles[source_index]));
  }
}

void krbn_core_configuration_move_profile(size_t source_index, size_t destination_index) {
  auto c = get_current_core_configuration();
  c->move_profile(source_index, destination_index);
}

void krbn_core_configuration_erase_profile(size_t index) {
  auto c = get_current_core_configuration();
  c->erase_profile(index);
}

int krbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device() {
  auto c = get_current_core_configuration();
  auto count = c->get_selected_profile().get_parameters()->get_delay_milliseconds_before_open_device().count();
  return static_cast<int>(count);
}

void krbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(int value) {
  auto c = get_current_core_configuration();
  c->get_selected_profile().get_parameters()->set_delay_milliseconds_before_open_device(
      std::chrono::milliseconds(value));
}

size_t krbn_core_configuration_get_selected_profile_simple_modifications_size(const krbn_device_identifiers* device_identifiers) {
  auto m = find_simple_modifications(device_identifiers);
  return m->get_pairs().size();
}

bool krbn_core_configuration_get_selected_profile_simple_modification_from_json_string(size_t index,
                                                                                       const krbn_device_identifiers* device_identifiers,
                                                                                       char* buffer,
                                                                                       size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto m = find_simple_modifications(device_identifiers);
  const auto& pairs = m->get_pairs();
  if (index < pairs.size()) {
    strlcpy(buffer, pairs[index].first.c_str(), length);
    return true;
  }

  return false;
}

bool krbn_core_configuration_get_selected_profile_simple_modification_to_json_string(size_t index,
                                                                                     const krbn_device_identifiers* device_identifiers,
                                                                                     char* buffer,
                                                                                     size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto m = find_simple_modifications(device_identifiers);
  const auto& pairs = m->get_pairs();
  if (index < pairs.size()) {
    strlcpy(buffer, pairs[index].second.c_str(), length);
    return true;
  }

  return false;
}

void krbn_core_configuration_replace_selected_profile_simple_modification(size_t index,
                                                                          const char* from_json_string,
                                                                          const char* to_json_string,
                                                                          const krbn_device_identifiers* device_identifiers) {
  auto m = find_simple_modifications(device_identifiers);
  if (from_json_string &&
      to_json_string) {
    m->replace_pair(index, from_json_string, to_json_string);
  }
}

void krbn_core_configuration_push_back_selected_profile_simple_modification(const krbn_device_identifiers* device_identifiers) {
  auto m = find_simple_modifications(device_identifiers);
  m->push_back_pair();
}

void krbn_core_configuration_erase_selected_profile_simple_modification(size_t index,
                                                                        const krbn_device_identifiers* device_identifiers) {
  auto m = find_simple_modifications(device_identifiers);
  m->erase_pair(index);
}

size_t krbn_core_configuration_get_selected_profile_fn_function_keys_size(const krbn_device_identifiers* device_identifiers) {
  auto k = find_fn_function_keys(device_identifiers);
  return k->get_pairs().size();
}

bool krbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(size_t index,
                                                                                   const krbn_device_identifiers* device_identifiers,
                                                                                   char* buffer,
                                                                                   size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto k = find_fn_function_keys(device_identifiers);
  const auto& pairs = k->get_pairs();
  if (index < pairs.size()) {
    strlcpy(buffer, pairs[index].first.c_str(), length);
    return true;
  }

  return false;
}

bool krbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(size_t index,
                                                                                 const krbn_device_identifiers* device_identifiers,
                                                                                 char* buffer,
                                                                                 size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto k = find_fn_function_keys(device_identifiers);
  const auto& pairs = k->get_pairs();
  if (index < pairs.size()) {
    strlcpy(buffer, pairs[index].second.c_str(), length);
    return true;
  }

  return false;
}

void krbn_core_configuration_replace_selected_profile_fn_function_key(const char* from_json_string,
                                                                      const char* to_json_string,
                                                                      const krbn_device_identifiers* device_identifiers) {
  auto k = find_fn_function_keys(device_identifiers);
  if (from_json_string &&
      to_json_string) {
    k->replace_second(from_json_string, to_json_string);
  }
}

size_t krbn_core_configuration_get_selected_profile_complex_modifications_rules_size() {
  auto c = get_current_core_configuration();
  return c->get_selected_profile().get_complex_modifications()->get_rules().size();
}

void krbn_core_configuration_erase_selected_profile_complex_modifications_rule(size_t index) {
  auto c = get_current_core_configuration();
  c->get_selected_profile().get_complex_modifications()->erase_rule(index);
}

void krbn_core_configuration_move_selected_profile_complex_modifications_rule(size_t source_index, size_t destination_index) {
  auto c = get_current_core_configuration();
  c->get_selected_profile().get_complex_modifications()->move_rule(source_index, destination_index);
}

bool krbn_core_configuration_get_selected_profile_complex_modifications_rule_description(size_t index,
                                                                                         char* buffer,
                                                                                         size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  const auto& rules = c->get_selected_profile().get_complex_modifications()->get_rules();
  if (index < rules.size()) {
    strlcpy(buffer, rules[index]->get_description().c_str(), length);
    return true;
  }

  return false;
}

bool krbn_core_configuration_get_selected_profile_complex_modifications_rule_enabled(size_t index) {
  auto c = get_current_core_configuration();
  const auto& rules = c->get_selected_profile().get_complex_modifications()->get_rules();
  if (index < rules.size()) {
    return rules[index]->get_enabled();
  }

  return false;
}

void krbn_core_configuration_set_selected_profile_complex_modifications_rule_enabled(size_t index, bool value) {
  auto c = get_current_core_configuration();
  const auto& rules = c->get_selected_profile().get_complex_modifications()->get_rules();
  if (index < rules.size()) {
    rules[index]->set_enabled(value);
  }
}

bool krbn_core_configuration_get_selected_profile_complex_modifications_rule_code_string(size_t index,
                                                                                         char* _Nonnull buffer,
                                                                                         size_t length,
                                                                                         krbn_complex_modifications_rule_code_type* _Nonnull code_type) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }
  if (code_type) {
    *code_type = krbn_complex_modifications_rule_code_type_json;
  }

  auto c = get_current_core_configuration();
  const auto& rules = c->get_selected_profile().get_complex_modifications()->get_rules();
  if (index < rules.size()) {
    if (code_type) {
      switch (rules[index]->get_code_type()) {
        case krbn::core_configuration::details::complex_modifications_rule::code_type::json:
          *code_type = krbn_complex_modifications_rule_code_type_json;
          break;
        case krbn::core_configuration::details::complex_modifications_rule::code_type::javascript:
          *code_type = krbn_complex_modifications_rule_code_type_javascript;
          break;
      }
    }

    auto code_string = rules[index]->get_code_string();
    if (code_string.length() < length) {
      strlcpy(buffer, code_string.c_str(), length);
      return true;
    }
  }

  return false;
}

bool krbn_core_configuration_get_selected_profile_complex_modifications_rule_search_text(size_t index,
                                                                                         char* buffer,
                                                                                         size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  const auto& rules = c->get_selected_profile().get_complex_modifications()->get_rules();
  if (index < rules.size()) {
    const auto& search_text = rules[index]->get_search_text();
    if (search_text.length() < length) {
      strlcpy(buffer, search_text.c_str(), length);
      return true;
    }
  }

  return false;
}

namespace {
krbn::core_configuration::details::complex_modifications_rule::code_type to_code_type(krbn_complex_modifications_rule_code_type type) {
  switch (type) {
    case krbn_complex_modifications_rule_code_type_javascript:
      return krbn::core_configuration::details::complex_modifications_rule::code_type::javascript;
    default:
      return krbn::core_configuration::details::complex_modifications_rule::code_type::json;
  }
}
} // namespace

void krbn_core_configuration_replace_selected_profile_complex_modifications_rule(size_t index,
                                                                                 const char* code_string,
                                                                                 krbn_complex_modifications_rule_code_type code_type,
                                                                                 char* error_message_buffer,
                                                                                 size_t error_message_buffer_length) {
  if (error_message_buffer && error_message_buffer_length > 0) {
    error_message_buffer[0] = '\0';
  }

  try {
    auto c = get_current_core_configuration();
    auto m = c->get_selected_profile().get_complex_modifications();
    auto r = std::make_shared<krbn::core_configuration::details::complex_modifications_rule>(
        code_string,
        to_code_type(code_type),
        m->get_parameters(),
        krbn::core_configuration::error_handling::strict);

    auto error_messages = krbn::complex_modifications_utility::lint_rule(*r);
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

    m->replace_rule(index, r);

  } catch (const std::exception& e) {
    auto message = fmt::format("error: {0}", e.what());
    strlcpy(error_message_buffer, message.c_str(), error_message_buffer_length);
  }
}

void krbn_core_configuration_push_front_selected_profile_complex_modifications_rule(const char* code_string,
                                                                                    krbn_complex_modifications_rule_code_type code_type,
                                                                                    char* error_message_buffer,
                                                                                    size_t error_message_buffer_length) {
  if (error_message_buffer && error_message_buffer_length > 0) {
    error_message_buffer[0] = '\0';
  }

  try {
    auto c = get_current_core_configuration();
    auto m = c->get_selected_profile().get_complex_modifications();

    auto r = std::make_shared<krbn::core_configuration::details::complex_modifications_rule>(
        code_string,
        to_code_type(code_type),
        m->get_parameters(),
        krbn::core_configuration::error_handling::strict);

    auto error_messages = krbn::complex_modifications_utility::lint_rule(*r);
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

    m->push_front_rule(r);

  } catch (const std::exception& e) {
    auto message = fmt::format("error: {0}", e.what());
    strlcpy(error_message_buffer, message.c_str(), error_message_buffer_length);
  }
}

//
// basic_simultaneous_threshold_milliseconds
//

int krbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds() {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  return p->get_basic_simultaneous_threshold_milliseconds();
}

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds(int value) {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  p->set_basic_simultaneous_threshold_milliseconds(value);
}

//
// basic_to_if_alone_timeout_milliseconds
//

int krbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds() {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  return p->get_basic_to_if_alone_timeout_milliseconds();
}

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds(int value) {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  p->set_basic_to_if_alone_timeout_milliseconds(value);
}

//
// basic_to_if_held_down_threshold_milliseconds
//

int krbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds() {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  return p->get_basic_to_if_held_down_threshold_milliseconds();
}

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds(int value) {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  p->set_basic_to_if_held_down_threshold_milliseconds(value);
}

//
// basic_to_delayed_action_delay_milliseconds
//

int krbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds() {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  return p->get_basic_to_delayed_action_delay_milliseconds();
}

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds(int value) {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  p->set_basic_to_delayed_action_delay_milliseconds(value);
}

//
// mouse_motion_to_scroll_speed
//

int krbn_core_configuration_get_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed() {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  return p->get_mouse_motion_to_scroll_speed();
}

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed(int value) {
  auto c = get_current_core_configuration();
  auto p = c->get_selected_profile().get_complex_modifications()->get_parameters();
  p->set_mouse_motion_to_scroll_speed(value);
}

void krbn_core_configuration_get_new_complex_modifications_rule_json_string(char* buffer,
                                                                            size_t length) {
  auto json_string = krbn::complex_modifications_utility::get_new_rule_json_string();
  strlcpy(buffer, json_string.c_str(), length);
}

void krbn_core_configuration_get_new_complex_modifications_rule_eval_js_string(char* buffer,
                                                                               size_t length) {
  auto code_string = krbn::complex_modifications_utility::get_new_rule_eval_js_string();
  strlcpy(buffer, code_string.c_str(), length);
}

bool krbn_eval_js_to_json_string(const char* code,
                                 char* buffer,
                                 size_t length,
                                 char* log_message_buffer,
                                 size_t log_message_buffer_length,
                                 char* error_message_buffer,
                                 size_t error_message_buffer_length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }
  if (log_message_buffer && log_message_buffer_length > 0) {
    log_message_buffer[0] = '\0';
  }
  if (error_message_buffer && error_message_buffer_length > 0) {
    error_message_buffer[0] = '\0';
  }

  if (!code) {
    strlcpy(error_message_buffer, "error: invalid javascript code", error_message_buffer_length);
    return false;
  }

  try {
    auto result = krbn::duktape_utility::eval_string_to_json(std::string(code));

    strlcpy(log_message_buffer, result.log_messages.c_str(), log_message_buffer_length);

    auto json_string = krbn::json_utility::dump(result.json);
    if (json_string.length() < length) {
      strlcpy(buffer, json_string.c_str(), length);
      return true;
    }

    strlcpy(error_message_buffer,
            "error: output buffer is too small",
            error_message_buffer_length);
  } catch (const std::exception& e) {
    auto message = fmt::format("error: {0}", e.what());
    strlcpy(error_message_buffer, message.c_str(), error_message_buffer_length);
  }

  return false;
}

void krbn_core_configuration_get_selected_profile_virtual_hid_keyboard_keyboard_type_v2(char* buffer,
                                                                                        size_t length) {
  auto c = get_current_core_configuration();
  strlcpy(buffer,
          c->get_selected_profile()
              .get_virtual_hid_keyboard()
              ->get_keyboard_type_v2()
              .c_str(),
          length);
}

void krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type_v2(const char* value) {
  auto c = get_current_core_configuration();
  c->get_selected_profile()
      .get_virtual_hid_keyboard()
      ->set_keyboard_type_v2(value);
}

int krbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale() {
  auto c = get_current_core_configuration();
  return c->get_selected_profile()
      .get_virtual_hid_keyboard()
      ->get_mouse_key_xy_scale();
}

void krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(int value) {
  auto c = get_current_core_configuration();
  c->get_selected_profile()
      .get_virtual_hid_keyboard()
      ->set_mouse_key_xy_scale(value);
}

bool krbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state() {
  auto c = get_current_core_configuration();
  return c->get_selected_profile()
      .get_virtual_hid_keyboard()
      ->get_indicate_sticky_modifier_keys_state();
}

void krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(bool value) {
  auto c = get_current_core_configuration();
  c->get_selected_profile()
      .get_virtual_hid_keyboard()
      ->set_indicate_sticky_modifier_keys_state(value);
}

bool krbn_core_configuration_get_selected_profile_device_ignore(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_ignore();
}

void krbn_core_configuration_set_selected_profile_device_ignore(const krbn_device_identifiers* device_identifiers,
                                                                bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_ignore(value);
}

bool krbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_manipulate_caps_lock_led();
}

void krbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(const krbn_device_identifiers* device_identifiers,
                                                                                  bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_manipulate_caps_lock_led(value);
}

bool krbn_core_configuration_get_selected_profile_device_ignore_vendor_events(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_ignore_vendor_events();
}

void krbn_core_configuration_set_selected_profile_device_ignore_vendor_events(const krbn_device_identifiers* device_identifiers,
                                                                              bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_ignore_vendor_events(value);
}

bool krbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_treat_as_built_in_keyboard();
}

void krbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(const krbn_device_identifiers* device_identifiers,
                                                                                    bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_treat_as_built_in_keyboard(value);
}

bool krbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_disable_built_in_keyboard_if_exists();
}

void krbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(const krbn_device_identifiers* device_identifiers,
                                                                                             bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_disable_built_in_keyboard_if_exists(value);
}

double krbn_core_configuration_get_selected_profile_device_pointing_motion_xy_multiplier(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_pointing_motion_xy_multiplier();
}

void krbn_core_configuration_set_selected_profile_device_pointing_motion_xy_multiplier(const krbn_device_identifiers* device_identifiers,
                                                                                       double value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_pointing_motion_xy_multiplier(value);
}

double krbn_core_configuration_pointing_motion_xy_multiplier_default_value() {
  return empty_device->find_default_value(
      empty_device->get_pointing_motion_xy_multiplier());
}

double krbn_core_configuration_get_selected_profile_device_pointing_motion_wheels_multiplier(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_pointing_motion_wheels_multiplier();
}

void krbn_core_configuration_set_selected_profile_device_pointing_motion_wheels_multiplier(const krbn_device_identifiers* device_identifiers,
                                                                                           double value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_pointing_motion_wheels_multiplier(value);
}

double krbn_core_configuration_pointing_motion_wheels_multiplier_default_value() {
  return empty_device->find_default_value(
      empty_device->get_pointing_motion_wheels_multiplier());
}

//
// mouse_flip_XXX
//

bool krbn_core_configuration_get_selected_profile_device_mouse_flip_x(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_flip_x();
}

void krbn_core_configuration_set_selected_profile_device_mouse_flip_x(const krbn_device_identifiers* device_identifiers,
                                                                      bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_flip_x(value);
}

bool krbn_core_configuration_get_selected_profile_device_mouse_flip_y(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_flip_y();
}

void krbn_core_configuration_set_selected_profile_device_mouse_flip_y(const krbn_device_identifiers* device_identifiers,
                                                                      bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_flip_y(value);
}

bool krbn_core_configuration_get_selected_profile_device_mouse_flip_vertical_wheel(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_flip_vertical_wheel();
}

void krbn_core_configuration_set_selected_profile_device_mouse_flip_vertical_wheel(const krbn_device_identifiers* device_identifiers,
                                                                                   bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_flip_vertical_wheel(value);
}

bool krbn_core_configuration_get_selected_profile_device_mouse_flip_horizontal_wheel(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_flip_horizontal_wheel();
}

void krbn_core_configuration_set_selected_profile_device_mouse_flip_horizontal_wheel(const krbn_device_identifiers* device_identifiers,
                                                                                     bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_flip_horizontal_wheel(value);
}

//
// mouse_discard_XXX
//

bool krbn_core_configuration_get_selected_profile_device_mouse_discard_x(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_discard_x();
}

void krbn_core_configuration_set_selected_profile_device_mouse_discard_x(const krbn_device_identifiers* device_identifiers,
                                                                         bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_discard_x(value);
}

bool krbn_core_configuration_get_selected_profile_device_mouse_discard_y(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_discard_y();
}

void krbn_core_configuration_set_selected_profile_device_mouse_discard_y(const krbn_device_identifiers* device_identifiers,
                                                                         bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_discard_y(value);
}

bool krbn_core_configuration_get_selected_profile_device_mouse_discard_vertical_wheel(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_discard_vertical_wheel();
}

void krbn_core_configuration_set_selected_profile_device_mouse_discard_vertical_wheel(const krbn_device_identifiers* device_identifiers,
                                                                                      bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_discard_vertical_wheel(value);
}

bool krbn_core_configuration_get_selected_profile_device_mouse_discard_horizontal_wheel(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_discard_horizontal_wheel();
}

void krbn_core_configuration_set_selected_profile_device_mouse_discard_horizontal_wheel(const krbn_device_identifiers* device_identifiers,
                                                                                        bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_discard_horizontal_wheel(value);
}

//
// mouse_swap_XXX
//

bool krbn_core_configuration_get_selected_profile_device_mouse_swap_xy(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_swap_xy();
}

void krbn_core_configuration_set_selected_profile_device_mouse_swap_xy(const krbn_device_identifiers* device_identifiers,
                                                                       bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_swap_xy(value);
}

bool krbn_core_configuration_get_selected_profile_device_mouse_swap_wheels(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_mouse_swap_wheels();
}

void krbn_core_configuration_set_selected_profile_device_mouse_swap_wheels(const krbn_device_identifiers* device_identifiers,
                                                                           bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_mouse_swap_wheels(value);
}

bool krbn_core_configuration_get_selected_profile_device_game_pad_swap_sticks(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_swap_sticks();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_swap_sticks(const krbn_device_identifiers* device_identifiers,
                                                                              bool value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_swap_sticks(value);
}

size_t krbn_core_configuration_get_selected_profile_not_connected_configured_devices_count(const char* _Nonnull connected_devices_json) {
  try {
    auto c = get_current_core_configuration();
    auto connected_devices = nlohmann::json::parse(connected_devices_json).get<krbn::connected_devices>();
    return c->get_selected_profile().not_connected_configured_devices_count(connected_devices);
  } catch (const std::exception& e) {
    std::cerr << __func__ << ": " << e.what() << std::endl;
  }

  return 0;
}

void krbn_core_configuration_erase_selected_profile_not_connected_configured_devices(const char* _Nonnull connected_devices_json) {
  try {
    auto c = get_current_core_configuration();
    auto connected_devices = nlohmann::json::parse(connected_devices_json).get<krbn::connected_devices>();
    c->get_selected_profile().erase_not_connected_configured_devices(connected_devices);
  } catch (const std::exception& e) {
    std::cerr << __func__ << ": " << e.what() << std::endl;
  }
}

// game_pad_xy_stick_deadzone

double krbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_deadzone(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_xy_stick_deadzone();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_deadzone(const krbn_device_identifiers* device_identifiers,
                                                                                    double value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_xy_stick_deadzone(value);
}

double krbn_core_configuration_game_pad_xy_stick_deadzone_default_value() {
  return empty_device->find_default_value(
      empty_device->get_game_pad_xy_stick_deadzone());
}

// game_pad_xy_stick_delta_magnitude_detection_threshold

double krbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_xy_stick_delta_magnitude_detection_threshold();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(const krbn_device_identifiers* device_identifiers,
                                                                                                               double value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_xy_stick_delta_magnitude_detection_threshold(value);
}

double krbn_core_configuration_game_pad_xy_stick_delta_magnitude_detection_threshold_default_value() {
  return empty_device->find_default_value(
      empty_device->get_game_pad_xy_stick_delta_magnitude_detection_threshold());
}

// game_pad_xy_stick_continued_movement_absolute_magnitude_threshold

double krbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const krbn_device_identifiers* device_identifiers,
                                                                                                                           double value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(value);
}

double krbn_core_configuration_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_default_value() {
  return empty_device->find_default_value(
      empty_device->get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold());
}

// game_pad_xy_stick_continued_movement_interval_milliseconds

int krbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_xy_stick_continued_movement_interval_milliseconds();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const krbn_device_identifiers* device_identifiers,
                                                                                                                    int value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->set_game_pad_xy_stick_continued_movement_interval_milliseconds(value);
}

int krbn_core_configuration_game_pad_xy_stick_continued_movement_interval_milliseconds_default_value() {
  return empty_device->find_default_value(
      empty_device->get_game_pad_xy_stick_continued_movement_interval_milliseconds());
}

// game_pad_wheels_stick_deadzone

double krbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_deadzone(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_wheels_stick_deadzone();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_deadzone(const krbn_device_identifiers* device_identifiers,
                                                                                        double value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_wheels_stick_deadzone(value);
}

double krbn_core_configuration_game_pad_wheels_stick_deadzone_default_value() {
  return empty_device->find_default_value(
      empty_device->get_game_pad_wheels_stick_deadzone());
}

// game_pad_wheels_stick_delta_magnitude_detection_threshold

double krbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_wheels_stick_delta_magnitude_detection_threshold();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(const krbn_device_identifiers* device_identifiers,
                                                                                                                   double value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_wheels_stick_delta_magnitude_detection_threshold(value);
}

double krbn_core_configuration_game_pad_wheels_stick_delta_magnitude_detection_threshold_default_value() {
  return empty_device->find_default_value(
      empty_device->get_game_pad_wheels_stick_delta_magnitude_detection_threshold());
}

// game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold

double krbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const krbn_device_identifiers* device_identifiers,
                                                                                                                               double value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(value);
}

double krbn_core_configuration_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_default_value() {
  return empty_device->find_default_value(
      empty_device->get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold());
}

// game_pad_wheels_stick_continued_movement_interval_milliseconds

int krbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  return d->get_game_pad_wheels_stick_continued_movement_interval_milliseconds();
}

void krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const krbn_device_identifiers* device_identifiers,
                                                                                                                        int value) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_wheels_stick_continued_movement_interval_milliseconds(value);
}

int krbn_core_configuration_game_pad_wheels_stick_continued_movement_interval_milliseconds_default_value() {
  return empty_device->find_default_value(
      empty_device->get_game_pad_wheels_stick_continued_movement_interval_milliseconds());
}

// game_pad_stick_x_formula

bool krbn_core_configuration_get_selected_profile_device_game_pad_stick_x_formula(const krbn_device_identifiers* device_identifiers,
                                                                                  char* buffer,
                                                                                  size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  auto formula = d->get_game_pad_stick_x_formula();
  // Return false if no enough space.
  if (formula.length() < length) {
    strlcpy(buffer, formula.c_str(), length);
    return true;
  }

  return false;
}

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(const krbn_device_identifiers* device_identifiers,
                                                                                  const char* value) {
  if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
    return false;
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_stick_x_formula(value);

  return true;
}

void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_x_formula(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_stick_x_formula(
      d->find_default_value(
          d->get_game_pad_stick_x_formula()));
}

// game_pad_stick_y_formula

bool krbn_core_configuration_get_selected_profile_device_game_pad_stick_y_formula(const krbn_device_identifiers* device_identifiers,
                                                                                  char* buffer,
                                                                                  size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  auto formula = d->get_game_pad_stick_y_formula();
  // Return false if no enough space.
  if (formula.length() < length) {
    strlcpy(buffer, formula.c_str(), length);
    return true;
  }

  return false;
}

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(const krbn_device_identifiers* device_identifiers,
                                                                                  const char* value) {
  if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
    return false;
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_stick_y_formula(value);

  return true;
}

void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_y_formula(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_stick_y_formula(
      d->find_default_value(
          d->get_game_pad_stick_y_formula()));
}

// game_pad_stick_vertical_wheel_formula

bool krbn_core_configuration_get_selected_profile_device_game_pad_stick_vertical_wheel_formula(const krbn_device_identifiers* device_identifiers,
                                                                                               char* buffer,
                                                                                               size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  auto formula = d->get_game_pad_stick_vertical_wheel_formula();
  // Return false if no enough space.
  if (formula.length() < length) {
    strlcpy(buffer, formula.c_str(), length);
    return true;
  }

  return false;
}

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(const krbn_device_identifiers* device_identifiers,
                                                                                               const char* value) {
  if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
    return false;
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_stick_vertical_wheel_formula(value);

  return true;
}

void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_vertical_wheel_formula(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_stick_vertical_wheel_formula(
      d->find_default_value(
          d->get_game_pad_stick_vertical_wheel_formula()));
}

// game_pad_stick_horizontal_wheel_formula

bool krbn_core_configuration_get_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const krbn_device_identifiers* device_identifiers,
                                                                                                 char* buffer,
                                                                                                 size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  auto formula = d->get_game_pad_stick_horizontal_wheel_formula();
  // Return false if no enough space.
  if (formula.length() < length) {
    strlcpy(buffer, formula.c_str(), length);
    return true;
  }

  return false;
}

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const krbn_device_identifiers* device_identifiers,
                                                                                                 const char* value) {
  if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
    return false;
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_stick_horizontal_wheel_formula(value);

  return true;
}

void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const krbn_device_identifiers* device_identifiers) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers));
  d->set_game_pad_stick_horizontal_wheel_formula(
      d->find_default_value(
          d->get_game_pad_stick_horizontal_wheel_formula()));
}
