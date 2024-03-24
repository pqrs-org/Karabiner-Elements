#include "libkrbn/libkrbn.h"
#include "application_launcher.hpp"
#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "dispatcher_utility.hpp"
#include "launchctl_utility.hpp"
#include "libkrbn/impl/libkrbn_components_manager.hpp"
#include "libkrbn/impl/libkrbn_cpp.hpp"
#include "process_utility.hpp"
#include "run_loop_thread_utility.hpp"
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

const char* libkrbn_get_distributed_notification_observed_object(void) {
  return krbn::constants::get_distributed_notification_observed_object();
}

const char* libkrbn_get_distributed_notification_console_user_server_is_disabled(void) {
  return krbn::constants::get_distributed_notification_console_user_server_is_disabled();
}

const char* libkrbn_get_user_configuration_directory(void) {
  return krbn::constants::get_user_configuration_directory().c_str();
}

const char* libkrbn_get_user_complex_modifications_assets_directory(void) {
  return krbn::constants::get_user_complex_modifications_assets_directory().c_str();
}

const char* libkrbn_get_system_app_icon_configuration_file_path(void) {
  return krbn::constants::get_system_app_icon_configuration_file_path().c_str();
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

void libkrbn_launchctl_manage_console_user_server(bool load) {
  krbn::launchctl_utility::manage_console_user_server(load);
}

void libkrbn_launchctl_manage_notification_window(bool load) {
  krbn::launchctl_utility::manage_notification_window(load);
}

void libkrbn_launchctl_manage_session_monitor(void) {
  krbn::launchctl_utility::manage_session_monitor();
}

void libkrbn_launchctl_restart_console_user_server(void) {
  krbn::launchctl_utility::restart_console_user_server();
}

void libkrbn_launchctl_bootout_console_user_server(void) {
  krbn::launchctl_utility::bootout_console_user_server();
}

void libkrbn_launch_event_viewer(void) {
  krbn::application_launcher::launch_event_viewer();
}

void libkrbn_launch_menu(void) {
  krbn::application_launcher::launch_menu();
}

void libkrbn_launch_settings(void) {
  krbn::application_launcher::launch_settings();
}

void libkrbn_launch_multitouch_extension(void) {
  krbn::application_launcher::launch_multitouch_extension(false);
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

void libkrbn_enable_version_monitor(libkrbn_version_monitor_callback callback,
                                    void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_version_monitor(callback,
                                                        refcon);
  }
}

void libkrbn_disable_version_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_version_monitor();
  }
}

//
// configuration_monitor
//

void libkrbn_enable_configuration_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_configuration_monitor();
  }
}

void libkrbn_disable_configuration_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_configuration_monitor();
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
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_complex_modifications_assets_manager();
  }
}

void libkrbn_disable_complex_modifications_assets_manager(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_complex_modifications_assets_manager();
  }
}

void libkrbn_complex_modifications_assets_manager_reload(void) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      m->reload();
    }
  }
}

size_t libkrbn_complex_modifications_assets_manager_get_files_size(void) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      return m->get_files_size();
    }
  }
  return 0;
}

const char* libkrbn_complex_modifications_assets_manager_get_file_title(size_t index) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      return m->get_file_title(index);
    }
  }
  return nullptr;
}

time_t libkrbn_complex_modifications_assets_manager_get_file_last_write_time(size_t index) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      return m->get_file_last_write_time(index);
    }
  }
  return 0;
}

size_t libkrbn_complex_modifications_assets_manager_get_rules_size(size_t file_index) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      return m->get_rules_size(file_index);
    }
  }
  return 0;
}

const char* libkrbn_complex_modifications_assets_manager_get_rule_description(size_t file_index,
                                                                              size_t index) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      return m->get_rule_description(file_index,
                                     index);
    }
  }
  return nullptr;
}

void libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                                                  size_t index,
                                                                                                  libkrbn_core_configuration* core_configuration) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      m->add_rule_to_core_configuration_selected_profile(file_index,
                                                         index,
                                                         core_configuration);
    }
  }
}

bool libkrbn_complex_modifications_assets_manager_user_file(size_t index) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      return m->user_file(index);
    }
  }
  return false;
}

void libkrbn_complex_modifications_assets_manager_erase_file(size_t index) {
  if (libkrbn_components_manager_) {
    if (auto m = libkrbn_components_manager_->get_complex_modifications_assets_manager()) {
      return m->erase_file(index);
    }
  }
}

//
// system_preferences_monitor
//

void libkrbn_enable_system_preferences_monitor(libkrbn_system_preferences_monitor_callback _Nullable callback,
                                               void* _Nullable refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_system_preferences_monitor(callback,
                                                                   refcon);
  }
}

void libkrbn_disable_system_preferences_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_system_preferences_monitor();
  }
}

size_t libkrbn_system_preferences_properties_get_keyboard_types_size(void) {
  libkrbn_system_preferences_properties p;
  return std::size(p.keyboard_types);
}

//
// connected_devices_monitor
//

void libkrbn_enable_connected_devices_monitor(libkrbn_connected_devices_monitor_callback callback,
                                              void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_connected_devices_monitor(callback,
                                                                  refcon);
  }
}

void libkrbn_disable_connected_devices_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_connected_devices_monitor();
  }
}

//
// observer_state_json_file_monitor
//

void libkrbn_enable_observer_state_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                     void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_observer_state_json_file_monitor(callback,
                                                                         refcon);
  }
}

void libkrbn_disable_observer_state_json_file_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_observer_state_json_file_monitor();
  }
}

//
// grabber_state_json_file_monitor
//

void libkrbn_enable_grabber_state_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                    void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_grabber_state_json_file_monitor(callback,
                                                                        refcon);
  }
}

void libkrbn_disable_grabber_state_json_file_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_grabber_state_json_file_monitor();
  }
}

//
// device_details_json_file_monitor
//

void libkrbn_enable_device_details_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                     void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_device_details_json_file_monitor(callback,
                                                                         refcon);
  }
}

void libkrbn_disable_device_details_json_file_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_device_details_json_file_monitor();
  }
}

//
// manipulator_environment_json_file_monitor
//

void libkrbn_enable_manipulator_environment_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                              void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_manipulator_environment_json_file_monitor(callback,
                                                                                  refcon);
  }
}

void libkrbn_disable_manipulator_environment_json_file_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_manipulator_environment_json_file_monitor();
  }
}

//
// notification_message_json_file_monitor
//

void libkrbn_enable_notification_message_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                           void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_notification_message_json_file_monitor(callback,
                                                                               refcon);
  }
}

void libkrbn_disable_notification_message_json_file_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_notification_message_json_file_monitor();
  }
}

const char* libkrbn_get_notification_message_body(void) {
  static std::string message;

  std::ifstream input(krbn::constants::get_notification_message_file_path());
  if (input) {
    try {
      auto json = krbn::json_utility::parse_jsonc(input);
      message = json["body"].get<std::string>();
    } catch (const std::exception& e) {
      message = "";
    }
  }

  return message.c_str();
}

//
// frontmost_application_monitor
//

void libkrbn_enable_frontmost_application_monitor(libkrbn_frontmost_application_monitor_callback callback,
                                                  void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_frontmost_application_monitor(callback,
                                                                      refcon);
  }
}

void libkrbn_disable_frontmost_application_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_frontmost_application_monitor();
  }
}

//
// log_monitor
//

void libkrbn_enable_log_monitor(libkrbn_log_monitor_callback callback,
                                void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_log_monitor(callback,
                                                    refcon);
  }
}

void libkrbn_disable_log_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_log_monitor();
  }
}

//
// hid_value_monitor
//

void libkrbn_enable_hid_value_monitor(libkrbn_hid_value_monitor_callback callback,
                                      void* refcon) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_hid_value_monitor(callback,
                                                          refcon);
  }
}

void libkrbn_disable_hid_value_monitor(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_hid_value_monitor();
  }
}

bool libkrbn_hid_value_monitor_observed(void) {
  if (libkrbn_components_manager_) {
    return libkrbn_components_manager_->hid_value_monitor_observed();
  }
  return false;
}

//
// grabber_client
//

void libkrbn_enable_grabber_client(libkrbn_grabber_client_connected_callback connected_callback,
                                   libkrbn_grabber_client_connect_failed_callback connect_failed_callback,
                                   libkrbn_grabber_client_closed_callback closed_callback) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->enable_grabber_client(connected_callback,
                                                       connect_failed_callback,
                                                       closed_callback);
  }
}

void libkrbn_disable_grabber_client(void) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->disable_grabber_client();
  }
}

void libkrbn_grabber_client_async_set_app_icon(int number) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->grabber_client_async_set_app_icon(number);
  }
}

void libkrbn_grabber_client_async_set_keyboard_type(uint64_t country_code, uint64_t keyboard_type) {
  if (libkrbn_components_manager_) {
    libkrbn_components_manager_->grabber_client_async_set_keyboard_type(
        pqrs::hid::country_code::value_t(country_code),
        pqrs::osx::iokit_keyboard_type::value_t(keyboard_type));
  }
}

void libkrbn_grabber_client_async_set_variable(const char* name, int value) {
  if (libkrbn_components_manager_) {
    if (name) {
      libkrbn_components_manager_->grabber_client_async_set_variable(name, value);
    }
  }
}

void libkrbn_grabber_client_sync_set_variable(const char* name, int value) {
  if (libkrbn_components_manager_) {
    if (name) {
      libkrbn_components_manager_->grabber_client_sync_set_variable(name, value);
    }
  }
}
