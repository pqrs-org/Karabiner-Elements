#include "../../../../src/apps/SettingsWindow/src/cpp/settings_configuration_snapshot.hpp"
#include <boost/ut.hpp>
#include <cstdlib>
#include <fstream>
#include <unistd.h>

namespace {
nlohmann::json make_summary(const krbn::core_configuration::core_configuration& configuration) {
  const auto snapshot = settings_configuration_snapshot(configuration).to_json();
  const auto prefix = std::filesystem::temp_directory_path() / ("karabiner-changed-settings-" + std::to_string(getpid()));
  const auto input = prefix.string() + "-input.json";
  const auto output = prefix.string() + "-output.json";
  {
    std::ofstream stream(input);
    stream << snapshot;
  }
  const auto command = std::string("\"") + CHANGED_SETTINGS_EXECUTABLE + "\" < \"" + input + "\" > \"" + output + "\"";
  const auto status = std::system(command.c_str());
  std::filesystem::remove(input);
  if (status != 0) {
    std::filesystem::remove(output);
    throw std::runtime_error("Swift ChangedSettings failed");
  }
  nlohmann::json summary;
  {
    std::ifstream stream(output);
    stream >> summary;
  }
  std::filesystem::remove(output);
  return summary;
}
} // namespace

int main() {
  using namespace boost::ut;

  "defaults_and_explicit_default_values_are_omitted"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    expect(make_summary(configuration).empty());
    configuration.get_global_configuration().set_enable_cgeventtap_fallback(false);
    expect(make_summary(configuration).empty());

    configuration.get_global_configuration().set_enable_cgeventtap_fallback(true);
    auto summary = make_summary(configuration);
    expect(summary == nlohmann::json({{"global", {{"enable_cgeventtap_fallback", true}}}}));
    configuration.get_global_configuration().set_ui_language("ja");
    auto snapshot = settings_configuration_snapshot(configuration).to_json();
    expect(snapshot.at("global_configuration").at("ui_language") == "ja");
    summary["global"]["ui_language"] = "ja";
    expect(make_summary(configuration) == summary);

    configuration.get_global_configuration().set_enable_cgeventtap_fallback(false);
    configuration.get_global_configuration().set_ui_language("auto");
    expect(make_summary(configuration).empty());
  };

  "only_selected_profile_and_current_machine_are_reported"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    configuration.get_machine_specific().get_entry(krbn::karabiner_machine_identifier("other-machine")).set_enable_multitouch_extension(true);
    expect(make_summary(configuration).empty());
    configuration.get_machine_specific().get_entry().set_enable_multitouch_extension(true);
    expect(make_summary(configuration).at("machine_specific") ==
           nlohmann::json({{"enable_multitouch_extension", true}}));
    configuration.get_machine_specific().get_entry().set_enable_multitouch_extension(false);
    configuration.get_selected_profile().set_ignore_pointing_device_events_by_default(false);
    configuration.push_back_profile();
    configuration.select_profile(1);
    expect(make_summary(configuration).empty());
    configuration.select_profile(0);
    expect(make_summary(configuration).at("selected_profile") ==
           nlohmann::json({{"ignore_pointing_device_events_by_default", false}}));
  };

  "mappings_rules_and_devices_are_summarized_without_mutating_configuration"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    krbn::core_configuration::details::profile profile(nlohmann::json::parse(R"({
      "name": "Custom profile",
      "simple_modifications": [{"from": {"key_code": "a"}, "to": [{"key_code": "b"}]}],
      "fn_function_keys": [{"from": {"key_code": "f1"}, "to": [{"key_code": "a"}]}],
      "complex_modifications": {
        "parameters": {"basic.to_if_alone_timeout_milliseconds": 777},
        "rules": [
          {"description": "enabled rule", "manipulators": [{"type": "basic", "from": {"key_code": "a"}, "to": [{"key_code": "b"}]}]},
          {"description": "disabled rule", "enabled": false, "manipulators": [{"type": "basic", "from": {"key_code": "c"}, "to": [{"key_code": "d"}]}]}
        ]
      },
      "devices": [{
        "identifiers": {"vendor_id": 123, "product_id": 456, "is_keyboard": true},
        "ignore": true,
        "simple_modifications": [{"from": {"key_code": "c"}, "to": [{"key_code": "d"}]}]
      }, {
        "identifiers": {"vendor_id": 123, "product_id": 457, "is_keyboard": true},
        "ignore": false
      }, {
        "identifiers": {"vendor_id": 123, "product_id": 458, "is_keyboard": true}
      }]
    })"),
                                                       krbn::core_configuration::error_handling::strict);
    configuration.duplicate_profile(profile);
    configuration.select_profile(1);
    // Initialize the current machine entry before checking read-only behavior.
    static_cast<void>(configuration.get_machine_specific().get_entry());
    auto before = configuration.to_json();
    auto summary = make_summary(configuration).at("selected_profile");
    expect(summary.at("simple_modifications_count") == 1);
    expect(!summary.contains("simple_modifications"));
    expect(summary.at("fn_function_keys_count") == 1);
    expect(!summary.contains("fn_function_keys"));
    expect(!summary.contains("name"));
    expect(!summary.contains("selected"));
    auto complex = summary.at("complex_modifications");
    expect(complex.at("rules_count") == 2);
    expect(complex.at("enabled_rules_count") == 1);
    expect(!complex.contains("rules"));
    expect(complex.at("parameters").at("basic.to_if_alone_timeout_milliseconds") == 777);
    // An explicit value equal to its default is not a changed snapshot value.
    expect(summary.at("configured_devices_count") == 1);
    expect(!summary.contains("devices"));
    expect(configuration.to_json() == before);
  };

  "device_defaults_follow_the_selected_profile"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    auto& profile = configuration.get_selected_profile();
    auto identifiers = krbn::device_identifiers(krbn::device_identifiers::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(123),
        .product_id = pqrs::hid::product_id::value_t(456),
        .is_pointing_device = true,
    });
    auto device = profile.get_device(identifiers);
    expect(make_summary(configuration).empty());
    profile.set_ignore_pointing_device_events_by_default(false);
    auto summary = make_summary(configuration).at("selected_profile");
    expect(!summary.contains("configured_devices_count"));
    device->set_ignore(true);
    summary = make_summary(configuration).at("selected_profile");
    expect(summary.at("configured_devices_count") == 1);
    expect(!summary.contains("devices"));
  };

  "unknown_configuration_keys_are_preserved_but_not_reported"_test = [] {
    auto json = nlohmann::json::parse(R"({
      "global": {"language": "ja", "future_option": true,
        "notification_window_colors": {"light": {"future_color": "red"}}},
      "profiles": [{"name": "Unknown keys", "selected": true, "future_option": true,
        "parameters": {"future_parameter": 10},
        "virtual_hid_keyboard": {"future_option": true},
        "complex_modifications": {"future_option": true, "parameters": {"future_parameter": 10}},
        "devices": [{"identifiers": {"vendor_id": 123, "product_id": 456, "is_keyboard": true},
          "future_device_option": true}]}]
    })");
    json["machine_specific"][type_safe::get(krbn::constants::get_karabiner_machine_identifier())] = {{"future_option", true}};
    const auto path = std::filesystem::temp_directory_path() /
                      ("karabiner_changed_settings_" + std::to_string(getpid()) + ".json");
    {
      std::ofstream output(path);
      output << json;
    }
    krbn::core_configuration::core_configuration configuration(path, geteuid(), krbn::core_configuration::error_handling::strict);
    std::filesystem::remove(path);
    expect(configuration.get_load_state() == krbn::core_configuration::core_configuration::load_state::loaded);
    const auto before = configuration.to_json();
    expect(make_summary(configuration).empty());
    expect(configuration.to_json() == before);
    expect(configuration.get_global_configuration().to_json().at("language") == "ja");
    configuration.get_global_configuration().set_ui_language("en");
    expect(make_summary(configuration) == nlohmann::json({{"global", {{"ui_language", "en"}}}}));
    expect(configuration.get_global_configuration().to_json().at("language") == "ja");
  };

  "nested_snapshot_differences_omit_unchanged_siblings"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    configuration.get_global_configuration().get_notification_window_colors().get_light().set_background_color("#123456ff");
    configuration.get_selected_profile().get_virtual_hid_keyboard()->set_mouse_key_xy_scale(200);
    configuration.get_selected_profile().get_complex_modifications()->get_parameters()->set_mouse_motion_to_scroll_speed(150);
    expect(make_summary(configuration) == nlohmann::json::parse(R"({
      "global": {"notification_window_colors": {"light": {"background_color": "#123456ff"}}},
      "selected_profile": {
        "virtual_hid_keyboard": {"mouse_key_xy_scale": 200},
        "complex_modifications": {"parameters": {"mouse_motion_to_scroll.speed": 150}}
      }
    })"));
  };

  "incomplete_mapping_rows_and_default_function_keys_are_not_counted"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    auto& profile = configuration.get_selected_profile();
    profile.get_simple_modifications()->push_back_pair();
    profile.get_fn_function_keys()->push_back_pair();
    expect(make_summary(configuration).empty());
    profile.get_fn_function_keys()->replace_second(R"({"key_code":"f1"})", R"([{"key_code":"a"}])");
    expect(make_summary(configuration) == nlohmann::json({{"selected_profile", {{"fn_function_keys_count", 1}}}}));
    profile.get_fn_function_keys()->replace_second(R"({"key_code":"f1"})", R"([{"consumer_key_code":"display_brightness_decrement"}])");
    expect(make_summary(configuration).empty());
  };

  "disabled_rules_keep_a_zero_enabled_count"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    krbn::core_configuration::details::profile profile(nlohmann::json::parse(R"({
      "complex_modifications": {"rules": [{"description": "disabled", "enabled": false,
        "manipulators": [{"type": "basic", "from": {"key_code": "a"}, "to": [{"key_code": "b"}]}]}]}
    })"),
                                                       krbn::core_configuration::error_handling::strict);
    configuration.duplicate_profile(profile);
    configuration.select_profile(1);
    expect(make_summary(configuration) == nlohmann::json::parse(R"({
      "selected_profile": {"complex_modifications": {"rules_count": 1, "enabled_rules_count": 0}}
    })"));
  };

  "remembered_apple_pointing_device_defaults_are_not_reported"_test = [] {
    krbn::connected_devices connected_devices;
    const auto properties = krbn::device_properties::make_device_properties(nlohmann::json::parse(R"({
      "device_id": 12345,
      "device_identifiers": {"vendor_id": 1452, "product_id": 456, "is_pointing_device": true}
    })"));
    connected_devices.push_back_device(properties);
    settings_remembered_device_properties::get_instance().remember_connected_devices(connected_devices);
    krbn::core_configuration::core_configuration configuration;
    expect(make_summary(configuration).empty());
    auto& profile = configuration.get_selected_profile();
    profile.set_ignore_pointing_device_events_by_default(false);
    auto summary = make_summary(configuration).at("selected_profile");
    expect(summary == nlohmann::json({{"ignore_pointing_device_events_by_default", false}}));
    profile.get_device(properties->get_device_identifiers())->set_ignore(false);
    summary = make_summary(configuration).at("selected_profile");
    expect(summary.at("configured_devices_count") == 1);
  };

  return 0;
}
