#pragma once

#include "configuration_core.hpp"
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

    auto configuration_core_file_path = constants::get_configuration_core_file_path();

    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {constants::get_configuration_directory(), {configuration_core_file_path}},
    };
    file_monitor_ = std::make_unique<file_monitor>(logger_, targets, std::bind(&configuration_manager::reload_configuration_core, this, std::placeholders::_1));

    reload_configuration_core(configuration_core_file_path);
  }

  ~configuration_manager(void) {
    file_monitor_ = nullptr;
  }

private:
  void reload_configuration_core(const std::string& file_path) {
    auto new_ptr = std::make_unique<configuration_core>(logger_, file_path);
    // skip if karabiner.json is broken.
    if (configuration_core_ && !new_ptr->is_loaded()) {
      return;
    }

    configuration_core_ = std::move(new_ptr);
    logger_.info("configuration_core_ was loaded.");

    grabber_client_.clear_simple_modifications();
    for (const auto& pair : configuration_core_->get_current_profile_simple_modifications()) {
      grabber_client_.add_simple_modification(pair.first, pair.second);
    }

    grabber_client_.clear_fn_function_keys();
    for (const auto& pair : configuration_core_->get_current_profile_fn_function_keys()) {
      grabber_client_.add_fn_function_key(pair.first, pair.second);
    }
  }

  spdlog::logger& logger_;
  grabber_client& grabber_client_;

  std::unique_ptr<file_monitor> file_monitor_;

  std::unique_ptr<configuration_core> configuration_core_;
};
