#pragma once

#include "core_configuration.hpp"
#include "constants.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "grabber_client.hpp"
#include <CoreServices/CoreServices.h>
#include <memory>

class configuration_manager final {
public:
  configuration_manager(const configuration_manager&) = delete;

  configuration_manager(spdlog::logger& logger,
                        grabber_client& grabber_client) : logger_(logger),
                                                          grabber_client_(grabber_client) {
    mkdir(constants::get_configuration_directory(), 0700);

    auto core_configuration_file_path = constants::get_core_configuration_file_path();

    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {constants::get_configuration_directory(), {core_configuration_file_path}},
    };
    file_monitor_ = std::make_unique<file_monitor>(logger_, targets, std::bind(&configuration_manager::reload_core_configuration, this, std::placeholders::_1));

    reload_core_configuration(core_configuration_file_path);
  }

  ~configuration_manager(void) {
    file_monitor_ = nullptr;
  }

private:
  void reload_core_configuration(const std::string& file_path) {
    auto new_ptr = std::make_unique<core_configuration>(logger_, file_path);
    // skip if karabiner.json is broken.
    if (core_configuration_ && !new_ptr->is_loaded()) {
      return;
    }

    core_configuration_ = std::move(new_ptr);
    logger_.info("core_configuration_ was loaded.");

    grabber_client_.clear_simple_modifications();
    for (const auto& pair : core_configuration_->get_current_profile_simple_modifications()) {
      grabber_client_.add_simple_modification(pair.first, pair.second);
    }

    grabber_client_.clear_fn_function_keys();
    for (const auto& pair : core_configuration_->get_current_profile_fn_function_keys()) {
      grabber_client_.add_fn_function_key(pair.first, pair.second);
    }

    grabber_client_.clear_devices();
    for (const auto& tuple: core_configuration_->get_current_profile_devices()) {
      grabber_client_.add_device(std::get<0>(tuple), std::get<1>(tuple), std::get<2>(tuple));
    }
    grabber_client_.complete_devices();
  }

  spdlog::logger& logger_;
  grabber_client& grabber_client_;

  std::unique_ptr<file_monitor> file_monitor_;

  std::unique_ptr<core_configuration> core_configuration_;
};
