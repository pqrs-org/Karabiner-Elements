#include "complex_modifications_utility.hpp"
#include "connected_devices.hpp"
#include "duktape_utility.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "settings_components_manager.hpp"
#include "settings_configuration_monitor.hpp"
#include "settings_configuration_snapshot.hpp"
#include "settings_configuration_updater.hpp"
#include "settings_cpp.hpp"

namespace {
auto empty_core_configuration = gsl::make_not_null(std::make_shared<krbn::core_configuration::core_configuration>());

[[nodiscard]] pqrs::not_null_shared_ptr_t<krbn::core_configuration::core_configuration> get_current_core_configuration() {
  if (auto manager = settings_cpp::get_components_manager()) {
    if (auto c = manager->get_current_core_configuration()) {
      return c;
    }
  }

  return empty_core_configuration;
}

[[nodiscard]] pqrs::not_null_shared_ptr_t<krbn::core_configuration::details::simple_modifications> find_simple_modifications(const char* device_identifiers_json) {
  auto di = settings_cpp::make_device_identifiers(device_identifiers_json);
  auto c = get_current_core_configuration();
  if (di.empty()) {
    return c->get_selected_profile().get_simple_modifications();
  } else {
    auto d = c->get_selected_profile().get_device(di);
    return d->get_simple_modifications();
  }
}

[[nodiscard]] pqrs::not_null_shared_ptr_t<krbn::core_configuration::details::simple_modifications> find_fn_function_keys(const char* device_identifiers_json) {
  auto di = settings_cpp::make_device_identifiers(device_identifiers_json);
  auto c = get_current_core_configuration();
  if (di.empty()) {
    return c->get_selected_profile().get_fn_function_keys();
  } else {
    auto d = c->get_selected_profile().get_device(di);
    return d->get_fn_function_keys();
  }
}

void output_json(const nlohmann::json& json,
                 krbn_json_output_callback_with_context output,
                 void* context) {
  auto string = krbn::json_utility::dump(json);
  output(string.data(), string.size(), context);
}
} // namespace

bool krbn_core_configuration_save(char* error_message_buffer,
                                  size_t error_message_buffer_length) {
  if (error_message_buffer && error_message_buffer_length > 0) {
    error_message_buffer[0] = '\0';
  }

  if (auto manager = settings_cpp::get_components_manager()) {
    if (auto c = manager->get_current_core_configuration()) {
      static_cast<void>(manager->take_core_configuration_save_pending());

      try {
        c->sync_save_to_file();
        return true;
      } catch (const std::exception& e) {
        manager->mark_core_configuration_save_pending();
        strlcpy(error_message_buffer, e.what(), error_message_buffer_length);
        return false;
      }
    }
  }

  strlcpy(error_message_buffer, "core_configuration is not ready", error_message_buffer_length);
  return false;
}

void krbn_core_configuration_mark_save_pending(void) {
  if (auto manager = settings_cpp::get_components_manager()) {
    manager->mark_core_configuration_save_pending();
  }
}

void krbn_core_configuration_get_settings_configuration_snapshot_json(krbn_json_output_callback_with_context output,
                                                                      void* context) {
  auto c = get_current_core_configuration();
  output_json(settings_configuration_snapshot(*c)
                  .to_json(),
              output,
              context);
}

bool krbn_core_configuration_apply_settings_configuration_update(const char* json_string) {
  if (json_string) {
    try {
      auto c = get_current_core_configuration();
      return settings_configuration_updater::apply(
          nlohmann::json::parse(json_string),
          *c);
    } catch (const std::exception& e) {
      krbn::logger::get_logger()->error("Failed to apply settings configuration: {0}", e.what());
    }
  }

  return false;
}

void krbn_core_configuration_set_profile_name(size_t index, const char* value) {
  if (value) {
    auto c = get_current_core_configuration();
    c->set_profile_name(index, value);
  }
}

void krbn_core_configuration_select_profile(size_t index) {
  auto c = get_current_core_configuration();
  c->select_profile(index);
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

void krbn_core_configuration_replace_selected_profile_simple_modification(size_t index,
                                                                          const char* from_json_string,
                                                                          const char* to_json_string,
                                                                          const char* device_identifiers_json) {
  auto m = find_simple_modifications(device_identifiers_json);
  if (from_json_string &&
      to_json_string) {
    m->replace_pair(index, from_json_string, to_json_string);
  }
}

void krbn_core_configuration_push_back_selected_profile_simple_modification(const char* device_identifiers_json) {
  auto m = find_simple_modifications(device_identifiers_json);
  m->push_back_pair();
}

void krbn_core_configuration_erase_selected_profile_simple_modification(size_t index,
                                                                        const char* device_identifiers_json) {
  auto m = find_simple_modifications(device_identifiers_json);
  m->erase_pair(index);
}

void krbn_core_configuration_replace_selected_profile_fn_function_key(const char* from_json_string,
                                                                      const char* to_json_string,
                                                                      const char* device_identifiers_json) {
  auto k = find_fn_function_keys(device_identifiers_json);
  if (from_json_string &&
      to_json_string) {
    k->replace_second(from_json_string, to_json_string);
  }
}

void krbn_core_configuration_erase_selected_profile_complex_modifications_rule(size_t index) {
  auto c = get_current_core_configuration();
  c->get_selected_profile().get_complex_modifications()->erase_rule(index);
}

void krbn_core_configuration_move_selected_profile_complex_modifications_rule(size_t source_index, size_t destination_index) {
  auto c = get_current_core_configuration();
  c->get_selected_profile().get_complex_modifications()->move_rule(source_index, destination_index);
}

void krbn_core_configuration_set_selected_profile_complex_modifications_rule_enabled(size_t index, bool value) {
  auto c = get_current_core_configuration();
  const auto& rules = c->get_selected_profile().get_complex_modifications()->get_rules();
  if (index < rules.size()) {
    rules[index]->set_enabled(value);
  }
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

// game_pad_stick_x_formula

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(const char* device_identifiers_json,
                                                                                  const char* value) {
  if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
    return false;
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers_json));
  d->set_game_pad_stick_x_formula(value);

  return true;
}

void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_x_formula(const char* device_identifiers_json) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers_json));
  d->set_game_pad_stick_x_formula(
      d->find_default_value(
          d->get_game_pad_stick_x_formula()));
}

// game_pad_stick_y_formula

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(const char* device_identifiers_json,
                                                                                  const char* value) {
  if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
    return false;
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers_json));
  d->set_game_pad_stick_y_formula(value);

  return true;
}

void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_y_formula(const char* device_identifiers_json) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers_json));
  d->set_game_pad_stick_y_formula(
      d->find_default_value(
          d->get_game_pad_stick_y_formula()));
}

// game_pad_stick_vertical_wheel_formula

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(const char* device_identifiers_json,
                                                                                               const char* value) {
  if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
    return false;
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers_json));
  d->set_game_pad_stick_vertical_wheel_formula(value);

  return true;
}

void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_vertical_wheel_formula(const char* device_identifiers_json) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers_json));
  d->set_game_pad_stick_vertical_wheel_formula(
      d->find_default_value(
          d->get_game_pad_stick_vertical_wheel_formula()));
}

// game_pad_stick_horizontal_wheel_formula

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const char* device_identifiers_json,
                                                                                                 const char* value) {
  if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
    return false;
  }

  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers_json));
  d->set_game_pad_stick_horizontal_wheel_formula(value);

  return true;
}

void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const char* device_identifiers_json) {
  auto c = get_current_core_configuration();
  auto d = c->get_selected_profile().get_device(settings_cpp::make_device_identifiers(device_identifiers_json));
  d->set_game_pad_stick_horizontal_wheel_formula(
      d->find_default_value(
          d->get_game_pad_stick_horizontal_wheel_formula()));
}
