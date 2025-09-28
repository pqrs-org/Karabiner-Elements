#pragma once

#include "core_configuration/core_configuration.hpp"
#include "json_utility.hpp"
#include "manipulator/condition_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulators/basic/basic.hpp"

namespace krbn {
namespace grabber {
namespace device_grabber_details {
class fn_function_keys_manipulator_manager final {
public:
  fn_function_keys_manipulator_manager(void) {
    manipulator_manager_ = std::make_shared<manipulator::manipulator_manager>();
  }

  std::shared_ptr<manipulator::manipulator_manager> get_manipulator_manager(void) const {
    return manipulator_manager_;
  }

  void update(const core_configuration::details::profile& profile,
              const pqrs::osx::system_preferences::properties& system_preferences_properties) {
    manipulator_manager_->invalidate_manipulators();

    bool from_mandatory_modifiers_fn = false;

    if (system_preferences_properties.get_use_fkeys_as_standard_function_keys()) {
      // Use all F1, F2, etc. keys as standard function keys: On
      //
      // f1 -> f1
      // fn+f1 -> display_brightness_decrement

      from_mandatory_modifiers_fn = true;

      // consumer_key_code::dictation does not work with modifier_flag::fn.
      // So, we send the plain media keys.
      // (e.g., display_brightness_decrement, not fn+display_brightness_decrement)

    } else {
      // Use all F1, F2, etc. keys as standard function keys: Off
      //
      // f1 -> display_brightness_decrement
      // fn+f1...fn+f12 -> f1...f12

      for (int i = 1; i <= 12; ++i) {
        auto from_json = nlohmann::json::object({
            {"key_code", fmt::format("f{0}", i)},
            {"modifiers", nlohmann::json::object({
                              {"mandatory", nlohmann::json::array({"fn"})},
                              {"optional", nlohmann::json::array({"any"})},
                          })},
        });

        auto to_json = nlohmann::json::object({
            {"key_code", fmt::format("f{0}", i)},
            {"modifiers", nlohmann::json::array({"fn"})},
        });

        std::vector<pqrs::not_null_shared_ptr_t<manipulator::to_event_definition>> to_event_definitions;
        to_event_definitions.push_back(std::make_shared<manipulator::to_event_definition>(to_json));

        try {
          auto manipulator = std::make_shared<manipulator::manipulators::basic::basic>(manipulator::manipulators::basic::from_event_definition(from_json),
                                                                                       to_event_definitions);
          manipulator_manager_->push_back_manipulator(std::shared_ptr<manipulator::manipulators::base>(manipulator));

        } catch (const pqrs::json::unmarshal_error& e) {
          logger::get_logger()->error(fmt::format("karabiner.json error: {0}", e.what()));

        } catch (const std::exception& e) {
          logger::get_logger()->error(e.what());
        }
      }
    }

    // from_modifiers+f1 -> display_brightness_decrement ...

    for (const auto& device : profile.get_devices()) {
      for (const auto& pair : device->get_fn_function_keys()->get_pairs()) {
        try {
          if (auto m = make_function_key_manipulator(pair,
                                                     from_mandatory_modifiers_fn)) {
            m->push_back_condition(manipulator::condition_factory::make_event_changed_if_condition(false));
            m->push_back_condition(manipulator::condition_factory::make_device_unless_touch_bar_condition());

            auto c = manipulator::condition_factory::make_device_if_condition(*device);
            m->push_back_condition(c);

            manipulator_manager_->push_back_manipulator(m);
          }

        } catch (const pqrs::json::unmarshal_error& e) {
          logger::get_logger()->error(fmt::format("karabiner.json error: {0}", e.what()));

        } catch (const std::exception& e) {
          logger::get_logger()->error(e.what());
        }
      }
    }

    for (const auto& pair : profile.get_fn_function_keys()->get_pairs()) {
      if (auto m = make_function_key_manipulator(pair,
                                                 from_mandatory_modifiers_fn)) {
        m->push_back_condition(manipulator::condition_factory::make_event_changed_if_condition(false));
        m->push_back_condition(manipulator::condition_factory::make_device_unless_touch_bar_condition());

        manipulator_manager_->push_back_manipulator(m);
      }
    }

    //
    // Change keys which macOS will ignore.
    //

    // Touch ID
    {
      // from: Touch ID ("consumer_key_code": "menu")
      // to:   Lock key on Magic Keyboard without Touch ID {"consumer_key_code": "al_terminal_lock_or_screensaver"}
      try {
        auto from_json = R"(
{
  "consumer_key_code": "menu",
  "modifiers": {
    "optional": ["any"]
  }
}
      )"_json;

        auto to_json = R"(
{
  "consumer_key_code": "al_terminal_lock_or_screensaver"
}
      )"_json;

        std::vector<pqrs::not_null_shared_ptr_t<manipulator::to_event_definition>> to_event_definitions;

        to_event_definitions.push_back(std::make_shared<manipulator::to_event_definition>(to_json));

        auto manipulator = std::make_shared<manipulator::manipulators::basic::basic>(manipulator::manipulators::basic::from_event_definition(from_json),
                                                                                     to_event_definitions);

        manipulator_manager_->push_back_manipulator(std::shared_ptr<manipulator::manipulators::base>(manipulator));
      } catch (const std::exception& e) {
        logger::get_logger()->error(e.what());
      }
    }

    // Application launch buttons
    {
      nlohmann::json data = nlohmann::json::array();

      // Typically al_consumer_control_configuration is used as the key to open the music player.
      // https://source.android.com/docs/core/interaction/input/keyboard-devices
      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_consumer_control_configuration"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Music.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_word_processor"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Pages.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_text_editor"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'TextEdit.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_spreadsheet"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Numbers.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_presentation_app"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Keynote.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_email_reader"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Mail.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_calculator"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Calculator.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_local_machine_browser"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Finder.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_internet_browser"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Safari.app'"}})},
      }));

      data.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"consumer_key_code", "al_dictionary"}})},
          {"to", nlohmann::json::object({{"shell_command", "open -a 'Dictionary.app'"}})},
      }));

      for (const auto& d : data) {
        auto from_json = d["from"];
        from_json["modifiers"]["mandatory"] = nlohmann::json::array({"any"});

        auto to_json = d["to"];

        std::vector<pqrs::not_null_shared_ptr_t<manipulator::to_event_definition>> to_event_definitions;
        to_event_definitions.push_back(std::make_shared<manipulator::to_event_definition>(to_json));

        try {
          auto manipulator = std::make_shared<manipulator::manipulators::basic::basic>(manipulator::manipulators::basic::from_event_definition(from_json),
                                                                                       to_event_definitions);
          manipulator_manager_->push_back_manipulator(std::shared_ptr<manipulator::manipulators::base>(manipulator));

        } catch (const pqrs::json::unmarshal_error& e) {
          logger::get_logger()->error(fmt::format("karabiner.json error: {0}", e.what()));

        } catch (const std::exception& e) {
          logger::get_logger()->error(e.what());
        }
      }
    }
  }

private:
  std::shared_ptr<manipulator::manipulators::base> make_function_key_manipulator(const std::pair<std::string, std::string>& pair,
                                                                                 bool from_mandatory_modifiers_fn) const {
    try {
      auto from_json = json_utility::parse_jsonc(pair.first);
      if (from_json.empty()) {
        return nullptr;
      }
      from_json["modifiers"]["mandatory"] = nlohmann::json::array();
      if (from_mandatory_modifiers_fn) {
        from_json["modifiers"]["mandatory"].push_back("fn");
      }

      from_json["modifiers"]["optional"] = nlohmann::json::array({"any"});

      auto to_json = json_utility::parse_jsonc(pair.second);
      if (to_json.empty()) {
        return nullptr;
      }

      std::vector<pqrs::not_null_shared_ptr_t<manipulator::to_event_definition>> to_event_definitions;
      for (auto&& j : to_json) {
        // Note:
        // Normal f1...f12 keys will be changed media control keys by the Apple keyboard driver of macOS.
        // Therefore, if f1...f12 is specified in `to` events, we should send these keys with/without fn.
        //
        // - When `from_mandatory_modifiers_fn` == true:
        //     - Use all F1, F2, etc. keys as standard function keys: On
        //     - In this case, f1 is interpreted as a regular f1 key.
        // - When `from_mandatory_modifiers_fn` == false:
        //     - Use all F1, F2, etc. keys as standard function keys: Off
        //     - In this case, fn+f1 is interpreted as a regular f1 key.

        if (j.contains("key_code")) {
          if (f1_f12_key(j["key_code"])) {
            if (!from_mandatory_modifiers_fn) {
              if (!j["modifiers"].contains("fn")) {
                j["modifiers"].push_back("fn");
              }
            }
          }
        }

        // When we change fn+f10 to mute, directly mapping fn+f10 to mute will result in the following events being sent:
        //
        // - input: fn keydown
        //     - output: fn keydown
        // - input: f10 keydown
        //     - output: fn keyup
        //     - output: mute keydown
        // - input: fn10 keyup
        //     - output: mute keyup
        // - input: fn keyup
        //     - output: none
        //
        // In this case, the fn key is unintentionally tapped.
        // To avoid this, fn+f10 should be remapped to fn+mute instead.
        //
        // Note:
        // However, if we want to send f10 as is, sending it as fn+f10 will result in macOS converting it to mute.
        // For f1–f12 specifically, fn should not be added.

        if (from_mandatory_modifiers_fn) {
          if (j.contains("key_code") &&
              f1_f12_key(j["key_code"])) {
            // Do not append fn modifier
          } else {
            if (!j["modifiers"].contains("fn")) {
              j["modifiers"].push_back("fn");
            }
          }
        }

        to_event_definitions.push_back(std::make_shared<manipulator::to_event_definition>(j));
      }

      return std::make_shared<manipulator::manipulators::basic::basic>(manipulator::manipulators::basic::from_event_definition(from_json),
                                                                       to_event_definitions);
    } catch (const pqrs::json::unmarshal_error& e) {
      logger::get_logger()->error(fmt::format("karabiner.json error: {0}", e.what()));
    } catch (const std::exception& e) {
      logger::get_logger()->error(e.what());
    }

    return nullptr;
  }

  bool f1_f12_key(const std::string key_code) const {
    return key_code == "f1" ||
           key_code == "f2" ||
           key_code == "f3" ||
           key_code == "f4" ||
           key_code == "f5" ||
           key_code == "f6" ||
           key_code == "f7" ||
           key_code == "f8" ||
           key_code == "f9" ||
           key_code == "f10" ||
           key_code == "f11" ||
           key_code == "f12";
  }

  std::shared_ptr<manipulator::manipulator_manager> manipulator_manager_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
