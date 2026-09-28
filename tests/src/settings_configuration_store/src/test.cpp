#include "../../../../src/apps/SettingsWindow/src/cpp/settings_configuration_store.hpp"
#include "dispatcher_utility.hpp"
#include <boost/ut.hpp>
#include <future>

int main() {
  using namespace boost::ut;

  // CMake compiles this file using its absolute path, so this does not depend on the working directory.
  // Give each run its own directory to isolate asset deletion tests from other runs and user settings.
  const auto temporary_root = std::filesystem::path(__FILE__).parent_path() / "tmp";
  std::filesystem::create_directories(temporary_root);
  auto temporary_directory = (temporary_root / "karabiner-settings-assets-XXXXXX").string();
  if (!mkdtemp(temporary_directory.data())) {
    return 1;
  }
  setenv("XDG_CONFIG_HOME", temporary_directory.c_str(), 1);
  auto dispatcher = krbn::dispatcher_utility::initialize_dispatchers();

  "shutdown_drains_edits_in_order_before_saving"_test = [] {
    std::vector<nlohmann::json> responses;
    nlohmann::json saved;
    bool callbacks_on_dispatcher = true;
    int saves = 0;
    settings_configuration_store store({
        .updated = [](const auto&) {},
        .save =
            [&](auto& configuration) {
              saved = configuration.to_json();
              ++saves;
            },
    });
    std::promise<void> done;
    store.enqueue_to_dispatcher([&] {
      store.update(std::make_shared<krbn::core_configuration::core_configuration>());
      auto completed = [&](const auto& result) {
        callbacks_on_dispatcher &= store.dispatcher_thread();
        responses.push_back(result);
      };
      store.async_execute({
                              {"action", "append_profile"},
                          },
                          completed);
      store.async_execute({
                              {"action", "rename_profile"},
                              {"index", 1},
                              {"name", "New profile"},
                          },
                          completed);
      store.async_execute({
                              {"action", "select_profile"},
                              {"index", 1},
                          },
                          completed);
      store.async_execute({
                              {"action", "replace_simple"},
                              {"index", 0},
                              {"from", R"({"key_code":"a"})"},
                              {"to", R"([{"key_code":"b"}])"},
                          },
                          completed);
      // These commands have been accepted but their dispatcher jobs have not run.
      // stop must execute them before saving, rather than discard them at detach.
      store.stop();
      done.set_value();
    });
    expect(done.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    expect(callbacks_on_dispatcher);
    expect(responses.size() == size_t(4));
    expect(saves == 1_i);
    expect(saved.at("profiles") == nlohmann::json::parse(R"([
      {
        "name": "Default profile"
      },
      {
        "name": "New profile",
        "selected": true,
        "simple_modifications": [
          {
            "from": { "key_code": "a" },
            "to": [{ "key_code": "b" }]
          }
        ]
      }
    ])"));
    for (size_t i = 1; i < responses.size(); ++i) {
      expect(responses[i].at("revision") > responses[i - 1].at("revision"));
    }
    int rejected = 0;
    store.async_execute({{"action", "append_profile"}}, [&](const auto& result) {
      expect(result.contains("error"));
      ++rejected;
    });
    expect(rejected == 1_i);
  };

  "commands_require_a_loaded_configuration"_test = [] {
    int saves = 0;
    std::vector<nlohmann::json> responses;
    settings_configuration_store store({
        .updated = [](const auto&) {},
        .save = [&](auto&) { ++saves; },
    });
    std::promise<void> done;
    store.enqueue_to_dispatcher([&] {
      auto completed = [&](const auto& result) {
        responses.push_back(result);
      };
      store.async_execute({
                              {"action", "reload_assets"},
                          },
                          completed);
      store.async_execute({
                              {"action", "append_profile"},
                          },
                          completed);
      store.stop();
      done.set_value();
    });
    expect(done.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    expect(saves == 0_i);
    expect(responses.size() == size_t(2));
    for (const auto& response : responses) {
      expect(response.at("error") == "Configuration is not ready");
      expect(!response.contains("snapshot"));
    }
  };

  "empty_rows_and_snapshot_requests_do_not_schedule_saves"_test = [] {
    int saves = 0;
    std::vector<nlohmann::json> responses;
    settings_configuration_store store({
        .updated = [](const auto&) {},
        .save = [&](auto&) { ++saves; },
    });
    std::promise<void> done;
    store.enqueue_to_dispatcher([&] {
      store.update(std::make_shared<krbn::core_configuration::core_configuration>());
      auto completed = [&](const auto& result) {
        responses.push_back(result);
      };
      store.async_execute({
                              {"action", "append_simple"},
                          },
                          completed);
      store.async_execute({
                              {"action", "snapshot"},
                          },
                          completed);
      store.async_execute({
                              {"action", "unknown"},
                          },
                          completed);
      store.stop();
      done.set_value();
    });
    expect(done.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    expect(saves == 0_i);
    expect(responses.at(0).at("snapshot").at("selected_profile").at("simple_modifications").size() == size_t(2));
    expect(responses.at(1).at("snapshot") == responses.at(0).at("snapshot"));
    expect(responses.at(2).at("error") == "Unknown settings command: unknown");
  };

  "snapshots_keep_editable_empty_rows_for_profile_and_devices"_test = [] {
    settings_configuration_store store({
        .updated = [](const auto&) {},
        .save = [](auto&) {},
    });
    std::promise<void> done;
    store.enqueue_to_dispatcher([&] {
      auto configuration = std::make_shared<krbn::core_configuration::core_configuration>();
      const auto original_json = configuration->to_json();
      store.update(configuration);

      krbn::connected_devices devices;
      const auto properties = krbn::device_properties::make_device_properties(nlohmann::json::parse(R"({
        "device_id": 54321,
        "device_identifiers": {"vendor_id": 789, "product_id": 123, "is_keyboard": true}
      })"));
      devices.push_back_device(properties);
      store.set_connected_devices(devices);
      const auto device_key = properties->get_device_identifiers().to_normalized_json().dump();
      const auto empty_rows = nlohmann::json::parse(R"([
        {"index": 0, "from_json_string": "{}", "to_json_string": "[]"}
      ])");

      // Automatic rows do not change the configuration written to disk.
      expect(configuration->to_json() == original_json);
      for (const auto& key : {std::string("{}"), device_key}) {
        auto rows = [key](const auto& result) {
          const auto& profile = result.at("snapshot").at("selected_profile");
          return key == "{}" ? profile.at("simple_modifications")
                             : profile.at("devices").at(key).at("simple_modifications");
        };
        store.async_execute({{"action", "snapshot"}}, [&, rows](const auto& result) {
          expect(rows(result) == empty_rows);
        });
        // Index 0 must refer to an actual row, without an append_simple command.
        store.async_execute({{"action", "replace_simple"},
                             {"device", key},
                             {"index", 0},
                             {"from", R"({"key_code":"a"})"},
                             {"to", R"([{"key_code":"b"}])"}},
                            [rows](const auto& result) {
                              expect(rows(result) == nlohmann::json::parse(R"([
                                {"index": 0, "from_json_string": "{\"key_code\":\"a\"}",
                                 "to_json_string": "[{\"key_code\":\"b\"}]"}
                              ])"));
                            });
        // Removing the last row restores one blank row; later snapshots do not add more.
        for (const auto& action : {"erase_simple", "snapshot"}) {
          store.async_execute({{"action", action}, {"device", key}, {"index", 0}},
                              [&, rows](const auto& result) {
                                expect(rows(result) == empty_rows);
                              });
        }
      }
      store.stop();
      expect(configuration->to_json() == original_json);
      done.set_value();
    });
    expect(done.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
  };

  "device_counts_and_validation_are_computed_on_dispatcher"_test = [] {
    std::vector<nlohmann::json> responses;
    std::vector<nlohmann::json> notifications;
    settings_configuration_store store({
        .updated = [&](const auto& value) { notifications.push_back(value); },
        .save = [](auto&) {},
    });
    std::promise<void> done;
    store.enqueue_to_dispatcher([&] {
      auto configuration = std::make_shared<krbn::core_configuration::core_configuration>();
      const krbn::device_identifiers identifiers({.is_keyboard = true});
      configuration->get_selected_profile().get_device(identifiers)->set_ignore(true);
      store.update(configuration);
      auto completed = [&](const auto& result) {
        responses.push_back(result);
      };
      store.async_execute({
                              {"action", "prepend_rule"},
                              {"code_type", "json"},
                              {"code", "invalid"},
                          },
                          completed);
      store.async_execute({
                              {"action", "erase_disconnected_devices"},
                          },
                          completed);
      store.stop();
      done.set_value();
    });
    expect(done.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    expect(notifications.front().at("not_connected_configured_devices_count") == 1);
    expect(responses.at(0).contains("error"));
    expect(responses.at(1).at("not_connected_configured_devices_count") == 0);
    expect(responses.at(0).at("snapshot").at("selected_profile").at("complex_modifications").at("rules").empty());
  };

  "failed_save_reports_error_and_is_retried_on_stop"_test = [] {
    int attempts = 0;
    std::vector<nlohmann::json> responses;
    settings_configuration_store store({
        .updated = [](const auto&) {},
        .save =
            [&](auto&) {
              if (++attempts == 1) {
                throw std::runtime_error("test save failure");
              }
            },
    });
    std::promise<void> done;
    store.enqueue_to_dispatcher([&] {
      store.update(std::make_shared<krbn::core_configuration::core_configuration>());
      auto completed = [&](const auto& result) {
        responses.push_back(result);
      };
      store.async_execute({
                              {"action", "rename_profile"},
                              {"index", 0},
                              {"name", "Edited"},
                          },
                          completed);
      store.async_execute({
                              {"action", "sync_save"},
                          },
                          completed);
      store.stop();
      done.set_value();
    });
    expect(done.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    expect(attempts == 2_i);
    expect(responses.at(1).at("error") == "test save failure");
  };

  "load_error_blocks_pending_explicit_and_shutdown_saves"_test = [] {
    int saves = 0;
    settings_configuration_store store({
        .updated = [](const auto&) {},
        .save = [&](auto&) { ++saves; },
    });
    std::promise<void> done;
    store.enqueue_to_dispatcher([&] {
      store.update(std::make_shared<krbn::core_configuration::core_configuration>());
      store.async_execute(
          {
              {"action", "rename_profile"},
              {"index", 0},
              {"name", "Edited"},
          },
          [&](const auto&) {
            // An external parse failure arrives during the save debounce.
            store.suspend_configuration();
            store.enqueue_to_dispatcher(
                [&] {
                  // Let the 200 ms debounce expire before checking explicit and shutdown saves.
                  expect(saves == 0_i);
                  store.async_execute(
                      {
                          {"action", "sync_save"},
                      },
                      [&](const auto& result) {
                        expect(result.contains("error"));
                        expect(!result.contains("snapshot"));
                        store.stop();
                        expect(saves == 0_i);
                        done.set_value();
                      });
                },
                store.when_now() + std::chrono::milliseconds(300));
          });
    });
    expect(done.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
  };

  "load_recovery_allows_new_edits_to_be_saved"_test = [] {
    int saves = 0;
    settings_configuration_store store({
        .updated = [](const auto&) {},
        .save = [&](auto&) { ++saves; },
    });
    std::promise<void> done;
    store.enqueue_to_dispatcher([&] {
      store.update(std::make_shared<krbn::core_configuration::core_configuration>());
      store.suspend_configuration();
      store.update(std::make_shared<krbn::core_configuration::core_configuration>());
      store.async_execute(
          {
              {"action", "rename_profile"},
              {"index", 0},
              {"name", "Recovered"},
          },
          [&](const auto& result) {
            expect(!result.contains("error"));
            store.stop();
            expect(saves == 1_i);
            done.set_value();
          });
    });
    expect(done.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
  };

  "asset_commands_use_the_path_from_before_a_reload"_test = [] {
    const auto directory = krbn::constants::get_user_complex_modifications_assets_directory();
    std::filesystem::create_directories(directory);
    const auto first = directory / "first.json";
    const auto target = directory / "target.json";
    const auto other = directory / "other.json";
    std::ofstream(first) << R"({"title":"A","rules":[]})";
    std::ofstream(target) << R"({
      "title": "B",
      "rules": [
        {
          "description": "Target rule",
          "manipulators": [
            {
              "type": "basic",
              "from": { "key_code": "a" },
              "to": [{ "key_code": "b" }]
            }
          ]
        }
      ]
    })";
    std::ofstream(other) << R"({"title":"C","rules":[]})";

    settings_configuration_store store({
        .updated = [](const auto&) {},
        .save = [](auto&) {},
    });
    auto execute = [&](const nlohmann::json& command) {
      std::promise<nlohmann::json> completed;
      auto result = completed.get_future();
      store.async_execute(command,
                          [&](const auto& value) {
                            completed.set_value(value);
                          });
      return result.get();
    };
    std::promise<void> initialized;
    store.enqueue_to_dispatcher([&] {
      store.update(std::make_shared<krbn::core_configuration::core_configuration>());
      initialized.set_value();
    });
    initialized.get_future().get();
    auto files = execute({
                             {"action", "reload_assets"},
                         })
                     .at("files");
    auto target_index = [&](const auto& values) {
      for (size_t i = 0; i < values.size(); ++i) {
        if (values[i].at("file_path") == target.string()) {
          return i;
        }
      }
      throw std::runtime_error("Target asset was not loaded");
    };
    const auto original_index = target_index(files);
    const auto requested_path = files.at(original_index).at("file_path");

    // Removing an earlier entry changes the indices held by the dispatcher.
    std::filesystem::remove(first);
    files = execute({{"action", "reload_assets"}}).at("files");
    expect(target_index(files) < original_index);
    const auto added = execute({
        {"action", "add_rules"},
        {"rules", {{
                      {"file_path", requested_path},
                      {"index", 0},
                  }}},
    });
    expect(!added.contains("error"));
    expect(added.at("snapshot").at("selected_profile").at("complex_modifications").at("rules").at(0).at("description") == "Target rule");
    const auto result = execute({
        {"action", "erase_asset"},
        {"file_path", requested_path},
    });
    expect(!result.contains("error"));
    expect(!std::filesystem::exists(target));
    expect(std::filesystem::exists(other));

    const auto missing = execute({
        {"action", "add_rules"},
        {"rules", {{
                      {"file_path", requested_path},
                      {"index", 0},
                  }}},
    });
    expect(missing.contains("error"));

    // A stale deletion request must not fall back to another asset.
    execute({
        {"action", "erase_asset"},
        {"file_path", requested_path},
    });
    expect(std::filesystem::exists(other));
    store.stop();
  };

  std::filesystem::remove_all(temporary_directory);
  return 0;
}
