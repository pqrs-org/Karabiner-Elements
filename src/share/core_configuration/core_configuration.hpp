#pragma once

#include "connected_devices/connected_devices.hpp"
#include "constants.hpp"
#include "details/global_configuration.hpp"
#include "details/profile.hpp"
#include "details/profile/complex_modifications.hpp"
#include "details/profile/device.hpp"
#include "details/profile/simple_modifications.hpp"
#include "details/profile/virtual_hid_keyboard.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <fstream>
#include <glob.h>
#include <nlohmann/json.hpp>
#include <pqrs/filesystem.hpp>
#include <pqrs/osx/session.hpp>
#include <string>
#include <unordered_map>

// Example: tests/src/core_configuration/json/example.json

namespace krbn {
namespace core_configuration {
using namespace std::string_literals;

class core_configuration final {
public:
  core_configuration(const core_configuration&) = delete;

  core_configuration(const std::string& file_path) : loaded_(false),
                                                     global_configuration_(nlohmann::json()) {
    bool valid_file_owner = false;

    // Load karabiner.json only when the owner is root or current session user.
    if (pqrs::filesystem::exists(file_path)) {
      if (pqrs::filesystem::is_owned(file_path, 0)) {
        valid_file_owner = true;
      } else {
        if (auto console_user_id = pqrs::osx::session::find_console_user_id()) {
          if (pqrs::filesystem::is_owned(file_path, *console_user_id)) {
            valid_file_owner = true;
          }
        }
      }

      if (!valid_file_owner) {
        logger::get_logger()->warn("{0} is not owned by a valid user.", file_path);

      } else {
        std::ifstream input(file_path);
        if (input) {
          try {
            json_ = nlohmann::json::parse(input);

            if (auto v = json_utility::find_object(json_, "global")) {
              global_configuration_ = details::global_configuration(*v);
            }

            if (auto v = json_utility::find_array(json_, "profiles")) {
              for (const auto& profile_json : *v) {
                profiles_.emplace_back(profile_json);
              }
            }

            loaded_ = true;

          } catch (std::exception& e) {
            logger::get_logger()->error("parse error in {0}: {1}", file_path, e.what());
            json_ = nlohmann::json();
          }
        } else {
          logger::get_logger()->error("Failed to open {0}", file_path);
        }
      }
    }

    // Fallbacks
    if (profiles_.empty()) {
      profiles_.emplace_back(nlohmann::json({
          {"name", "Default profile"},
          {"selected", true},
      }));
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["global"] = global_configuration_.to_json();
    j["profiles"] = profiles_;
    return j;
  }

  bool is_loaded(void) const { return loaded_; }

  const details::global_configuration& get_global_configuration(void) const {
    return global_configuration_;
  }
  details::global_configuration& get_global_configuration(void) {
    return global_configuration_;
  }

  const std::vector<details::profile>& get_profiles(void) const {
    return profiles_;
  }
  void set_profile_name(size_t index, const std::string name) {
    if (index < profiles_.size()) {
      profiles_[index].set_name(name);
    }
  }
  void select_profile(size_t index) {
    if (index < profiles_.size()) {
      for (size_t i = 0; i < profiles_.size(); ++i) {
        if (i == index) {
          profiles_[i].set_selected(true);
        } else {
          profiles_[i].set_selected(false);
        }
      }
    }
  }
  void push_back_profile(void) {
    profiles_.emplace_back(nlohmann::json({
        {"name", "New profile"},
    }));
  }
  void erase_profile(size_t index) {
    if (index < profiles_.size()) {
      if (profiles_.size() > 1) {
        profiles_.erase(profiles_.begin() + index);
      }
    }
  }

  const details::profile& get_selected_profile(void) const {
    for (auto&& profile : profiles_) {
      if (profile.get_selected()) {
        return profile;
      }
    }
    return profiles_[0];
  }
  details::profile& get_selected_profile(void) {
    return const_cast<details::profile&>(static_cast<const core_configuration&>(*this).get_selected_profile());
  }

  // Note:
  // Be careful calling `save` method.
  // If the configuration file is corrupted temporarily (user editing the configuration file in editor),
  // the user data will be lost by the `save` method.
  // Thus, we should call the `save` method only when it is neccessary.

  void sync_save_to_file(void) {
    make_backup_file();
    remove_old_backup_files();

    auto file_path = constants::get_user_core_configuration_file_path();
    json_utility::sync_save_to_file(to_json(), file_path, 0700, 0600);
  }

private:
  void make_backup_file(void) {
    auto file_path = constants::get_user_core_configuration_file_path();

    if (!pqrs::filesystem::exists(file_path)) {
      return;
    }

    auto backups_directory = constants::get_user_core_configuration_automatic_backups_directory();
    if (backups_directory.empty()) {
      return;
    }

    pqrs::filesystem::create_directory_with_intermediate_directories(backups_directory, 0700);

    auto backup_file_path = backups_directory +
                            "/karabiner_" + make_current_local_yyyymmdd_string() + ".json";
    if (pqrs::filesystem::exists(backup_file_path)) {
      return;
    }

    pqrs::filesystem::copy(file_path, backup_file_path);
  }

  void remove_old_backup_files(void) {
    auto backups_directory = constants::get_user_core_configuration_automatic_backups_directory();
    if (backups_directory.empty()) {
      return;
    }

    auto pattern = backups_directory + "/karabiner_????????.json";

    glob_t glob_result;
    if (glob(pattern.c_str(), 0, nullptr, &glob_result) == 0) {
      const int keep_count = 20;
      for (int i = 0; i < static_cast<int>(glob_result.gl_pathc) - keep_count; ++i) {
        unlink(glob_result.gl_pathv[i]);
      }
    }
    globfree(&glob_result);
  }

  static std::string make_current_local_yyyymmdd_string(void) {
    auto t = time(nullptr);

    tm tm;
    localtime_r(&t, &tm);

    return fmt::format("{0:04d}{1:02d}{2:02d}",
                       tm.tm_year + 1900,
                       tm.tm_mon + 1,
                       tm.tm_mday);
  }

  nlohmann::json json_;
  bool loaded_;

  details::global_configuration global_configuration_;
  std::vector<details::profile> profiles_;
};
} // namespace core_configuration
} // namespace krbn
