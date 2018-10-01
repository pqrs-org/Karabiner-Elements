#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device.hpp"
#include "hid_manager.hpp"

namespace {
class hid_set_report final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  hid_set_report(const hid_set_report&) = delete;

  hid_set_report(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
    std::vector<std::pair<krbn::hid_usage_page, krbn::hid_usage>> targets({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
    });

    hid_manager_ = std::make_unique<krbn::hid_manager>(weak_dispatcher,
                                                       targets);

    hid_manager_->device_detected.connect([](auto&& weak_hid) {
      if (auto hid = weak_hid.lock()) {
        hid->opened.connect([weak_hid] {
          if (auto hid = weak_hid.lock()) {
            pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input keyboard_input;
            const uint8_t* report = reinterpret_cast<const uint8_t*>(&keyboard_input);

            hid->async_set_report(kIOHIDReportTypeInput,
                                  report[0],
                                  report,
                                  sizeof(keyboard_input));
            krbn::logger::get_logger().info("async_set_report is called {0}", hid->get_name_for_log());
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

  ~hid_set_report(void) {
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

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  hid_set_report hid_set_report(dispatcher);

  CFRunLoopRun();

  return 0;
}
