#include "hid_manager.hpp"

namespace {
class control_led final {
public:
  control_led(const control_led&) = delete;

  control_led(void) : hid_manager_({
                          std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
                      }) {
    hid_manager_.device_detected.connect([](auto&& human_interface_device) {
      auto r = human_interface_device->open();
      if (r != kIOReturnSuccess) {
        krbn::logger::get_logger().error("failed to open");
        return;
      }

      if (auto caps_lock_led_state = human_interface_device->get_caps_lock_led_state()) {
        switch (*caps_lock_led_state) {
          case krbn::led_state::on:
            krbn::logger::get_logger().info("caps_lock_led_state is on.");
            break;
          case krbn::led_state::off:
            krbn::logger::get_logger().info("caps_lock_led_state is off.");
            break;
        }

        if (caps_lock_led_state == krbn::led_state::on) {
          human_interface_device->set_caps_lock_led_state(krbn::led_state::off);
        } else {
          human_interface_device->set_caps_lock_led_state(krbn::led_state::on);
        }

        krbn::logger::get_logger().info("set_caps_lock_led_state is called.");

      } else {
        krbn::logger::get_logger().info("failed to get caps_lock_led_state.");
      }
    });

    hid_manager_.start();
  }

  ~control_led(void) {
    hid_manager_.stop();
  }

private:
  krbn::hid_manager hid_manager_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  if (getuid() != 0) {
    krbn::logger::get_logger().error("control_led requires root privilege.");
    return 1;
  }

  control_led d;
  CFRunLoopRun();
  return 0;
}
