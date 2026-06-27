#include "dispatcher_utility.hpp"
#include "hid_keyboard_caps_lock_led_state_manager.hpp"
#include "iokit_utility.hpp"
#include "run_loop_thread_utility.hpp"
#include <csignal>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_return.hpp>

namespace {
class control_led final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  control_led(const control_led&) = delete;

  control_led(krbn::led_state led_state) : dispatcher_client() {
    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::keyboard),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(weak_dispatcher_,
                                                                  pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this, led_state](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        auto device_id = krbn::make_device_id(registry_entry_id);
        auto device_name = krbn::iokit_utility::make_device_name_for_log(device_id,
                                                                         *device_ptr);
        krbn::logger::get_logger()->info("{0} is matched.", device_name);

        pqrs::osx::iokit_return r = IOHIDDeviceOpen(*device_ptr, kIOHIDOptionsTypeNone);
        if (!r) {
          std::cerr << "IOHIDDeviceOpen:" << r << " " << device_name << std::endl;
          return;
        }

        pqrs::not_null_shared_ptr_t<krbn::hid_keyboard_caps_lock_led_state_manager> manager =
            std::make_shared<krbn::hid_keyboard_caps_lock_led_state_manager>(*device_ptr);
        caps_lock_led_state_managers_.insert_or_assign(registry_entry_id, manager);

        manager->set_state(led_state);

        manager->async_start();
      }
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      krbn::logger::get_logger()->info("registry_entry_id:{0} is terminated.",
                                       type_safe::get(registry_entry_id));

      caps_lock_led_state_managers_.erase(registry_entry_id);
    });

    hid_manager_->async_start();
  }

  ~control_led() override {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
    });
  }

private:
  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id::value_t, pqrs::not_null_shared_ptr_t<krbn::hid_keyboard_caps_lock_led_state_manager>> caps_lock_led_state_managers_;
};

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  std::signal(SIGINT, [](int) noexcept {
    global_wait->notify();
  });

  if (geteuid() != 0) {
    krbn::logger::get_logger()->error("control_led requires root privilege.");
  } else {
    if (argc == 1) {
      krbn::logger::get_logger()->error("Usage: control_led on|off");
      krbn::logger::get_logger()->error("  Example: control_led on");
      krbn::logger::get_logger()->error("  Example: control_led off");
    } else {
      auto p = std::make_unique<control_led>(std::string(argv[1]) == "on" ? krbn::led_state::on : krbn::led_state::off);

      // ------------------------------------------------------------

      global_wait->wait_notice();

      // ------------------------------------------------------------

      p = nullptr;
    }
  }

  std::cout << "finished" << std::endl;

  return 0;
}
