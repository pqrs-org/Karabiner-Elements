#include "complex_modifications_utility.hpp"
#include "duktape_utility.hpp"
#include "json_utility.hpp"
#include "settings.hpp"
#include "settings_components_manager.hpp"
#include "settings_cpp.hpp"

void krbn_async_configuration_command(const char* json_string,
                                      krbn_json_output_callback_with_context output,
                                      void* context) {
  auto completed = [output, context](const nlohmann::json& result) {
    auto json = krbn::json_utility::dump(result);
    output(json.data(), json.size(), context);
  };
  try {
    auto command = nlohmann::json::parse(json_string);
    if (auto manager = settings_cpp::get_components_manager()) {
      manager->get_configuration_store()->async_execute(std::move(command), completed);
      return;
    }
    completed({{"error", "Settings components are stopped"}});
  } catch (const std::exception& e) {
    completed({{"error", e.what()}});
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
