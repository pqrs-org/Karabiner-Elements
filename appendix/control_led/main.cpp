#include "hid_manager.hpp"

namespace {
class control_led final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  control_led(const control_led&) = delete;

  control_led(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
              bool led_state) : dispatcher_client(weak_dispatcher) {
    std::vector<std::pair<krbn::hid_usage_page, krbn::hid_usage>> targets({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
    });

    hid_manager_ = std::make_unique<krbn::hid_manager>(weak_dispatcher,
                                                       targets);

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

  ~control_led(void) {
    hid_manager_ = nullptr;
  }

private:
  std::unique_ptr<krbn::hid_manager> hid_manager_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  if (getuid() != 0) {
    krbn::logger::get_logger().error("control_led requires root privilege.");
    return 1;
  }

  if (argc == 1) {
    krbn::logger::get_logger().error("Usage: control_led on|off");
    krbn::logger::get_logger().error("  Example: control_led on");
    krbn::logger::get_logger().error("  Example: control_led off");
    return 1;
  }

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  auto p = std::make_unique<control_led>(dispatcher,
                                         std::string(argv[1]) == "on");

  CFRunLoopRun();

  p = nullptr;

  return 0;
}
