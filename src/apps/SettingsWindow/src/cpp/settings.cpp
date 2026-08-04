#include "settings.hpp"
#include "app_icon.hpp"
#include "application_launcher.hpp"
#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "filesystem_utility.hpp"
#include "json_utility.hpp"
#include "run_loop_thread_utility.hpp"
#include "services_utility.hpp"
#include "settings_components_manager.hpp"
#include "settings_cpp.hpp"
#include "types.hpp"
#include "update_utility.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <pqrs/cf/dictionary.hpp>
#include <pqrs/karabiner/driverkit/virtual_hid_device_service.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <string>

namespace {
std::shared_ptr<krbn::dispatcher_utility::scoped_dispatcher_manager> scoped_dispatcher_manager_;
std::shared_ptr<krbn::run_loop_thread_utility::scoped_run_loop_thread_manager> scoped_run_loop_thread_manager_;
} // namespace

std::shared_ptr<settings_components_manager> settings_components_manager_;

void krbn_initialize() {
  krbn::logger::get_logger()->debug(__func__);

  if (!scoped_dispatcher_manager_) {
    scoped_dispatcher_manager_ = krbn::dispatcher_utility::initialize_dispatchers();
  }

  if (!scoped_run_loop_thread_manager_) {
    scoped_run_loop_thread_manager_ = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
        pqrs::cf::run_loop_thread::failure_policy::exit);
  }

  if (!settings_components_manager_) {
    settings_components_manager_ = std::make_shared<settings_components_manager>();
  }
}

void krbn_terminate() {
  krbn::logger::get_logger()->debug(__func__);

  settings_components_manager_ = nullptr;

  scoped_run_loop_thread_manager_ = nullptr;

  scoped_dispatcher_manager_ = nullptr;
}

void krbn_load_custom_environment_variables() {
  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();
  krbn::environment_variable_utility::log(environment_variables);
}

void krbn_enqueue_callback(void (*callback)()) {
  if (auto manager = settings_components_manager_) {
    manager->enqueue_callback(callback);
  }
}

void krbn_get_user_configuration_directory(char* buffer,
                                           size_t length) {
  strlcpy(buffer, krbn::constants::get_user_configuration_directory().c_str(), length);
}

void krbn_get_user_complex_modifications_assets_directory(char* buffer,
                                                          size_t length) {
  strlcpy(buffer, krbn::constants::get_user_complex_modifications_assets_directory().c_str(), length);
}

void krbn_get_user_tmp_directory(char* buffer,
                                 size_t length) {
  strlcpy(buffer, krbn::constants::get_user_tmp_directory().c_str(), length);
}

void krbn_services_register_core_daemons() {
  krbn::services_utility::register_core_daemons();
}

void krbn_services_register_core_agents() {
  krbn::services_utility::register_core_agents();
}

void krbn_services_bootout_old_agents() {
  krbn::services_utility::bootout_old_agents();
}

void krbn_services_restart_console_user_server_agent() {
  krbn::services_utility::restart_console_user_server_agent();
}

void krbn_services_unregister_all_agents() {
  krbn::services_utility::unregister_all_agents();
}

bool krbn_services_daemons_enabled() {
  return krbn::services_utility::core_daemons_enabled() == true;
}

bool krbn_services_agents_enabled() {
  return krbn::services_utility::core_agents_enabled() == true;
}

void krbn_updater_check_for_updates_stable_only() {
  krbn::update_utility::check_for_updates_stable_only();
}

void krbn_updater_check_for_updates_with_beta_version() {
  krbn::update_utility::check_for_updates_with_beta_version();
}

void krbn_killall_settings() {
  krbn::application_launcher::killall_settings();
}

void krbn_launch_uninstaller() {
  krbn::application_launcher::launch_uninstaller();
}

bool krbn_system_core_configuration_file_path_exists() {
  return krbn::filesystem_utility::exists(krbn::constants::get_system_core_configuration_file_path());
}

bool krbn_system_preferences_virtual_hid_keyboard_modifier_mappings_exists() {
  if (auto matching_dictionary = pqrs::cf::make_cf_mutable_dictionary()) {
    CFDictionarySetValue(*matching_dictionary,
                         CFSTR(kIOProviderClassKey),
                         CFSTR("IOHIDEventService"));

    if (auto dict = pqrs::cf::make_cf_mutable_dictionary()) {
      CFDictionarySetValue(*dict,
                           CFSTR("SerialNumber"),
                           CFSTR("pqrs.org:Karabiner-DriverKit-VirtualHIDKeyboard"));

      CFDictionarySetValue(*matching_dictionary,
                           CFSTR(kIOPropertyMatchKey),
                           *dict);

      auto modifier_mappings = pqrs::osx::system_preferences::get_modifier_mappings(*matching_dictionary);
      return !modifier_mappings.empty();
    }
  }

  return false;
}

int krbn_get_app_icon_number() {
  auto icon = krbn::app_icon(krbn::constants::get_system_app_icon_configuration_file_path());
  return icon.get_number();
}

void krbn_save_prettierrc() {
  krbn::complex_modifications_utility::save_prettierrc();
}

//
// configuration_monitor
//

void krbn_enable_configuration_monitor() {
  if (auto manager = settings_components_manager_) {
    manager->enable_configuration_monitor();
  }
}

void krbn_register_core_configuration_updated_callback(krbn_core_configuration_updated_t callback) {
  if (auto manager = settings_components_manager_) {
    if (auto m = manager->get_settings_configuration_monitor()) {
      m->register_krbn_core_configuration_updated_callback(callback);
    }
  }
}

//
// complex_modifications_assets_manager
//

void krbn_enable_complex_modifications_assets_manager() {
  if (auto manager = settings_components_manager_) {
    manager->enable_complex_modifications_assets_manager();
  }
}

void krbn_complex_modifications_assets_manager_reload(krbn_json_output_callback output) {
  auto json = nlohmann::json::array();

  if (auto manager = settings_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      json = m->reload_and_get_files_json();
    }
  }

  auto json_string = krbn::json_utility::dump(json);
  output(json_string.data(), json_string.size());
}

void krbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                                               size_t index) {
  if (auto manager = settings_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      if (auto c = manager->get_current_core_configuration()) {
        m->add_rule_to_core_configuration_selected_profile(file_index,
                                                           index,
                                                           *c);
      }
    }
  }
}

void krbn_complex_modifications_assets_manager_erase_file(size_t index) {
  if (auto manager = settings_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      return m->erase_file(index);
    }
  }
}

//
// file_monitor
//

void krbn_enable_file_monitors() {
  if (auto manager = settings_components_manager_) {
    manager->enable_file_monitors();
  }
}

void krbn_register_file_updated_callback(const char* file_path,
                                         krbn_file_updated_t callback) {
  if (auto manager = settings_components_manager_) {
    if (auto m = manager->get_settings_file_monitors()) {
      m->register_krbn_file_updated_callback(file_path, callback);
    }
  }
}

void krbn_unregister_file_updated_callback(const char* file_path,
                                           krbn_file_updated_t callback) {
  if (auto manager = settings_components_manager_) {
    if (auto m = manager->get_settings_file_monitors()) {
      m->unregister_krbn_file_updated_callback(file_path, callback);
    }
  }
}

//
// log_monitor
//

void krbn_enable_log_monitor() {
  if (auto manager = settings_components_manager_) {
    manager->enable_log_monitor();
  }
}

void krbn_disable_log_monitor() {
  if (auto manager = settings_components_manager_) {
    manager->disable_log_monitor();
  }
}

void krbn_register_log_messages_updated_callback(krbn_log_messages_updated_t callback) {
  if (auto manager = settings_components_manager_) {
    if (auto m = manager->get_settings_log_monitor()) {
      m->register_krbn_log_messages_updated_callback(callback);
    }
  }
}

//
// core_service_client
//

void krbn_enable_core_service_daemon_client() {
  if (auto manager = settings_components_manager_) {
    manager->enable_core_service_daemon_client();
  }
}

void krbn_core_service_daemon_client_async_start() {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_core_service_daemon_client()) {
      c->async_start();
    }
  }
}

void krbn_core_service_daemon_client_async_get_connected_devices() {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_core_service_daemon_client()) {
      c->async_get_connected_devices();
    }
  }
}

void krbn_register_core_service_daemon_client_connected_devices_received_callback(krbn_core_service_daemon_client_connected_devices_received_t _Nonnull callback) {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_core_service_daemon_client()) {
      c->register_connected_devices_received_callback(callback);
    }
  }
}

void krbn_core_service_daemon_client_async_get_system_variables() {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_core_service_daemon_client()) {
      c->async_get_system_variables();
    }
  }
}

void krbn_register_core_service_daemon_client_system_variables_received_callback(krbn_core_service_daemon_client_system_variables_received_t _Nonnull callback) {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_core_service_daemon_client()) {
      c->register_system_variables_received_callback(callback);
    }
  }
}

void krbn_core_service_daemon_client_async_set_app_icon(int number) {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_core_service_daemon_client()) {
      c->async_set_app_icon(number);
    }
  }
}

//
// console_user_server_client
//

void krbn_enable_console_user_server_client(uid_t uid) {
  if (auto manager = settings_components_manager_) {
    manager->enable_console_user_server_client(uid);
  }
}

void krbn_console_user_server_client_async_start() {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_console_user_server_client()) {
      c->async_start();
    }
  }
}

void krbn_register_console_user_server_client_status_changed_callback(krbn_console_user_server_client_status_changed_t callback) {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_console_user_server_client()) {
      c->register_status_changed_callback(callback);
    }
  }
}

krbn_console_user_server_client_status krbn_console_user_server_client_get_status() {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_console_user_server_client()) {
      return c->get_status();
    }
  }

  return krbn_console_user_server_client_status_none;
}

void krbn_console_user_server_client_async_get_settings_window_guidance() {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_console_user_server_client()) {
      c->async_get_settings_window_guidance();
    }
  }
}

void krbn_register_console_user_server_client_settings_window_guidance_received_callback(krbn_console_user_server_client_settings_window_guidance_received_t callback) {
  if (auto manager = settings_components_manager_) {
    if (auto c = manager->get_settings_console_user_server_client()) {
      c->register_settings_window_guidance_received_callback(callback);
    }
  }
}
