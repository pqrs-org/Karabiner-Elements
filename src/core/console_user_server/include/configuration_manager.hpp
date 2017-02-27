#pragma once

#include "application_launcher.hpp"
#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "core_configuration.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "grabber_client.hpp"
#include "update_utility.hpp"
#include <CoreServices/CoreServices.h>
#include <memory>

class configuration_manager final {
public:
  configuration_manager(const configuration_manager&) = delete;

  configuration_manager(spdlog::logger& logger,
                        grabber_client& grabber_client) : logger_(logger),
                                                          grabber_client_(grabber_client),
                                                          need_to_check_for_updates_(true) {
    filesystem::create_directory_with_intermediate_directories(constants::get_user_configuration_directory(), 0700);

    configuration_monitor_ = std::make_unique<configuration_monitor>(logger_,
                                                                     constants::get_user_core_configuration_file_path(),
                                                                     std::bind(&configuration_manager::core_configuration_updated_callback, this, std::placeholders::_1));
  }

private:
  void core_configuration_updated_callback(core_configuration& core_configuration) {
    // ----------------------------------------
    // Send configuration to grabber.

    grabber_client_.core_configuration_updated();

    grabber_client_.clear_simple_modifications();
    for (const auto& pair : core_configuration.get_selected_profile().get_simple_modifications_key_code_map(logger_)) {
      grabber_client_.add_simple_modification(pair.first, pair.second);
    }

    grabber_client_.clear_fn_function_keys();
    for (const auto& pair : core_configuration.get_selected_profile().get_fn_function_keys_key_code_map(logger_)) {
      grabber_client_.add_fn_function_key(pair.first, pair.second);
    }

    {
      krbn::virtual_hid_keyboard_configuration_struct virtual_hid_keyboard_configuration_struct;
      if (auto keyboard_type = krbn::types::get_keyboard_type(core_configuration.get_selected_profile().get_virtual_hid_keyboard().get_keyboard_type())) {
        virtual_hid_keyboard_configuration_struct.keyboard_type = *keyboard_type;
      }
      virtual_hid_keyboard_configuration_struct.caps_lock_delay_milliseconds = core_configuration.get_selected_profile().get_virtual_hid_keyboard().get_caps_lock_delay_milliseconds();

      grabber_client_.virtual_hid_keyboard_configuration_updated(virtual_hid_keyboard_configuration_struct);
    }

    grabber_client_.clear_devices();
    for (const auto& device : core_configuration.get_selected_profile().get_devices()) {
      krbn::device_identifiers_struct identifiers;
      identifiers.vendor_id = device.get_identifiers().get_vendor_id();
      identifiers.product_id = device.get_identifiers().get_product_id();
      identifiers.is_keyboard = device.get_identifiers().get_is_keyboard();
      identifiers.is_pointing_device = device.get_identifiers().get_is_pointing_device();

      krbn::device_configuration_struct configuration;
      configuration.ignore = device.get_ignore();
      configuration.disable_built_in_keyboard_if_exists = device.get_disable_built_in_keyboard_if_exists();

      grabber_client_.add_device(identifiers, configuration);
    }
    grabber_client_.complete_devices();

    // ----------------------------------------
    // Check for updates
    if (need_to_check_for_updates_) {
      need_to_check_for_updates_ = false;
      if (core_configuration.get_global_configuration().get_check_for_updates_on_startup()) {
        logger_.info("Check for updates...");
        update_utility::check_for_updates_in_background();
      }
    }

    // ----------------------------------------
    // Launch menu
    if (core_configuration.get_global_configuration().get_show_in_menu_bar()) {
      application_launcher::launch_menu();
    }
  }

  spdlog::logger& logger_;
  grabber_client& grabber_client_;

  std::unique_ptr<configuration_monitor> configuration_monitor_;

  bool need_to_check_for_updates_;
};
