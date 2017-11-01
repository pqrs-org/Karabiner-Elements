#pragma once

#include "event_queue.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "manipulator/manipulator_timer.hpp"

namespace krbn {
namespace unit_testing {
class manipulator_helper final {
public:
  static void run_tests(const nlohmann::json& json) {
    logger::get_logger().info("krbn::unit_testing::manipulator_helper::run_tests");

    for (const auto& test : json) {
      logger::get_logger().info("{0}", test["description"].get<std::string>());

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
            manipulator_managers.back()->push_back_manipulator(manipulator::manipulator_factory::make_manipulator(j, parameters));
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

      if (test.find("expected_post_event_to_virtual_devices_queue") != std::end(test)) {
        post_event_to_virtual_devices_manipulator = std::make_shared<krbn::manipulator::details::post_event_to_virtual_devices>();

        manipulator_managers.push_back(std::make_unique<manipulator::manipulator_manager>());
        manipulator_managers.back()->push_back_manipulator(post_event_to_virtual_devices_manipulator);

        event_queues.push_back(std::make_shared<event_queue>());
        connector.emplace_back_connection(*(manipulator_managers.back()),
                                          event_queues.back());
      }

      REQUIRE(!manipulator_managers.empty());
      REQUIRE(!event_queues.empty());

      auto input_event_arrived_connection = krbn_notification_center::get_instance().input_event_arrived.connect([&]() {
          connector.manipulate();
        });

      {
        std::ifstream ifs(test["input_event_queue"].get<std::string>());
        REQUIRE(ifs);
        for (const auto& j : nlohmann::json::parse(ifs)) {
          auto action_it = j.find("action");
          if (action_it == std::end(j)) {
            auto e = event_queue::queued_event(j);
            event_queues.front()->push_back_event(e);
            connector.manipulate();
          } else {
            auto s = action_it->get<std::string>();
            if (s == "invalidate_manipulators") {
              connector.invalidate_manipulators();
            } else if (s == "invoke_manipulator_timer") {
              uint64_t time_stamp = 0;
              if (j.find("time_stamp") != std::end(j)) {
                time_stamp = j["time_stamp"];
              }
              krbn::manipulator::manipulator_timer::get_instance().signal(time_stamp);
            }
          }
        }
      }

      if (test.find("expected_event_queue") != std::end(test)) {
        std::ifstream ifs(test["expected_event_queue"].get<std::string>());
        REQUIRE(ifs);
        auto expected = nlohmann::json::parse(ifs);

        REQUIRE(event_queues.front()->get_events().empty());
        REQUIRE(nlohmann::json(event_queues.back()->get_events()).dump() == expected.dump());

      } else if (test.find("expected_post_event_to_virtual_devices_queue") != std::end(test)) {
        std::ifstream ifs(test["expected_post_event_to_virtual_devices_queue"].get<std::string>());
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
