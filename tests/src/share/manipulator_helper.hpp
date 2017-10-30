#pragma once

#include "event_queue.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "manipulator/manipulator_timer.hpp"

namespace krbn {
namespace unit_testing {
class manipulator_helper final {
public:
  static void run_tests(const nlohmann::json& json) {
    for (const auto& test : json) {
      logger::get_logger().info(test["description"].get<std::string>());

      manipulator::manipulator_managers_connector connector;
      std::vector<std::unique_ptr<manipulator::manipulator_manager>> manipulator_managers;
      std::vector<std::shared_ptr<event_queue>> event_queues;

      core_configuration::profile::complex_modifications::parameters parameters;
      for (const auto& rule : test["rules"]) {
        manipulator_managers.push_back(std::make_unique<manipulator::manipulator_manager>());

        for (const auto& j : nlohmann::json::parse(std::ifstream(rule.get<std::string>()))) {
          manipulator_managers.back()->push_back_manipulator(manipulator::manipulator_factory::make_manipulator(j, parameters));
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

      REQUIRE(!manipulator_managers.empty());
      REQUIRE(!event_queues.empty());

      push_back_events(*(event_queues.front()),
                       nlohmann::json::parse(std::ifstream(test["input_event_queue"].get<std::string>())));

      auto expected_event_queue = std::make_shared<event_queue>();
      push_back_events(*expected_event_queue,
                       nlohmann::json::parse(std::ifstream(test["expected_event_queue"].get<std::string>())));

      connector.manipulate();

      REQUIRE(event_queues.front()->get_events().empty());
      REQUIRE(event_queues.back()->get_events() == expected_event_queue->get_events());
    }
  }

  static void push_back_events(event_queue& event_queue,
                               const nlohmann::json& json) {
    for (const auto& j : json) {
      auto e = event_queue::queued_event(j);
      event_queue.push_back_event(e);
    }
  }
};
} // namespace unit_testing
} // namespace krbn
