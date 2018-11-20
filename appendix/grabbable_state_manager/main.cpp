#include "boost_defs.hpp"

#include "dispatcher_utility.hpp"
#include "grabbable_state_manager.hpp"
#include "hid_observer.hpp"
#include <boost/optional/optional_io.hpp>
#include <csignal>
#include <pqrs/osx/iokit_hid_manager.hpp>

namespace {
class grabbable_state_manager_demo final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  grabbable_state_manager_demo(const grabbable_state_manager_demo&) = delete;

  grabbable_state_manager_demo(void) : dispatcher_client() {
    grabbable_state_manager_ = std::make_unique<krbn::grabbable_state_manager>();

    grabbable_state_manager_->grabbable_state_changed.connect([](auto&& grabbable_state) {
      std::cout << "grabbable_state_changed "
                << grabbable_state.get_registry_entry_id()
                << " "
                << grabbable_state.get_state()
                << std::endl;
    });

    std::vector<pqrs::cf_ptr<CFDictionaryRef>> matching_dictionaries{
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
      auto hid = std::make_shared<krbn::human_interface_device>(*device_ptr,
                                                                registry_entry_id);
      hids_[registry_entry_id] = hid;
      auto device_name = hid->get_name_for_log();

      hid->values_arrived.connect([this](auto&& shared_event_queue) {
        if (grabbable_state_manager_) {
          grabbable_state_manager_->update(*shared_event_queue);
        }
      });

      // Observe

      auto hid_observer = std::make_shared<krbn::hid_observer>(hid);

      hid_observer->device_observed.connect([device_name] {
        krbn::logger::get_logger().info("{0} is observed.", device_name);
      });

      hid_observer->device_unobserved.connect([device_name] {
        krbn::logger::get_logger().info("{0} is unobserved.", device_name);
      });

      hid_observer->async_observe();

      hid_observers_[hid->get_registry_entry_id()] = hid_observer;
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      krbn::logger::get_logger().info("registry_entry_id:{0} is terminated.", type_safe::get(registry_entry_id));

      hids_.erase(registry_entry_id);
      hid_observers_.erase(registry_entry_id);
    });

    hid_manager_->async_start();
  }

  virtual ~grabbable_state_manager_demo(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
      hid_observers_.clear();
    });
  }

private:
  std::unique_ptr<krbn::grabbable_state_manager> grabbable_state_manager_;
  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, std::shared_ptr<krbn::human_interface_device>> hids_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, std::shared_ptr<krbn::hid_observer>> hid_observers_;
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
