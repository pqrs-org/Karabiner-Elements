#pragma once

#include "complex_modifications_utility.hpp"
#include "settings_complex_modifications_assets_manager.hpp"
#include "settings_configuration_snapshot.hpp"
#include "settings_configuration_updater.hpp"
#include <atomic>
#include <cassert>
#include <deque>
#include <mutex>
#include <pqrs/dispatcher.hpp>
#include <spdlog/fmt/ranges.h>
#include <stdexcept>
#include <string_view>
#include <utility>

// Configuration and child objects are accessed only on the dispatcher thread.
// UI callers submit value-only commands and receive serialized snapshots.
class settings_configuration_store final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  using completion = std::function<void(const nlohmann::json&)>;
  using save_function = std::function<void(krbn::core_configuration::core_configuration&)>;

  struct callbacks final {
    completion updated;
    save_function save;
  };

  explicit settings_configuration_store(callbacks callbacks)
      : updated_(std::move(callbacks.updated)),
        save_(std::move(callbacks.save)) {
    dispatcher_client_constructor_guard_.initialize();
  }

  ~settings_configuration_store() override {
    stop();
  }

  [[nodiscard]] static uint64_t current_revision() noexcept {
    return snapshot_revision_.load();
  }

  // Thread-safe admission. The mutex protects only the pending request queue,
  // never the configuration. Every accepted request completes, including at stop.
  void async_execute(nlohmann::json command,
                     completion completed) {
    bool accepted = false;

    {
      std::lock_guard lock(requests_mutex_);

      if (!stopped_) {
        requests_.push_back({
            std::move(command),
            completed,
        });

        accepted = enqueue_to_dispatcher([this] {
          process_next_request();
        });

        if (!accepted) {
          requests_.pop_back();
        }
      }
    }

    if (!accepted) {
      completed({
          {"error", "Settings components are stopped"},
      });
    }
  }

  void stop() {
    std::call_once(
        stop_once_,
        [this] {
          {
            std::lock_guard lock(requests_mutex_);

            stopped_ = true;
          }

          detach_from_dispatcher([this] {
            // detach removes queued dispatcher jobs. Drain their accepted requests
            // explicitly before the final save and lifecycle completion notification.
            while (process_next_request()) {
            }

            save_if_pending();
          });
        });
  }

  // Called by settings_configuration_monitor on the dispatcher thread after the initial
  // successful load, a changed configuration is loaded, or recovery from a load error.
  // Accept the loaded configuration, enable editing and saving, and notify the UI.
  // UI edits modify configuration_ in execute_command instead of calling this method.
  void update(std::shared_ptr<krbn::core_configuration::core_configuration> configuration) {
    assert(dispatcher_thread());

    configuration_ = std::move(configuration);
    configuration_ready_ = true;
    save_pending_ = false;
    save_error_.clear();
    publish();
  }

  // Called by settings_configuration_monitor on the dispatcher thread when the load state
  // changes to an error (for example, invalid JSON or insufficient permissions).
  // Block edits and saves and cancel any pending save so the previous configuration cannot
  // overwrite the file that failed to load. update re-enables them after a successful load.
  void suspend_configuration() {
    assert(dispatcher_thread());

    configuration_ready_ = false;
    updated_({
        {"configuration_unavailable", true},
        {"revision", ++snapshot_revision_},
    });
    save_pending_ = false;
  }

  void set_connected_devices(const krbn::connected_devices& devices) {
    assert(dispatcher_thread());

    connected_devices_ = devices;
    settings_remembered_device_properties::get_instance().remember_connected_devices(devices);
    publish();
  }

private:
  enum class action_type {
    reload_assets,
    erase_asset,
    snapshot,
    sync_save,
    add_rules,
    patch,
    select_profile,
    rename_profile,
    append_profile,
    duplicate_profile,
    move_profile,
    erase_profile,
    replace_simple,
    append_simple,
    erase_simple,
    replace_fn,
    replace_rule,
    prepend_rule,
    move_rule,
    enable_rule,
    erase_rule,
    erase_disconnected_devices,
    set_formula,
    reset_formula,
  };

  [[nodiscard]] static action_type parse_action(std::string_view name) {
    // The JSON protocol uses strings; only the internal dispatch uses this enum.
    static constexpr std::pair<std::string_view, action_type> actions[] = {
        {"reload_assets", action_type::reload_assets},
        {"erase_asset", action_type::erase_asset},
        {"snapshot", action_type::snapshot},
        {"sync_save", action_type::sync_save},
        {"add_rules", action_type::add_rules},
        {"patch", action_type::patch},
        {"select_profile", action_type::select_profile},
        {"rename_profile", action_type::rename_profile},
        {"append_profile", action_type::append_profile},
        {"duplicate_profile", action_type::duplicate_profile},
        {"move_profile", action_type::move_profile},
        {"erase_profile", action_type::erase_profile},
        {"replace_simple", action_type::replace_simple},
        {"append_simple", action_type::append_simple},
        {"erase_simple", action_type::erase_simple},
        {"replace_fn", action_type::replace_fn},
        {"replace_rule", action_type::replace_rule},
        {"prepend_rule", action_type::prepend_rule},
        {"move_rule", action_type::move_rule},
        {"enable_rule", action_type::enable_rule},
        {"erase_rule", action_type::erase_rule},
        {"erase_disconnected_devices", action_type::erase_disconnected_devices},
        {"set_formula", action_type::set_formula},
        {"reset_formula", action_type::reset_formula},
    };

    for (const auto& [command_name, action] : actions) {
      if (name == command_name) {
        return action;
      }
    }
    throw std::runtime_error("Unknown settings command: " + std::string(name));
  }

  struct request {
    nlohmann::json command;
    completion completed;
  };

  bool process_next_request() {
    assert(dispatcher_thread());

    std::optional<request> r;
    {
      std::lock_guard lock(requests_mutex_);

      if (requests_.empty()) {
        return false;
      }

      r = std::move(requests_.front());

      requests_.pop_front();
    }

    auto result = nlohmann::json::object();
    try {
      result = execute_command(r->command);
    } catch (const std::exception& e) {
      if (configuration_ && configuration_ready_) {
        result = make_snapshot();
      }
      result["error"] = e.what();
    }
    r->completed(result);
    return true;
  }

  nlohmann::json execute_command(const nlohmann::json& command) {
    assert(dispatcher_thread());

    const auto action = parse_action(command.at("action").get<std::string>());

    // Commands come from the loaded settings UI. Reject requests arriving before
    // loading completes or while a load error prevents configuration access.
    if (!configuration_ ||
        !configuration_ready_) {
      throw std::runtime_error("Configuration is not ready");
    }
    auto& profile = configuration_->get_selected_profile();

    // No default: adding an action without handling it here must produce a compiler warning.
    switch (action) {
      case action_type::reload_assets: {
        return {
            {"files", assets_.reload_and_get_files_json()},
            {"revision", ++snapshot_revision_},
        };
      }

      case action_type::erase_asset: {
        assets_.erase_file(command.at("file_path").get<std::string>());
        return {
            {"files", assets_.reload_and_get_files_json()},
            {"revision", ++snapshot_revision_},
        };
      }

      case action_type::snapshot: {
        return make_snapshot();
      }

      case action_type::sync_save: {
        // Finish writing before responding so installing the system default profile
        // copies the latest karabiner.json. Scheduling a save could copy the old file.
        // This runs synchronously on the dispatcher; the UI awaits the response asynchronously.
        // Keep the save pending if save_ throws so shutdown can retry it,
        // even when this explicit save was not preceded by an edit.
        save_pending_ = true;
        save_(*configuration_);
        save_pending_ = false;
        save_error_.clear();
        return make_snapshot();
      }

      case action_type::add_rules: {
        for (const auto& rule : command.at("rules")) {
          assets_.add_rule_to_core_configuration_selected_profile(rule.at("file_path").get<std::string>(),
                                                                  rule.at("index"),
                                                                  *configuration_);
        }
        break;
      }

      case action_type::patch: {
        static_cast<void>(settings_configuration_updater::apply_patch(command.at("patch"),
                                                                      *configuration_));
        break;
      }

      case action_type::select_profile: {
        configuration_->select_profile(command.at("index"));
        break;
      }

      case action_type::rename_profile: {
        configuration_->set_profile_name(command.at("index"),
                                         command.at("name"));
        break;
      }

      case action_type::append_profile: {
        configuration_->push_back_profile();
        break;
      }

      case action_type::duplicate_profile: {
        auto index = command.at("index").get<size_t>();
        if (index < configuration_->get_profiles().size()) {
          configuration_->duplicate_profile(*configuration_->get_profiles()[index]);
        }
        break;
      }

      case action_type::move_profile: {
        configuration_->move_profile(command.at("source"),
                                     command.at("destination"));
        break;
      }

      case action_type::erase_profile: {
        configuration_->erase_profile(command.at("index"));
        break;
      }

      case action_type::replace_simple:
      case action_type::append_simple:
      case action_type::erase_simple:
      case action_type::replace_fn: {
        const auto identifiers = krbn::json_utility::parse_jsonc(command.value("device", "{}")).get<krbn::device_identifiers>();

        auto modifications = profile.get_simple_modifications();
        if (action == action_type::replace_fn) {
          modifications = profile.get_fn_function_keys();
        }
        if (!identifiers.empty()) {
          auto device = profile.get_device(identifiers);
          modifications = (action == action_type::replace_fn)
                              ? device->get_fn_function_keys()
                              : device->get_simple_modifications();
        }

        if (action == action_type::append_simple) {
          modifications->push_back_pair();
          // Empty rows are omitted from the saved JSON, so do not schedule a save for this action.
          return make_snapshot();
        } else if (action == action_type::erase_simple) {
          modifications->erase_pair(command.at("index"));
        } else if (action == action_type::replace_fn) {
          modifications->replace_second(command.at("from"),
                                        command.at("to"));
        } else {
          modifications->replace_pair(command.at("index"),
                                      command.at("from"),
                                      command.at("to"));
        }
        break;
      }

      case action_type::replace_rule:
      case action_type::prepend_rule: {
        if (command.contains("expected_profile")) {
          const auto index = command.at("expected_profile").get<size_t>();

          auto codes = nlohmann::json::array();
          for (const auto& rule : profile.get_complex_modifications()->get_rules()) {
            codes.push_back(rule->get_code_string());
          }

          if (index >= configuration_->get_profiles().size() ||
              &*configuration_->get_profiles()[index] != &profile ||
              codes != command.at("expected_rules")) {
            throw std::runtime_error("The editing target has changed");
          }
        }

        auto modifications = profile.get_complex_modifications();
        using rule = krbn::core_configuration::details::complex_modifications_rule;
        auto type = (command.at("code_type") == "javascript")
                        ? rule::code_type::javascript
                        : rule::code_type::json;
        auto r = rule::make_from_code(command.at("code"),
                                      type,
                                      modifications->get_parameters(),
                                      krbn::core_configuration::error_handling::strict);
        auto errors = krbn::complex_modifications_utility::lint_rule(*r);
        if (!errors.empty()) {
          throw std::runtime_error(fmt::format("{}",
                                               fmt::join(errors, "\n")));
        }
        if (action == action_type::replace_rule) {
          modifications->replace_rule(command.at("index"),
                                      r);
        } else {
          modifications->push_front_rule(r);
        }
        break;
      }

      case action_type::move_rule: {
        profile.get_complex_modifications()->move_rule(command.at("source"),
                                                       command.at("destination"));
        break;
      }

      case action_type::enable_rule: {
        const auto& rules = profile.get_complex_modifications()->get_rules();
        auto index = command.at("index").get<size_t>();
        if (index < rules.size()) {
          rules[index]->set_enabled(command.at("enabled"));
        }
        break;
      }

      case action_type::erase_rule: {
        profile.get_complex_modifications()->erase_rule(command.at("index"));
        break;
      }

      case action_type::erase_disconnected_devices: {
        profile.erase_not_connected_configured_devices(connected_devices_);
        break;
      }

      case action_type::set_formula:
      case action_type::reset_formula: {
        auto device = profile.get_device(krbn::json_utility::parse_jsonc(command.at("device").get<std::string>()).get<krbn::device_identifiers>());

        auto formula = command.at("formula").get<std::string>();
        std::string value;
        if (action == action_type::set_formula) {
          value = command.at("value");
          if (!krbn::core_configuration::details::device::validate_stick_formula(value)) {
            throw std::runtime_error("Invalid formula");
          }
        }

        auto set = [&](auto getter, auto setter) {
          if (action == action_type::reset_formula) {
            value = device->find_default_value(((*device).*getter)());
          }
          ((*device).*setter)(value);
        };

        using device_type = krbn::core_configuration::details::device;
        if (formula == "x") {
          set(&device_type::get_game_pad_stick_x_formula,
              &device_type::set_game_pad_stick_x_formula);
        } else if (formula == "y") {
          set(&device_type::get_game_pad_stick_y_formula,
              &device_type::set_game_pad_stick_y_formula);
        } else if (formula == "vertical_wheel") {
          set(&device_type::get_game_pad_stick_vertical_wheel_formula,
              &device_type::set_game_pad_stick_vertical_wheel_formula);
        } else if (formula == "horizontal_wheel") {
          set(&device_type::get_game_pad_stick_horizontal_wheel_formula,
              &device_type::set_game_pad_stick_horizontal_wheel_formula);
        } else {
          throw std::runtime_error("Unknown formula");
        }
        break;
      }
    }

    // All edits reaching this point schedule a save. Non-saving actions return above.
    schedule_save();
    return make_snapshot();
  }

  void schedule_save() {
    save_pending_ = true;
    save_task_.debounce_after(
        [this] {
          save_if_pending();
        },
        std::chrono::milliseconds(200));
  }

  void save_if_pending() {
    assert(dispatcher_thread());

    if (!configuration_ ||
        !configuration_ready_ ||
        !save_pending_) {
      return;
    }

    try {
      save_(*configuration_);
      save_pending_ = false;
      save_error_.clear();
    } catch (const std::exception& e) {
      save_error_ = e.what();
      krbn::logger::get_logger()->error("Failed to save configuration: {0}",
                                        e.what());
    }
    publish();
  }

  [[nodiscard]] nlohmann::json make_snapshot() {
    assert(dispatcher_thread());

    // Keep an editable row in the actual data so snapshot indices can be used directly
    // by replace_simple. Empty rows are omitted from the saved configuration.
    auto& profile = configuration_->get_selected_profile();
    if (profile.get_simple_modifications()->get_pairs().empty()) {
      profile.get_simple_modifications()->push_back_pair();
    }

    // Materialize remembered devices before iterating over the profile's devices.
    for (const auto& properties : settings_remembered_device_properties::get_instance().get_device_properties()) {
      static_cast<void>(profile.get_device(properties->get_device_identifiers()));
    }
    for (const auto& device : profile.get_devices()) {
      if (device->get_simple_modifications()->get_pairs().empty()) {
        device->get_simple_modifications()->push_back_pair();
      }
    }

    return {{"snapshot", settings_configuration_snapshot(*configuration_).to_json()},
            {"revision", ++snapshot_revision_},
            {"not_connected_configured_devices_count", configuration_->get_selected_profile().not_connected_configured_devices_count(connected_devices_)},
            {"save_error", save_error_}};
  }

  void publish() {
    if (configuration_ &&
        configuration_ready_) {
      updated_(make_snapshot());
    }
  }

  std::mutex requests_mutex_;
  std::deque<request> requests_;

  bool stopped_ = false;
  std::once_flag stop_once_;

  std::shared_ptr<krbn::core_configuration::core_configuration> configuration_;
  krbn::connected_devices connected_devices_;
  settings_complex_modifications_assets_manager assets_;

  completion updated_;
  save_function save_;

  bool configuration_ready_ = false;
  bool save_pending_ = false;
  std::string save_error_;

  inline static std::atomic<uint64_t> snapshot_revision_{0};

  // Construct after potentially throwing members; destruction requires detach.
  pqrs::dispatcher::extra::debounced_task save_task_{*this};
};
