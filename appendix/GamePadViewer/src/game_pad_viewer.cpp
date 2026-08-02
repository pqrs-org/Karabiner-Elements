#include "game_pad_viewer.hpp"
#include "device_properties.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "logger.hpp"
#include "process_lifecycle_manager.hpp"
#include "run_loop_thread_utility.hpp"
#include <atomic>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>
#include <unordered_map>

namespace {
std::atomic<game_pad_viewer_hid_value_arrived_callback> hid_value_arrived_callback;
std::atomic<bool> hid_value_monitor_observed;

class hid_value_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  hid_value_monitor(const hid_value_monitor&) = delete;

  explicit hid_value_monitor(game_pad_viewer_hid_value_arrived_callback callback)
      : dispatcher_client(),
        callback_(callback) {
    hid_value_monitor_observed = false;

    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::joystick),
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::game_pad),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(
        pqrs::dispatcher::extra::get_shared_dispatcher(),
        pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
        matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (!device_ptr) {
        return;
      }

      auto device_id = krbn::make_device_id(registry_entry_id);
      auto device_properties = krbn::device_properties::make_device_properties(device_id,
                                                                               *device_ptr);
      auto monitor = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(
          pqrs::dispatcher::extra::get_shared_dispatcher(),
          pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
          *device_ptr);
      hid_queue_value_monitors_.insert_or_assign(device_id, monitor);

      monitor->started.connect([] {
        hid_value_monitor_observed = true;
      });

      monitor->values_arrived.connect([this, device_id, device_properties](auto&& values) {
        values_arrived(device_id,
                       device_properties,
                       values);
      });

      monitor->async_start(kIOHIDOptionsTypeNone,
                           std::chrono::milliseconds(3000));
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      hid_queue_value_monitors_.erase(krbn::make_device_id(registry_entry_id));
    });

    hid_manager_->error_occurred.connect([](auto&& message, auto&& kern_return) {
      krbn::logger::get_logger()->error("{0}: {1}", message, kern_return.to_string());
    });

    hid_manager_->async_start();
  }

  ~hid_value_monitor() {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
      hid_queue_value_monitors_.clear();
      hid_value_monitor_observed = false;
    });
  }

private:
  void values_arrived(krbn::device_id device_id,
                      pqrs::not_null_shared_ptr_t<krbn::device_properties> device_properties,
                      pqrs::not_null_shared_ptr_t<std::vector<pqrs::cf::cf_ptr<IOHIDValueRef>>> values) {
    if (!callback_) {
      return;
    }

    for (const auto& value : *values) {
      auto v = pqrs::osx::iokit_hid_value(*value);
      auto usage_page = v.get_usage_page();
      auto usage = v.get_usage();
      auto logical_max = v.get_logical_max();
      auto logical_min = v.get_logical_min();

      if (usage_page && usage && logical_max && logical_min) {
        callback_(type_safe::get(device_id),
                  device_properties->get_device_identifiers().get_is_keyboard(),
                  device_properties->get_device_identifiers().get_is_pointing_device(),
                  device_properties->get_device_identifiers().get_is_game_pad(),
                  type_safe::get(*usage_page),
                  type_safe::get(*usage),
                  *logical_max,
                  *logical_min,
                  v.get_integer_value());
      }
    }
  }

  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<krbn::device_id,
                     pqrs::not_null_shared_ptr_t<pqrs::osx::iokit_hid_queue_value_monitor>>
      hid_queue_value_monitors_;
  game_pad_viewer_hid_value_arrived_callback callback_;
};

class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager()
      : dispatcher_client() {
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      hid_value_monitor_ = nullptr;
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      if (auto callback = hid_value_arrived_callback.load()) {
        hid_value_monitor_ = std::make_unique<hid_value_monitor>(callback);
      }
    });
  }

private:
  std::unique_ptr<hid_value_monitor> hid_value_monitor_;
};

std::shared_ptr<krbn::dispatcher_utility::scoped_dispatcher_manager> scoped_dispatcher_manager;
std::shared_ptr<krbn::run_loop_thread_utility::scoped_run_loop_thread_manager> scoped_run_loop_thread_manager;
} // namespace

void game_pad_viewer_initialize(game_pad_viewer_hid_value_arrived_callback callback) {
  scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();
  krbn::environment_variable_utility::log(environment_variables);

  hid_value_arrived_callback = callback;

  krbn::process_lifecycle_manager::initialize_shared_instance(
      krbn::process_lifecycle_manager::configuration{
          .components_manager_maker =
              [] {
                return std::make_unique<components_manager>();
              },
          .termination_completion_handler = [] {},
      });
  krbn::process_lifecycle_manager::async_start();
}

void game_pad_viewer_terminate(void) {
  krbn::process_lifecycle_manager::terminate_shared_instance();
  scoped_run_loop_thread_manager = nullptr;
  scoped_dispatcher_manager = nullptr;
}

bool game_pad_viewer_hid_value_monitor_observed(void) {
  return hid_value_monitor_observed;
}
