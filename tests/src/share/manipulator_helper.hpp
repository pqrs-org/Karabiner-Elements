#pragma once

#include "event_queue.hpp"
#include "json_utility.hpp"
#include "manipulator/condition_factory.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp"
#include <boost/ut.hpp>
#include <pqrs/json.hpp>
#include <pqrs/thread_wait.hpp>

namespace krbn {
namespace unit_testing {
using namespace boost::ut;
using namespace boost::ut::literals;

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
    logger::get_logger()->info("krbn::unit_testing::manipulator_helper::run_tests");

    for (const auto& test : json) {
      logger::get_logger()->info("{0}", test["description"].get<std::string>());

      pseudo_time_source_->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));

      auto core_configuration = std::make_shared<krbn::core_configuration::core_configuration>();
      auto console_user_server_client = std::make_shared<krbn::console_user_server_client>();
      auto notification_message_manager = std::make_shared<krbn::notification_message_manager>("tmp/notification_message.json");
      auto connector = std::make_shared<manipulator::manipulator_managers_connector>();
      auto manipulator_managers = std::make_shared<std::vector<std::shared_ptr<manipulator::manipulator_manager>>>();
      auto event_queues = std::make_shared<std::vector<std::shared_ptr<event_queue::queue>>>();
      std::shared_ptr<krbn::manipulator::manipulators::post_event_to_virtual_devices::post_event_to_virtual_devices> post_event_to_virtual_devices_manipulator;

      // Build manipulators

      for (const auto& rule : test["rules"]) {
        manipulator_managers->push_back(std::make_shared<manipulator::manipulator_manager>());

        {
          std::ifstream ifs(rule.get<std::string>());
          expect(static_cast<bool>(ifs));
          for (const auto& j : json_utility::parse_jsonc(ifs)) {
            auto parameters = std::make_shared<core_configuration::details::complex_modifications_parameters>();
            auto m = manipulator::manipulator_factory::make_manipulator(j,
                                                                        parameters);

            if (auto conditions = pqrs::json::find_array(j, "conditions")) {
              for (const auto& c : conditions->value()) {
                m->push_back_condition(krbn::manipulator::condition_factory::make_condition(c));
              }
            }

            manipulator_managers->back()->push_back_manipulator(m);
          }
        }

        if (event_queues->empty()) {
          event_queues->push_back(std::make_shared<event_queue::queue>(fmt::format("queue_{0}", event_queues->size())));
          event_queues->push_back(std::make_shared<event_queue::queue>(fmt::format("complex_modifications_applied_event_queue_{0}", event_queues->size())));
          connector->emplace_back_connection(manipulator_managers->back(),
                                             (*event_queues)[0],
                                             (*event_queues)[1]);
        } else {
          event_queues->push_back(std::make_shared<event_queue::queue>(fmt::format("complex_modifications_applied_event_queue_{0}", event_queues->size())));
          connector->emplace_back_connection(manipulator_managers->back(),
                                             event_queues->back());
        }
      }

      //
      // fn function keys
      //

      {
        manipulator_managers->push_back(std::make_unique<manipulator::manipulator_manager>());

        //
        // f10 -> mute
        //

        auto from_json = nlohmann::json::object({
            {"key_code", "f10"},
            {"modifiers", nlohmann::json::object({
                              {"optional", nlohmann::json::array({"any"})},
                          })},
        });

        auto to_json = nlohmann::json::object({
            {"consumer_key_code", "mute"},
        });
        std::vector<gsl::not_null<std::shared_ptr<manipulator::to_event_definition>>> to_event_definitions;
        to_event_definitions.push_back(std::make_shared<manipulator::to_event_definition>(to_json));

        auto m = std::make_shared<manipulator::manipulators::basic::basic>(manipulator::manipulators::basic::from_event_definition(from_json),
                                                                           to_event_definitions);
        m->push_back_condition(manipulator::condition_factory::make_event_changed_if_condition(false));

        manipulator_managers->back()->push_back_manipulator(m);

        if (event_queues->empty()) {
          event_queues->push_back(std::make_shared<event_queue::queue>(fmt::format("queue_{0}", event_queues->size())));
          event_queues->push_back(std::make_shared<event_queue::queue>(fmt::format("fn_function_keys_applied_event_queue", event_queues->size())));
          connector->emplace_back_connection(manipulator_managers->back(),
                                             (*event_queues)[0],
                                             (*event_queues)[1]);
        } else {
          event_queues->push_back(std::make_shared<event_queue::queue>(fmt::format("fn_function_keys_applied_event_queue", event_queues->size())));
          connector->emplace_back_connection(manipulator_managers->back(),
                                             event_queues->back());
        }
      }

      if (pqrs::json::find<std::string>(test, "expected_post_event_to_virtual_devices_queue")) {
        post_event_to_virtual_devices_manipulator =
            std::make_shared<krbn::manipulator::manipulators::post_event_to_virtual_devices::post_event_to_virtual_devices>(
                console_user_server_client,
                notification_message_manager);

        manipulator_managers->push_back(std::make_unique<manipulator::manipulator_manager>());
        manipulator_managers->back()->push_back_manipulator(post_event_to_virtual_devices_manipulator);

        event_queues->push_back(std::make_shared<event_queue::queue>(fmt::format("posted_event_queue", event_queues->size())));
        connector->emplace_back_connection(manipulator_managers->back(),
                                           event_queues->back());
      }

      expect(!event_queues->empty());

      // Run manipulators

      auto input_event_arrived_connection = krbn_notification_center::get_instance().input_event_arrived.connect([this, core_configuration, connector] {
        connector->manipulate(now_,
                              core_configuration);
      });

      bool pause_manipulation = false;

      std::ifstream ifs(test["input_event_queue"].get<std::string>());
      expect(static_cast<bool>(ifs));
      for (const auto& j : json_utility::parse_jsonc(ifs)) {
        enqueue_to_dispatcher([this, core_configuration, connector, event_queues, j, &pause_manipulation] {
          if (auto s = pqrs::json::find<std::string>(j, "action")) {
            if (*s == "invalidate_manipulators") {
              connector->invalidate_manipulators();
            } else if (*s == "invoke_dispatcher") {
              if (auto t = pqrs::json::find<uint64_t>(j, "time_stamp")) {
                advance_now(std::chrono::milliseconds(*t));
              }
            } else if (*s == "manipulate") {
              absolute_time_point time_stamp(0);
              if (auto t = pqrs::json::find<uint64_t>(j, "time_stamp")) {
                auto ms = std::chrono::milliseconds(*t);
                time_stamp = absolute_time_point(0) +
                             pqrs::osx::chrono::make_absolute_time_duration(ms);
                advance_now(ms);
              }

              connector->manipulate(time_stamp,
                                    core_configuration);
            }

          } else if (auto v = pqrs::json::find<bool>(j, "pause_manipulation")) {
            pause_manipulation = *v;
            if (!pause_manipulation) {
              if (core_configuration->get_global_configuration().get_reorder_same_timestamp_input_events_to_prioritize_modifiers()) {
                event_queues->front()->sort_events();
              }
              connector->manipulate(now_,
                                    core_configuration);
            }

          } else if (auto v = pqrs::json::find<bool>(j, "reorder_same_timestamp_input_events_to_prioritize_modifiers")) {
            core_configuration->get_global_configuration().set_reorder_same_timestamp_input_events_to_prioritize_modifiers(*v);

          } else {
            auto e = event_queue::entry::make_from_json(j);

            event_queues->front()->push_back_entry(e);

            advance_now(pqrs::osx::chrono::make_milliseconds(e.get_event_time_stamp().get_time_stamp() - absolute_time_point(0)));

            if (!pause_manipulation) {
              if (core_configuration->get_global_configuration().get_reorder_same_timestamp_input_events_to_prioritize_modifiers()) {
                event_queues->front()->sort_events();
              }
              connector->manipulate(now_,
                                    core_configuration);
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

        if (auto s = pqrs::json::find<std::string>(j, "action")) {
          if (*s == "invoke_dispatcher") {
            if (auto t = pqrs::json::find<uint64_t>(j, "time_stamp")) {
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

      if (auto s = pqrs::json::find<std::string>(test, "expected_event_queue")) {
        if (overwrite_expected_results) {
          std::ofstream ofs(*s);
          expect(static_cast<bool>(ofs));
          ofs << nlohmann::json(event_queues->back()->get_entries()).dump(4) << std::endl;
        }

        std::ifstream ifs(*s);
        expect(static_cast<bool>(ifs));
        auto expected = json_utility::parse_jsonc(ifs);

        expect(event_queues->front()->get_entries().empty());
        expect(nlohmann::json(event_queues->back()->get_entries()).dump() == expected.dump());

      } else if (auto s = pqrs::json::find<std::string>(test, "expected_post_event_to_virtual_devices_queue")) {
        if (overwrite_expected_results) {
          std::ofstream ofs(*s);
          expect(static_cast<bool>(ofs));
          ofs << nlohmann::json(post_event_to_virtual_devices_manipulator->get_queue().get_events()).dump(4) << std::endl;
        }

        std::ifstream ifs(*s);
        expect(static_cast<bool>(ifs));
        auto expected = json_utility::parse_jsonc(ifs);

        expect(post_event_to_virtual_devices_manipulator.get());
        expect(nlohmann::json(post_event_to_virtual_devices_manipulator->get_queue().get_events()).dump() == expected.dump());

      } else {
        logger::get_logger()->error("There are not expected results.");
        expect(false);
      }

      input_event_arrived_connection.disconnect();

      manipulator_managers->clear();
      post_event_to_virtual_devices_manipulator = nullptr;
    }

    logger::get_logger()->info("krbn::unit_testing::manipulator_helper::run_tests finished");
  }

private:
  void advance_now(std::chrono::milliseconds ms) {
    if (pqrs::dispatcher::time_point(ms) > pseudo_time_source_->now()) {
      pseudo_time_source_->set_now(pqrs::dispatcher::time_point(ms));
      now_ = absolute_time_point(0) + pqrs::osx::chrono::make_absolute_time_duration(ms);
    }
  }

  std::weak_ptr<pqrs::dispatcher::time_source> original_weak_time_source_;
  std::shared_ptr<pqrs::dispatcher::pseudo_time_source> pseudo_time_source_;
  absolute_time_point now_;
};
} // namespace unit_testing
} // namespace krbn
