#include "dispatcher_utility.hpp"
#include "grabbable_state_manager/manager.hpp"
#include "iokit_utility.hpp"
#include <csignal>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>

namespace {
class grabbable_state_manager_demo final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  grabbable_state_manager_demo(const grabbable_state_manager_demo&) = delete;

  grabbable_state_manager_demo(void) : dispatcher_client() {
    grabbable_state_manager_ = std::make_unique<krbn::grabbable_state_manager::manager>();

    grabbable_state_manager_->grabbable_state_changed.connect([](auto&& grabbable_state) {
      std::cout << "grabbable_state_changed "
                << grabbable_state.get_device_id()
                << " "
                << grabbable_state.get_state()
                << std::endl;
    });

    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_keyboard),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_mouse),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_pointer),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(weak_dispatcher_,
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        auto device_id = krbn::make_device_id(registry_entry_id);

        std::cout << krbn::iokit_utility::make_device_name_for_log(device_id,
                                                                   *device_ptr)
                  << "is matched." << std::endl;

        auto hid_queue_value_monitor = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(weak_dispatcher_,
                                                                                                  *device_ptr);
        hid_queue_value_monitors_[device_id] = hid_queue_value_monitor;

        hid_queue_value_monitor->values_arrived.connect([this, device_id](auto&& values_ptr) {
          if (grabbable_state_manager_) {
            auto event_queue = krbn::event_queue::utility::make_queue(device_id,
                                                                      krbn::iokit_utility::make_hid_values(values_ptr));
            grabbable_state_manager_->update(*event_queue);
          }
        });

        hid_queue_value_monitor->async_start(kIOHIDOptionsTypeNone,
                                             std::chrono::milliseconds(3000));
      }
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      krbn::logger::get_logger().info("registry_entry_id:{0} is terminated.", type_safe::get(registry_entry_id));

      auto device_id = krbn::make_device_id(registry_entry_id);
      hid_queue_value_monitors_.erase(device_id);
    });

    hid_manager_->async_start();
  }

  virtual ~grabbable_state_manager_demo(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
      hid_queue_value_monitors_.clear();
    });
  }

private:
  std::unique_ptr<krbn::grabbable_state_manager::manager> grabbable_state_manager_;
  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<krbn::device_id, std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor>> hid_queue_value_monitors_;
};

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  std::signal(SIGINT, [](int) {
    global_wait->notify();
  });

  auto d = std::make_unique<grabbable_state_manager_demo>();

  // ------------------------------------------------------------

  global_wait->wait_notice();

  // ------------------------------------------------------------

  d = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  std::cout << "finished" << std::endl;

  return 0;
}
