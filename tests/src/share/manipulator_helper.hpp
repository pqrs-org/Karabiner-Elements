#pragma once

#include "event_queue.hpp"
#include "json_utility.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "manipulator/manipulator_timer.hpp"

namespace krbn {
namespace unit_testing {
class manipulator_helper final {
public:
  static void run_tests(const nlohmann::json& json,
                        bool overwrite_expected_results = false) {
    logger::get_logger().info("krbn::unit_testing::manipulator_helper::run_tests");

    for (const auto& test : json) {
      logger::get_logger().info("{0}", test["description"].get<std::string>());

      system_preferences::values system_preferences_values;
      manipulator::manipulator_managers_connector connector;
      std::vector<std::unique_ptr<manipulator::manipulator_manager>> manipulator_managers;
      std::vector<std::shared_ptr<event_queue>> event_queues;
      std::shared_ptr<krbn::manipulator::details::post_event_to_virtual_devices> post_event_to_virtual_devices_manipulator;

      core_configuration::profile::complex_modifications::parameters parameters;
      for (const auto& rule : test["rules"]) {
        manipulator_managers.push_back(std::make_unique<manipulator::manipulator_manager>());

        {
          std::ifstream ifs(rule.get<std::string>());
          REQUIRE(ifs);
          for (const auto& j : nlohmann::json::parse(ifs)) {
            auto m = manipulator::manipulator_factory::make_manipulator(j, parameters);

            if (auto conditions = json_utility::find_array(j, "conditions")) {
              for (const auto& c : *conditions) {
                m->push_back_condition(krbn::manipulator::manipulator_factory::make_condition(c));
              }
            }

            manipulator_managers.back()->push_back_manipulator(m);
          }
        }

        if (event_queues.empty()) {
          event_queues.push_back(std::make_shared<event_queue>());
          event_queues.push_back(std::make_shared<event_queue>());
          connector.emplace_back_connection(*(manipulator_managers.back()),
                                            event_queues[0],
                                            event_queues[1]);
        } else {
          event_queues.push_back(std::make_shared<event_queue>());
          connector.emplace_back_connection(*(manipulator_managers.back()),
                                            event_queues.back());
        }
      }

      if (json_utility::find_optional<std::string>(test, "expected_post_event_to_virtual_devices_queue")) {
        post_event_to_virtual_devices_manipulator = std::make_shared<krbn::manipulator::details::post_event_to_virtual_devices>(system_preferences_values);

        manipulator_managers.push_back(std::make_unique<manipulator::manipulator_manager>());
        manipulator_managers.back()->push_back_manipulator(post_event_to_virtual_devices_manipulator);

        event_queues.push_back(std::make_shared<event_queue>());
        connector.emplace_back_connection(*(manipulator_managers.back()),
                                          event_queues.back());
      }

      REQUIRE(!manipulator_managers.empty());
      REQUIRE(!event_queues.empty());

      uint64_t now = 0;

      auto input_event_arrived_connection = krbn_notification_center::get_instance().input_event_arrived.connect([&]() {
        connector.manipulate(now);
      });

      {
        bool pause_manipulation = false;

        std::ifstream ifs(test["input_event_queue"].get<std::string>());
        REQUIRE(ifs);
        for (const auto& j : nlohmann::json::parse(ifs)) {
          if (auto s = json_utility::find_optional<std::string>(j, "action")) {
            if (*s == "invalidate_manipulators") {
              connector.invalidate_manipulators();
            } else if (*s == "invoke_manipulator_timer") {
              uint64_t time_stamp = 0;
              if (auto t = json_utility::find_optional<uint64_t>(j, "time_stamp")) {
                time_stamp = *t;
              }
              krbn::manipulator::manipulator_timer::get_instance().signal(time_stamp);
            }

          } else if (auto v = json_utility::find_optional<bool>(j, "pause_manipulation")) {
            pause_manipulation = *v;
            if (!pause_manipulation) {
              connector.manipulate(now);
            }

          } else {
            auto e = event_queue::queued_event(j);
            now = e.get_event_time_stamp().get_time_stamp();
            event_queues.front()->push_back_event(e);
            if (!pause_manipulation) {
              connector.manipulate(now);
            }
          }
        }
      }

      if (auto s = json_utility::find_optional<std::string>(test, "expected_event_queue")) {
        if (overwrite_expected_results) {
          std::ofstream ofs(*s);
          REQUIRE(ofs);
          ofs << nlohmann::json(event_queues.back()->get_events()).dump(4) << std::endl;
        }

        std::ifstream ifs(*s);
        REQUIRE(ifs);
        auto expected = nlohmann::json::parse(ifs);

        REQUIRE(event_queues.front()->get_events().empty());
        REQUIRE(nlohmann::json(event_queues.back()->get_events()).dump() == expected.dump());

      } else if (auto s = json_utility::find_optional<std::string>(test, "expected_post_event_to_virtual_devices_queue")) {
        if (overwrite_expected_results) {
          std::ofstream ofs(*s);
          REQUIRE(ofs);
          ofs << nlohmann::json(post_event_to_virtual_devices_manipulator->get_queue().get_events()).dump(4) << std::endl;
        }

        std::ifstream ifs(*s);
        REQUIRE(ifs);
        auto expected = nlohmann::json::parse(ifs);

        REQUIRE(post_event_to_virtual_devices_manipulator);
        REQUIRE(event_queues.front()->get_events().empty());
        REQUIRE(nlohmann::json(post_event_to_virtual_devices_manipulator->get_queue().get_events()).dump() == expected.dump());

      } else {
        logger::get_logger().error("There are not expected results.");
        REQUIRE(false);
      }

      input_event_arrived_connection.disconnect();
    }

    logger::get_logger().info("krbn::unit_testing::manipulator_helper::run_tests finished");
  }
};
} // namespace unit_testing
} // namespace krbn
