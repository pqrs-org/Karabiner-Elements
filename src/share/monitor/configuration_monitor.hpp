#pragma once

// `krbn::configuration_monitor` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "filesystem_utility.hpp"
#include "logger.hpp"
#include <chrono>
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

  configuration_monitor(const std::optional<std::string>& user_core_configuration_file_path,
                        uid_t expected_user_core_configuration_file_owner,
                        core_configuration::error_handling error_handling,
                        const std::string& system_core_configuration_file_path = constants::get_system_core_configuration_file_path())
      : dispatcher_client(),
        user_core_configuration_file_path_(user_core_configuration_file_path),
        expected_user_core_configuration_file_owner_(expected_user_core_configuration_file_owner),
        error_handling_(error_handling),
        system_core_configuration_file_path_(system_core_configuration_file_path),
        file_removal_task_(*this) {
    std::vector<std::string> targets;
    if (user_core_configuration_file_path_) {
      targets.push_back(*user_core_configuration_file_path_);
    }
    targets.push_back(system_core_configuration_file_path_);

    file_monitor_ = std::make_unique<pqrs::osx::file_monitor>(weak_dispatcher_,
                                                              targets);

    file_monitor_->file_changed.connect([this](auto&& changed_file_path,
                                               auto&& changed_file_body) {
      // A non-null body means that a readable file is available, so it can be processed immediately.
      // Before the initial load state is known, a null body represents the initial missing or
      // unreadable state rather than removal of a previously loaded file, so process that immediately too.
      if (changed_file_body || !load_state_) {
        file_removal_task_.cancel();
        handle_file_changed(changed_file_path);
      } else {
        //
        // Handle the case where a configuration file disappears after it has been loaded once.
        //

        if (user_core_configuration_file_path_ &&
            changed_file_path == system_core_configuration_file_path_ &&
            path_may_exist(*user_core_configuration_file_path_)) {
          // The system configuration is not active while the user configuration exists.
          return;
        }

        // Editors may replace a file by removing and recreating it. Wait briefly before treating
        // the removal as final so clients do not observe a transient default configuration.
        file_removal_task_.debounce_after(
            [this, changed_file_path] {
              handle_file_changed(changed_file_path);
            },
            file_removal_delay);
      }
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
  void handle_file_changed(const std::string& changed_file_path) {
    auto file_path = changed_file_path;
    if (user_core_configuration_file_path_ &&
        path_may_exist(*user_core_configuration_file_path_)) {
      if (changed_file_path == system_core_configuration_file_path_) {
        // system_core_configuration_file_path_ is updated.
        // We ignore it because we are using user_core_configuration_file_path_.
        return;
      }

      file_path = *user_core_configuration_file_path_;

    } else if (path_may_exist(system_core_configuration_file_path_)) {
      file_path = system_core_configuration_file_path_;

    } else {
      // Use a missing user path, if one was specified, to load the default configuration.
      file_path = user_core_configuration_file_path_.value_or(system_core_configuration_file_path_);
    }

    if (path_may_exist(file_path)) {
      logger::get_logger()->info("Load {0}...", file_path);
    }

    auto c = std::make_shared<core_configuration::core_configuration>(file_path,
                                                                      expected_user_core_configuration_file_owner_,
                                                                      error_handling_);

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
  }

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

  static constexpr auto file_removal_delay = std::chrono::milliseconds(1000);

  const std::optional<std::string> user_core_configuration_file_path_;
  const uid_t expected_user_core_configuration_file_owner_;
  const core_configuration::error_handling error_handling_;
  const std::string system_core_configuration_file_path_;

  std::unique_ptr<pqrs::osx::file_monitor> file_monitor_;
  pqrs::dispatcher::extra::debounced_task file_removal_task_;
  std::shared_ptr<core_configuration::core_configuration> core_configuration_;
  std::optional<core_configuration::core_configuration::load_state> load_state_;
  std::string parse_error_message_;
};
} // namespace krbn
