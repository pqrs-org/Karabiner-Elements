#include "dispatcher_utility.hpp"
#include "hid_manager.hpp"

namespace {
class control_led final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  control_led(const control_led&) = delete;

  control_led(bool led_state) : dispatcher_client() {
    std::vector<std::pair<krbn::hid_usage_page, krbn::hid_usage>> targets({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
    });

    hid_manager_ = std::make_unique<krbn::hid_manager>(targets);

    hid_manager_->device_detected.connect([led_state](auto&& weak_hid) {
      if (auto hid = weak_hid.lock()) {
        krbn::logger::get_logger().info("{0} is detected.", hid->get_name_for_log());

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
      }
    });

    hid_manager_->async_start();
  }

  virtual ~control_led(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
    });
  }

private:
  std::unique_ptr<krbn::hid_manager> hid_manager_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
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

      CFRunLoopRun();

      p = nullptr;
    }
  }

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
