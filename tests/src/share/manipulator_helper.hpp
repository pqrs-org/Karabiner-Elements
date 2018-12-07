#pragma once

#include "event_queue.hpp"
#include "json_utility.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include <pqrs/thread_wait.hpp>

namespace krbn {
namespace unit_testing {
class manipulator_helper final : pqrs::dispatcher::extra::dispatcher_client {
public:
  manipulator_helper(void) : dispatcher_client() {
    pseudo_time_source_ = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

    if (auto d = weak_dispatcher_.lock()) {
      original_weak_time_source_ = d->lock_weak_time_source();
      d->set_weak_time_source(pseudo_time_source_);
    }
  }

  virtual ~manipulator_helper(void) {
    detach_from_dispatcher([this] {
      if (auto d = weak_dispatcher_.lock()) {
        d->set_weak_time_source(original_weak_time_source_);
      }
    });
  }

  void run_tests(const nlohmann::json& json,
                 bool overwrite_expected_results = false) {
    logger::get_logger().info("krbn::unit_testing::manipulator_helper::run_tests");

    for (const auto& test : json) {
      logger::get_logger().info("{0}", test["description"].get<std::string>());

      pseudo_time_source_->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));

      system_preferences system_preferences;
      auto console_user_server_client = std::make_shared<krbn::console_user_server_client>();
      auto connector = std::make_shared<manipulator::manipulator_managers_connector>();
      auto manipulator_managers = std::make_shared<std::vector<std::shared_ptr<manipulator::manipulator_manager>>>();
      auto event_queues = std::make_shared<std::vector<std::shared_ptr<event_queue::queue>>>();
      std::shared_ptr<krbn::manipulator::details::post_event_to_virtual_devices> post_event_to_virtual_devices_manipulator;

      // Build manipulators

      for (const auto& rule : test["rules"]) {
        manipulator_managers->push_back(std::make_shared<manipulator::manipulator_manager>());

        {
          std::ifstream ifs(rule.get<std::string>());
          REQUIRE(ifs);
          for (const auto& j : nlohmann::json::parse(ifs)) {
            core_configuration::details::complex_modifications_parameters parameters;
            auto m = manipulator::manipulator_factory::make_manipulator(j,
                                                                        parameters);

            if (auto conditions = json_utility::find_array(j, "conditions")) {
              for (const auto& c : *conditions) {
                m->push_back_condition(krbn::manipulator::manipulator_factory::make_condition(c));
              }
            }

            manipulator_managers->back()->push_back_manipulator(m);
          }
        }

        if (event_queues->empty()) {
          event_queues->push_back(std::make_shared<event_queue::queue>());
          event_queues->push_back(std::make_shared<event_queue::queue>());
          connector->emplace_back_connection(manipulator_managers->back(),
                                             (*event_queues)[0],
                                             (*event_queues)[1]);
        } else {
          event_queues->push_back(std::make_shared<event_queue::queue>());
          connector->emplace_back_connection(manipulator_managers->back(),
                                             event_queues->back());
        }
      }

      if (json_utility::find_optional<std::string>(test, "expected_post_event_to_virtual_devices_queue")) {
        post_event_to_virtual_devices_manipulator = std::make_shared<krbn::manipulator::details::post_event_to_virtual_devices>(system_preferences,
                                                                                                                                console_user_server_client);

        manipulator_managers->push_back(std::make_unique<manipulator::manipulator_manager>());
        manipulator_managers->back()->push_back_manipulator(post_event_to_virtual_devices_manipulator);

        event_queues->push_back(std::make_shared<event_queue::queue>());
        connector->emplace_back_connection(manipulator_managers->back(),
                                           event_queues->back());
      }

      REQUIRE(!manipulator_managers->empty());
      REQUIRE(!event_queues->empty());

      // Run manipulators

      auto input_event_arrived_connection = krbn_notification_center::get_instance().input_event_arrived.connect([=] {
        connector->manipulate(now_);
      });

      bool pause_manipulation = false;

      std::ifstream ifs(test["input_event_queue"].get<std::string>());
      REQUIRE(ifs);
      for (const auto& j : nlohmann::json::parse(ifs)) {
        enqueue_to_dispatcher([=, &pause_manipulation] {
          if (auto s = json_utility::find_optional<std::string>(j, "action")) {
            if (*s == "invalidate_manipulators") {
              connector->invalidate_manipulators();
            } else if (*s == "invoke_dispatcher") {
              if (auto t = json_utility::find_optional<uint64_t>(j, "time_stamp")) {
                advance_now(std::chrono::milliseconds(*t));
              }
            } else if (*s == "manipulate") {
              absolute_time_point time_stamp(0);
              if (auto t = json_utility::find_optional<uint64_t>(j, "time_stamp")) {
                auto ms = std::chrono::milliseconds(*t);
                time_stamp = absolute_time_point(0) +
                             time_utility::to_absolute_time_duration(ms);
                advance_now(ms);
              }

              connector->manipulate(time_stamp);
            }

          } else if (auto v = json_utility::find_optional<bool>(j, "pause_manipulation")) {
            pause_manipulation = *v;
            if (!pause_manipulation) {
              connector->manipulate(now_);
            }

          } else {
            auto e = event_queue::entry::make_from_json(j);

            event_queues->front()->push_back_entry(e);

            advance_now(time_utility::to_milliseconds(e.get_event_time_stamp().get_time_stamp() - absolute_time_point(0)));

            if (!pause_manipulation) {
              connector->manipulate(now_);
            }
          }
        });

        // Wait immediate queues

        {
          auto wait = pqrs::make_thread_wait();
          enqueue_to_dispatcher([wait] {
            wait->notify();
          });
          wait->wait_notice();
        }

        if (auto s = json_utility::find_optional<std::string>(j, "action")) {
          if (*s == "invoke_dispatcher") {
            if (auto t = json_utility::find_optional<uint64_t>(j, "time_stamp")) {
              auto ms = std::chrono::milliseconds(*t);

              enqueue_to_dispatcher([this, ms] {
                advance_now(ms);
              });

              auto wait = pqrs::make_thread_wait();
              enqueue_to_dispatcher(
                  [wait] {
                    wait->notify();
                  },
                  pqrs::dispatcher::time_point(std::chrono::milliseconds(0)) + ms);
              wait->wait_notice();
            }
          }
        }
      }

      // Test the result

      if (auto s = json_utility::find_optional<std::string>(test, "expected_event_queue")) {
        if (overwrite_expected_results) {
          std::ofstream ofs(*s);
          REQUIRE(ofs);
          ofs << nlohmann::json(event_queues->back()->get_entries()).dump(4) << std::endl;
        }

        std::ifstream ifs(*s);
        REQUIRE(ifs);
        auto expected = nlohmann::json::parse(ifs);

        REQUIRE(event_queues->front()->get_entries().empty());
        REQUIRE(nlohmann::json(event_queues->back()->get_entries()).dump() == expected.dump());

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
        REQUIRE(nlohmann::json(post_event_to_virtual_devices_manipulator->get_queue().get_events()).dump() == expected.dump());

      } else {
        logger::get_logger().error("There are not expected results.");
        REQUIRE(false);
      }

      input_event_arrived_connection.disconnect();

      manipulator_managers->clear();
      post_event_to_virtual_devices_manipulator = nullptr;
    }

    logger::get_logger().info("krbn::unit_testing::manipulator_helper::run_tests finished");
  }

private:
  void advance_now(std::chrono::milliseconds ms) {
    if (pqrs::dispatcher::time_point(ms) > pseudo_time_source_->now()) {
      pseudo_time_source_->set_now(pqrs::dispatcher::time_point(ms));
      now_ = absolute_time_point(0) + time_utility::to_absolute_time_duration(ms);
    }
  }

  std::weak_ptr<pqrs::dispatcher::time_source> original_weak_time_source_;
  std::shared_ptr<pqrs::dispatcher::pseudo_time_source> pseudo_time_source_;
  absolute_time_point now_;
};
} // namespace unit_testing
} // namespace krbn
