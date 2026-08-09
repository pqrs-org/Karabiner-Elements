#include "settings.hpp"
#include "app_icon.hpp"
#include "application_launcher.hpp"
#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "filesystem_utility.hpp"
#include "json_utility.hpp"
#include "process_lifecycle_manager.hpp"
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

class settings_process_lifecycle_components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_process_lifecycle_components_manager(const settings_process_lifecycle_components_manager&) = delete;

  settings_process_lifecycle_components_manager(const settings_components_manager::callbacks& callbacks,
                                                krbn_components_manager_stopped_t components_manager_stopped_callback)
      : dispatcher_client(),
        components_manager_(std::make_shared<settings_components_manager>(callbacks)),
        components_manager_stopped_callback_(components_manager_stopped_callback) {
    settings_cpp::set_components_manager(components_manager_);
  }

  ~settings_process_lifecycle_components_manager() override {
    detach_from_dispatcher([this] {
      components_manager_->sync_save_core_configuration_if_pending();
      settings_cpp::set_components_manager(std::weak_ptr<settings_components_manager>());
      components_manager_ = nullptr;
      components_manager_stopped_callback_();
    });
  }

  void async_start() {
    components_manager_->async_start();
  }

private:
  std::shared_ptr<settings_components_manager> components_manager_;
  const krbn_components_manager_stopped_t components_manager_stopped_callback_;
};
} // namespace

std::weak_ptr<settings_components_manager> settings_components_manager_;
std::mutex settings_components_manager_mutex_;

void krbn_initialize(krbn_core_configuration_updated_t core_configuration_updated_callback,
                     krbn_core_configuration_load_state_changed_t core_configuration_load_state_changed_callback,
                     krbn_log_messages_updated_t log_messages_updated_callback,
                     krbn_core_service_daemon_client_connected_devices_received_t connected_devices_received_callback,
                     krbn_core_service_daemon_client_system_variables_received_t system_variables_received_callback,
                     krbn_console_user_server_client_status_changed_t console_user_server_client_status_changed_callback,
                     krbn_console_user_server_client_settings_window_guidance_received_t settings_window_guidance_received_callback,
                     krbn_components_manager_stopped_t components_manager_stopped_callback) {
  krbn::logger::get_logger()->debug(__func__);

  if (!scoped_dispatcher_manager_) {
    scoped_dispatcher_manager_ = krbn::dispatcher_utility::initialize_dispatchers();
  }

  if (!scoped_run_loop_thread_manager_) {
    scoped_run_loop_thread_manager_ = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
        pqrs::cf::run_loop_thread::failure_policy::exit);
  }

  auto callbacks = settings_components_manager::callbacks{
      .core_configuration_updated = core_configuration_updated_callback,
      .core_configuration_load_state_changed = core_configuration_load_state_changed_callback,
      .log_messages_updated = log_messages_updated_callback,
      .connected_devices_received = connected_devices_received_callback,
      .system_variables_received = system_variables_received_callback,
      .console_user_server_client_status_changed = console_user_server_client_status_changed_callback,
      .settings_window_guidance_received = settings_window_guidance_received_callback,
  };
  krbn::process_lifecycle_manager::initialize_shared_instance(
      krbn::process_lifecycle_manager::configuration{
          .components_manager_maker =
              [callbacks, components_manager_stopped_callback] {
                return std::make_unique<settings_process_lifecycle_components_manager>(
                    callbacks,
                    components_manager_stopped_callback);
              },
          .termination_completion_handler = [] {},
      });
  krbn::process_lifecycle_manager::async_start();
}

void krbn_terminate() {
  krbn::logger::get_logger()->debug(__func__);

  krbn::process_lifecycle_manager::terminate_shared_instance();

  settings_cpp::set_components_manager(std::weak_ptr<settings_components_manager>());

  scoped_run_loop_thread_manager_ = nullptr;

  scoped_dispatcher_manager_ = nullptr;
}

void krbn_load_custom_environment_variables() {
  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();
  krbn::environment_variable_utility::log(environment_variables);
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
// complex_modifications_assets_manager
//

void krbn_complex_modifications_assets_manager_reload(krbn_json_output_callback output) {
  auto json = nlohmann::json::array();

  if (auto manager = settings_cpp::get_components_manager()) {
    json = manager->reload_complex_modifications_assets();
  }

  auto json_string = krbn::json_utility::dump(json);
  output(json_string.data(), json_string.size());
}

void krbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                                               size_t index) {
  if (auto manager = settings_cpp::get_components_manager()) {
    manager->add_complex_modifications_rule_to_core_configuration_selected_profile(file_index,
                                                                                   index);
  }
}

void krbn_complex_modifications_assets_manager_erase_file(size_t index) {
  if (auto manager = settings_cpp::get_components_manager()) {
    manager->erase_complex_modifications_asset_file(index);
  }
}

//
// core_service_client
//

void krbn_core_service_daemon_client_async_set_app_icon(int number) {
  if (auto manager = settings_cpp::get_components_manager()) {
    manager->async_set_app_icon(number);
  }
}

//
// console_user_server_client
//

bool krbn_console_user_server_client_connected() {
  if (auto manager = settings_cpp::get_components_manager()) {
    return manager->console_user_server_client_connected();
  }

  return false;
}
