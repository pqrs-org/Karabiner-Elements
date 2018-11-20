#include "dispatcher_utility.hpp"
#include "human_interface_device.hpp"
#include <csignal>
#include <pqrs/osx/iokit_hid_manager.hpp>

namespace {
class control_led final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  control_led(const control_led&) = delete;

  control_led(bool led_state) : dispatcher_client() {
    std::vector<pqrs::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_keyboard),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(weak_dispatcher_,
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this, led_state](auto&& registry_entry_id, auto&& device_ptr) {
      auto hid = std::make_shared<krbn::human_interface_device>(*device_ptr,
                                                                registry_entry_id);
      hids_[registry_entry_id] = hid;
      auto device_name = hid->get_name_for_log();
      auto weak_hid = std::weak_ptr<krbn::human_interface_device>(hid);

      krbn::logger::get_logger().info("{0}:{1} is matched.", device_name,
                                      type_safe::get(registry_entry_id));

      hid->opened.connect([led_state, weak_hid] {
        if (auto hid = weak_hid.lock()) {
          hid->async_set_caps_lock_led_state(led_state ? krbn::led_state::on : krbn::led_state::off);
          krbn::logger::get_logger().info("async_set_caps_lock_led_state is called.");
        }
      });

      hid->open_failed.connect([weak_hid](auto&& error_code) {
        if (auto hid = weak_hid.lock()) {
          krbn::logger::get_logger().error("failed to open {0}", hid->get_name_for_log());
        }
      });

      hid->async_open();
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      krbn::logger::get_logger().info("registry_entry_id:{0} is terminated.",
                                      type_safe::get(registry_entry_id));

      hids_.erase(registry_entry_id);
    });

    hid_manager_->async_start();
  }

  virtual ~control_led(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
    });
  }

private:
  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, std::shared_ptr<krbn::human_interface_device>> hids_;
};

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  std::signal(SIGINT, [](int) {
    global_wait->notify();
  });

  if (getuid() != 0) {
    krbn::logger::get_logger().error("control_led requires root privilege.");
  } else {
    if (argc == 1) {
      krbn::logger::get_logger().error("Usage: control_led on|off");
      krbn::logger::get_logger().error("  Example: control_led on");
      krbn::logger::get_logger().error("  Example: control_led off");
    } else {
      auto p = std::make_unique<control_led>(std::string(argv[1]) == "on");

      // ------------------------------------------------------------

      global_wait->wait_notice();

      // ------------------------------------------------------------

      p = nullptr;
    }
  }

  krbn::dispatcher_utility::terminate_dispatchers();

  std::cout << "finished" << std::endl;

  return 0;
}
