#include "libkrbn/libkrbn.h"
#include "application_launcher.hpp"
#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "dispatcher_utility.hpp"
#include "libkrbn/impl/libkrbn_components_manager.hpp"
#include "libkrbn/impl/libkrbn_cpp.hpp"
#include "process_utility.hpp"
#include "run_loop_thread_utility.hpp"
#include "services_utility.hpp"
#include "types.hpp"
#include "update_utility.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <pqrs/karabiner/driverkit/virtual_hid_device_service.hpp>
#include <string>

namespace {
std::shared_ptr<krbn::dispatcher_utility::scoped_dispatcher_manager> scoped_dispatcher_manager_;
std::shared_ptr<krbn::run_loop_thread_utility::scoped_run_loop_thread_manager> scoped_run_loop_thread_manager_;
} // namespace

std::shared_ptr<libkrbn_components_manager> libkrbn_components_manager_;

void libkrbn_set_logging_level_off(void) {
  krbn::logger::get_logger()->set_level(spdlog::level::off);
}

void libkrbn_set_logging_level_info(void) {
  krbn::logger::get_logger()->set_level(spdlog::level::info);
}

void libkrbn_initialize(void) {
  krbn::logger::get_logger()->info(__func__);

  if (!scoped_dispatcher_manager_) {
    scoped_dispatcher_manager_ = krbn::dispatcher_utility::initialize_dispatchers();
  }

  if (!scoped_run_loop_thread_manager_) {
    scoped_run_loop_thread_manager_ = krbn::run_loop_thread_utility::initialize_shared_run_loop_thread();
  }

  if (!libkrbn_components_manager_) {
    libkrbn_components_manager_ = std::make_shared<libkrbn_components_manager>();
  }
}

void libkrbn_terminate(void) {
  krbn::logger::get_logger()->info(__func__);

  libkrbn_components_manager_ = nullptr;

  scoped_run_loop_thread_manager_ = nullptr;

  scoped_dispatcher_manager_ = nullptr;
}

void libkrbn_enqueue_callback(void (*callback)(void)) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enqueue_callback(callback);
  }
}

void libkrbn_get_user_configuration_directory(char* buffer,
                                              size_t length) {
  strlcpy(buffer, krbn::constants::get_user_configuration_directory().c_str(), length);
}

void libkrbn_get_user_complex_modifications_assets_directory(char* buffer,
                                                             size_t length) {
  strlcpy(buffer, krbn::constants::get_user_complex_modifications_assets_directory().c_str(), length);
}

void libkrbn_get_system_app_icon_configuration_file_path(char* buffer,
                                                         size_t length) {
  strlcpy(buffer, krbn::constants::get_system_app_icon_configuration_file_path().c_str(), length);
}

bool libkrbn_user_pid_directory_writable(void) {
  auto pid_directory = krbn::constants::get_user_pid_directory();

  pqrs::filesystem::create_directory_with_intermediate_directories(pid_directory, 0755);
  if (!pqrs::filesystem::is_directory(pid_directory)) {
    return false;
  }

  auto doctor_file = pid_directory / "doctor";

  if (pqrs::filesystem::exists(doctor_file)) {
    std::error_code error_code;
    std::filesystem::remove(pid_directory / "doctor", error_code);
  }

  if (pqrs::filesystem::exists(doctor_file)) {
    return false;
  }

  std::ofstream ofs(doctor_file);
  if (!ofs) {
    return false;
  }

  ofs << "writable";
  ofs.close();

  if (!pqrs::filesystem::exists(doctor_file)) {
    return false;
  }

  return true;
}

bool libkrbn_lock_single_application_with_user_pid_file(const char* _Nonnull pid_file_name) {
  return krbn::process_utility::lock_single_application_with_user_pid_file(pid_file_name);
}

void libkrbn_unlock_single_application(void) {
  krbn::process_utility::unlock_single_application();
}

void libkrbn_services_register_core_daemons(void) {
  krbn::services_utility::register_core_daemons();
}

void libkrbn_services_register_core_agents(void) {
  krbn::services_utility::register_core_agents();
}

void libkrbn_services_bootout_old_agents(void) {
  krbn::services_utility::bootout_old_agents();
}

void libkrbn_services_restart_console_user_server_agent(void) {
  krbn::services_utility::restart_console_user_server_agent();
}

void libkrbn_services_unregister_all_agents(void) {
  krbn::services_utility::unregister_all_agents();
}

bool libkrbn_services_core_daemons_enabled(void) {
  return krbn::services_utility::core_daemons_enabled();
}

bool libkrbn_services_core_agents_enabled(void) {
  return krbn::services_utility::core_agents_enabled();
}

bool libkrbn_services_daemon_running(const char* service_name) {
  return krbn::services_utility::daemon_running(service_name);
}

bool libkrbn_services_agent_running(const char* service_name) {
  return krbn::services_utility::agent_running(service_name);
}

bool libkrbn_services_core_daemons_running(void) {
  return krbn::services_utility::core_daemons_running();
}

bool libkrbn_services_core_agents_running(void) {
  return krbn::services_utility::core_agents_running();
}

void libkrbn_updater_check_for_updates_stable_only(void) {
  krbn::update_utility::check_for_updates_stable_only();
}

void libkrbn_updater_check_for_updates_with_beta_version(void) {
  krbn::update_utility::check_for_updates_with_beta_version();
}

void libkrbn_launch_event_viewer(void) {
  krbn::application_launcher::launch_event_viewer();
}

void libkrbn_launch_settings(void) {
  krbn::application_launcher::launch_settings();
}

void libkrbn_killall_settings(void) {
  krbn::application_launcher::killall_settings();
}

void libkrbn_launch_uninstaller(void) {
  krbn::application_launcher::launch_uninstaller();
}

bool libkrbn_driver_running(void) {
  return pqrs::karabiner::driverkit::virtual_hid_device_service::utility::driver_running();
}

bool libkrbn_virtual_hid_keyboard_exists(void) {
  return pqrs::karabiner::driverkit::virtual_hid_device_service::utility::virtual_hid_keyboard_exists();
}

bool libkrbn_virtual_hid_pointing_exists(void) {
  return pqrs::karabiner::driverkit::virtual_hid_device_service::utility::virtual_hid_pointing_exists();
}

bool libkrbn_system_core_configuration_file_path_exists(void) {
  return pqrs::filesystem::exists(krbn::constants::get_system_core_configuration_file_path());
}

bool libkrbn_is_momentary_switch_event_target(int32_t usage_page, int32_t usage) {
  return krbn::momentary_switch_event::target(
      pqrs::hid::usage_page::value_t(usage_page),
      pqrs::hid::usage::value_t(usage));
}

bool libkrbn_is_modifier_flag(int32_t usage_page, int32_t usage) {
  return krbn::momentary_switch_event(
             pqrs::hid::usage_page::value_t(usage_page),
             pqrs::hid::usage::value_t(usage))
      .modifier_flag();
}

void libkrbn_get_momentary_switch_event_json_string(char* buffer, size_t length, int32_t usage_page, int32_t usage) {
  auto json = nlohmann::json(krbn::momentary_switch_event(pqrs::hid::usage_page::value_t(usage_page),
                                                          pqrs::hid::usage::value_t(usage)));
  if (json.is_null()) {
    json = nlohmann::json::object({
        {"usage_page", usage_page},
        {"usage", usage},
    });
  }

  strlcpy(buffer, json.dump().c_str(), length);
}

void libkrbn_get_momentary_switch_event_usage_name(char* buffer, size_t length, int32_t usage_page, int32_t usage) {
  strlcpy(buffer, "", length);

  auto json = nlohmann::json(krbn::momentary_switch_event(pqrs::hid::usage_page::value_t(usage_page),
                                                          pqrs::hid::usage::value_t(usage)));
  if (json.is_null()) {
    json = nlohmann::json::object({
        {"usage", usage},
    });
  }

  for (const auto& [key, value] : json.items()) {
    strlcpy(buffer, value.dump().c_str(), length);
  }
}

void libkrbn_get_modifier_flag_name(char* buffer, size_t length, int32_t usage_page, int32_t usage) {
  if (auto modifier_flag = krbn::momentary_switch_event(
                               pqrs::hid::usage_page::value_t(usage_page),
                               pqrs::hid::usage::value_t(usage))
                               .make_modifier_flag()) {
    if (auto name = get_modifier_flag_name(*modifier_flag)) {
      strlcpy(buffer, name->data(), length);
      return;
    }
  }

  strlcpy(buffer, "", length);
}

void libkrbn_get_simple_modification_json_string(char* buffer, size_t length, int32_t usage_page, int32_t usage) {
  //
  // Skip pqrs::hid::usage::button::button_1 to avoid disabling left click
  //

  if (pqrs::hid::usage_page::value_t(usage_page) == pqrs::hid::usage_page::button &&
      pqrs::hid::usage::value_t(usage) == pqrs::hid::usage::button::button_1) {
    strlcpy(buffer, "", length);
    return;
  }

  //
  // Create json
  //

  auto json = nlohmann::json(krbn::momentary_switch_event(pqrs::hid::usage_page::value_t(usage_page),
                                                          pqrs::hid::usage::value_t(usage)));
  if (json.is_null()) {
    strlcpy(buffer, "", length);

  } else {
    json = nlohmann::json::object({
        {"from", json},
    });
    strlcpy(buffer, json.dump().c_str(), length);
  }
}

//
// version_monitor
//

void libkrbn_enable_version_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_version_monitor();
  }
}

void libkrbn_disable_version_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_version_monitor();
  }
}

void libkrbn_register_version_updated_callback(libkrbn_version_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_version_monitor()) {
      m->register_libkrbn_version_updated_callback(callback);
    }
  }
}

void libkrbn_unregister_version_updated_callback(libkrbn_version_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_version_monitor()) {
      m->unregister_libkrbn_version_updated_callback(callback);
    }
  }
}

bool libkrbn_get_version(char* buffer,
                         size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_version_monitor()) {
      strlcpy(buffer, m->get_version().c_str(), length);
      return true;
    }
  }

  return false;
}

//
// configuration_monitor
//

void libkrbn_enable_configuration_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_configuration_monitor();
  }
}

void libkrbn_disable_configuration_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_configuration_monitor();
  }
}

void libkrbn_register_core_configuration_updated_callback(libkrbn_core_configuration_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_configuration_monitor()) {
      m->register_libkrbn_core_configuration_updated_callback(callback);
    }
  }
}

void libkrbn_unregister_core_configuration_updated_callback(libkrbn_core_configuration_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_configuration_monitor()) {
      m->unregister_libkrbn_core_configuration_updated_callback(callback);
    }
  }
}

//
// complex_modifications_assets_manager
//

void libkrbn_enable_complex_modifications_assets_manager(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_complex_modifications_assets_manager();
  }
}

void libkrbn_disable_complex_modifications_assets_manager(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_complex_modifications_assets_manager();
  }
}

void libkrbn_complex_modifications_assets_manager_reload(void) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      m->reload();
    }
  }
}

size_t libkrbn_complex_modifications_assets_manager_get_files_size(void) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      return m->get_files_size();
    }
  }
  return 0;
}

bool libkrbn_complex_modifications_assets_manager_get_file_title(size_t index,
                                                                 char* buffer,
                                                                 size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      strlcpy(buffer, m->get_file_title(index), length);
      return true;
    }
  }

  return false;
}

time_t libkrbn_complex_modifications_assets_manager_get_file_last_write_time(size_t index) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      return m->get_file_last_write_time(index);
    }
  }
  return 0;
}

size_t libkrbn_complex_modifications_assets_manager_get_rules_size(size_t file_index) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      return m->get_rules_size(file_index);
    }
  }
  return 0;
}

bool libkrbn_complex_modifications_assets_manager_get_rule_description(size_t file_index,
                                                                       size_t index,
                                                                       char* buffer,
                                                                       size_t length) {
  if (buffer && length > 0) {
    buffer[0] = '\0';
  }

  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      strlcpy(buffer, m->get_rule_description(file_index, index), length);
      return true;
    }
  }

  return false;
}

void libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                                                  size_t index) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      if (auto c = manager->get_current_core_configuration()) {
        m->add_rule_to_core_configuration_selected_profile(file_index,
                                                           index,
                                                           *c);
      }
    }
  }
}

bool libkrbn_complex_modifications_assets_manager_user_file(size_t index) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      return m->user_file(index);
    }
  }
  return false;
}

void libkrbn_complex_modifications_assets_manager_erase_file(size_t index) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_complex_modifications_assets_manager()) {
      return m->erase_file(index);
    }
  }
}

//
// system_preferences_monitor
//

void libkrbn_enable_system_preferences_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_system_preferences_monitor();
  }
}

void libkrbn_disable_system_preferences_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_system_preferences_monitor();
  }
}

void libkrbn_register_system_preferences_updated_callback(libkrbn_system_preferences_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_system_preferences_monitor()) {
      m->register_libkrbn_system_preferences_updated_callback(callback);
    }
  }
}

void libkrbn_unregister_system_preferences_updated_callback(libkrbn_system_preferences_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_system_preferences_monitor()) {
      m->unregister_libkrbn_system_preferences_updated_callback(callback);
    }
  }
}

bool libkrbn_system_preferences_properties_get_use_fkeys_as_standard_function_keys(void) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto p = manager->get_current_system_preferences_properties()) {
      return p->get_use_fkeys_as_standard_function_keys();
    }
  }

  return false;
}

//
// connected_devices_monitor
//

void libkrbn_enable_connected_devices_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_connected_devices_monitor();
  }
}

void libkrbn_disable_connected_devices_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_connected_devices_monitor();
  }
}

void libkrbn_register_connected_devices_updated_callback(libkrbn_connected_devices_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_connected_devices_monitor()) {
      m->register_libkrbn_connected_devices_updated_callback(callback);
    }
  }
}

void libkrbn_unregister_connected_devices_updated_callback(libkrbn_connected_devices_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_connected_devices_monitor()) {
      m->unregister_libkrbn_connected_devices_updated_callback(callback);
    }
  }
}

//
// file_monitor
//

void libkrbn_enable_file_monitors(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_file_monitors();
  }
}

void libkrbn_disable_file_monitors(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_file_monitors();
  }
}

void libkrbn_register_file_updated_callback(const char* file_path, libkrbn_file_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_file_monitors()) {
      m->register_libkrbn_file_updated_callback(file_path, callback);
    }
  }
}

void libkrbn_unregister_file_updated_callback(const char* file_path, libkrbn_file_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_file_monitors()) {
      m->unregister_libkrbn_file_updated_callback(file_path, callback);
    }
  }
}

void libkrbn_get_devices_json_file_path(char* buffer, size_t length) {
  strlcpy(buffer, krbn::constants::get_devices_json_file_path().c_str(), length);
}

void libkrbn_get_grabber_state_json_file_path(char* buffer, size_t length) {
  strlcpy(buffer, krbn::constants::get_grabber_state_json_file_path().c_str(), length);
}

void libkrbn_get_manipulator_environment_json_file_path(char* buffer, size_t length) {
  strlcpy(buffer, krbn::constants::get_manipulator_environment_json_file_path().c_str(), length);
}

void libkrbn_get_notification_message_json_file_path(char* buffer, size_t length) {
  strlcpy(buffer, krbn::constants::get_notification_message_file_path().c_str(), length);
}

//
// frontmost_application_monitor
//

void libkrbn_enable_frontmost_application_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_frontmost_application_monitor();
  }
}

void libkrbn_disable_frontmost_application_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_frontmost_application_monitor();
  }
}

void libkrbn_register_frontmost_application_changed_callback(libkrbn_frontmost_application_changed callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_frontmost_application_monitor()) {
      m->register_libkrbn_frontmost_application_changed_callback(callback);
    }
  }
}

void libkrbn_unregister_frontmost_application_changed_callback(libkrbn_frontmost_application_changed callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_frontmost_application_monitor()) {
      m->unregister_libkrbn_frontmost_application_changed_callback(callback);
    }
  }
}

bool libkrbn_get_frontmost_application(char* bundle_identifier_buffer,
                                       size_t bundle_identifier_buffer_length,
                                       char* file_path_buffer,
                                       size_t file_path_buffer_length) {
  if (bundle_identifier_buffer && bundle_identifier_buffer_length > 0) {
    bundle_identifier_buffer[0] = '\0';
  }
  if (file_path_buffer && file_path_buffer_length > 0) {
    file_path_buffer[0] = '\0';
  }

  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_frontmost_application_monitor()) {
      if (auto a = m->get_application()) {
        strlcpy(bundle_identifier_buffer, a->get_bundle_identifier().value_or("").c_str(), bundle_identifier_buffer_length);
        strlcpy(file_path_buffer, a->get_file_path().value_or("").c_str(), file_path_buffer_length);
        return true;
      }
    }
  }

  return false;
}

//
// log_monitor
//

void libkrbn_enable_log_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_log_monitor();
  }
}

void libkrbn_disable_log_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_log_monitor();
  }
}

void libkrbn_register_log_messages_updated_callback(libkrbn_log_messages_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_log_monitor()) {
      m->register_libkrbn_log_messages_updated_callback(callback);
    }
  }
}

void libkrbn_unregister_log_messages_updated_callback(libkrbn_log_messages_updated callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_log_monitor()) {
      m->unregister_libkrbn_log_messages_updated_callback(callback);
    }
  }
}

//
// hid_value_monitor
//

void libkrbn_enable_hid_value_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->enable_hid_value_monitor();
  }
}

void libkrbn_disable_hid_value_monitor(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_hid_value_monitor();
  }
}

void libkrbn_register_hid_value_arrived_callback(libkrbn_hid_value_arrived callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_hid_value_monitor()) {
      m->register_libkrbn_hid_value_arrived_callback(callback);
    }
  }
}

void libkrbn_unregister_hid_value_arrived_callback(libkrbn_hid_value_arrived callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_hid_value_monitor()) {
      m->unregister_libkrbn_hid_value_arrived_callback(callback);
    }
  }
}

bool libkrbn_hid_value_monitor_observed(void) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto m = manager->get_libkrbn_hid_value_monitor()) {
      return m->get_observed();
    }
  }
  return false;
}

//
// grabber_client
//

void libkrbn_enable_grabber_client(const char* client_socket_directory_name) {
  if (auto manager = libkrbn_components_manager_) {
    std::optional<std::string> name;
    if (client_socket_directory_name) {
      name = client_socket_directory_name;
    }

    manager->enable_grabber_client(name);
  }
}

void libkrbn_disable_grabber_client(void) {
  if (auto manager = libkrbn_components_manager_) {
    manager->disable_grabber_client();
  }
}

void libkrbn_grabber_client_async_start(void) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto c = manager->get_libkrbn_grabber_client()) {
      c->async_start();
    }
  }
}

void libkrbn_register_grabber_client_status_changed_callback(libkrbn_grabber_client_status_changed callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto c = manager->get_libkrbn_grabber_client()) {
      c->register_libkrbn_grabber_client_status_changed_callback(callback);
    }
  }
}

void libkrbn_unregister_grabber_client_status_changed_callback(libkrbn_grabber_client_status_changed callback) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto c = manager->get_libkrbn_grabber_client()) {
      c->unregister_libkrbn_grabber_client_status_changed_callback(callback);
    }
  }
}

libkrbn_grabber_client_status libkrbn_grabber_client_get_status(void) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto c = manager->get_libkrbn_grabber_client()) {
      return c->get_status();
    }
  }

  return libkrbn_grabber_client_status_none;
}

void libkrbn_grabber_client_async_connect_multitouch_extension(void) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto c = manager->get_libkrbn_grabber_client()) {
      c->async_connect_multitouch_extension();
    }
  }
}

void libkrbn_grabber_client_async_set_app_icon(int number) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto c = manager->get_libkrbn_grabber_client()) {
      c->async_set_app_icon(number);
    }
  }
}

void libkrbn_grabber_client_async_set_variable(const char* name, int value) {
  if (auto manager = libkrbn_components_manager_) {
    if (auto c = manager->get_libkrbn_grabber_client()) {
      if (name) {
        c->async_set_variable(name, value);
      }
    }
  }
}
