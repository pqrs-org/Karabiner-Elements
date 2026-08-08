#pragma once

// `krbn::configuration_monitor` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "filesystem_utility.hpp"
#include "logger.hpp"
#include <filesystem>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/osx/file_monitor.hpp>

namespace krbn {
class configuration_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(std::weak_ptr<core_configuration::core_configuration>)> core_configuration_updated;
  nod::signal<void(core_configuration::core_configuration::load_state)> load_state_changed;
  nod::signal<void(const std::string&)> parse_error_message_changed;

  // Methods

  configuration_monitor(const std::string& user_core_configuration_file_path,
                        uid_t expected_user_core_configuration_file_owner,
                        core_configuration::error_handling error_handling,
                        const std::string& system_core_configuration_file_path = constants::get_system_core_configuration_file_path()) : dispatcher_client() {
    std::vector<std::string> targets = {
        user_core_configuration_file_path,
        system_core_configuration_file_path,
    };

    file_monitor_ = std::make_unique<pqrs::osx::file_monitor>(weak_dispatcher_,
                                                              targets);

    file_monitor_->file_changed.connect([this,
                                         user_core_configuration_file_path,
                                         expected_user_core_configuration_file_owner,
                                         error_handling,
                                         system_core_configuration_file_path](auto&& changed_file_path,
                                                                              auto&& changed_file_body) {
      auto file_path = changed_file_path;
      if (path_may_exist(user_core_configuration_file_path)) {
        // Note:
        // user_core_configuration_file_path == system_core_configuration_file_path
        // if console_user_server is not running.

        if (changed_file_path != user_core_configuration_file_path &&
            changed_file_path == system_core_configuration_file_path) {
          // system_core_configuration_file_path is updated.
          // We ignore it because we are using user_core_configuration_file_path.
          return;
        }
      } else {
        if (changed_file_path == user_core_configuration_file_path) {
          // user_core_configuration_file_path is removed.

          if (path_may_exist(system_core_configuration_file_path)) {
            file_path = system_core_configuration_file_path;
          }
        }
      }

      if (path_may_exist(file_path)) {
        logger::get_logger()->info("Load {0}...", file_path);
      }

      auto c = std::make_shared<core_configuration::core_configuration>(file_path,
                                                                        expected_user_core_configuration_file_owner,
                                                                        error_handling);

      auto previous_load_state = load_state_;
      auto load_state = c->get_load_state();
      if (!load_state_ || *load_state_ != load_state) {
        load_state_ = load_state;
        enqueue_to_dispatcher([this, load_state] {
          this->load_state_changed(load_state);
        });
      }

      // If a parse error occurs, parse_error_message_changed should be called, but core_configuration_updated should not.
      // Therefore, we handle the parse error first.
      auto parse_error_message = c->get_parse_error_message();

      if (parse_error_message_ != parse_error_message) {
        parse_error_message_ = parse_error_message;
        enqueue_to_dispatcher([this, parse_error_message] {
          this->parse_error_message_changed(parse_error_message);
        });
      }

      if (load_state != core_configuration::core_configuration::load_state::loaded) {
        return;
      }

      auto recovered_from_error = previous_load_state &&
                                  *previous_load_state != core_configuration::core_configuration::load_state::loaded;
      if (!recovered_from_error &&
          core_configuration_ &&
          core_configuration_->to_json() == c->to_json()) {
        return;
      }

      core_configuration_ = c;

      logger::get_logger()->info("core_configuration is updated.");

      enqueue_to_dispatcher([this, c] {
        core_configuration_updated(c);
      });
    });
  }

  ~configuration_monitor() override {
    detach_from_dispatcher([this] {
      file_monitor_ = nullptr;
    });
  }

  void async_start() {
    file_monitor_->async_start();
  }

private:
  // Unlike std::filesystem::exists, this treats errors such as permission_denied as evidence that
  // the path is not missing. This prevents falling back to another configuration when the intended
  // configuration exists but cannot be accessed.
  [[nodiscard]] static bool path_may_exist(const std::filesystem::path& path) {
    std::error_code error;
    auto status = std::filesystem::status(path, error);
    if (!error) {
      return std::filesystem::exists(status);
    }

    return error != std::errc::no_such_file_or_directory &&
           error != std::errc::not_a_directory;
  }

  std::unique_ptr<pqrs::osx::file_monitor> file_monitor_;

  std::shared_ptr<core_configuration::core_configuration> core_configuration_;
  std::optional<core_configuration::core_configuration::load_state> load_state_;
  std::string parse_error_message_;
};
} // namespace krbn
