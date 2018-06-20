#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device.hpp"
#include "hid_manager.hpp"

namespace {
class hid_set_report final {
public:
  hid_set_report(const hid_set_report&) = delete;

  hid_set_report(void) {
    hid_manager_.device_detected.connect([&](auto&& human_interface_device) {
      auto r = human_interface_device.open();
      if (r != kIOReturnSuccess) {
        krbn::logger::get_logger().error("failed to open");
        return;
      }

      {
        pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input keyboard_input;
        const uint8_t* report = reinterpret_cast<const uint8_t*>(&keyboard_input);

        auto kr = human_interface_device.set_report(kIOHIDReportTypeInput,
                                                    report[0],
                                                    report,
                                                    sizeof(keyboard_input));
        if (kr != kIOReturnSuccess) {
          krbn::logger::get_logger().error("Failed to IOHIDDeviceSetReport: {0}",
                                           krbn::iokit_utility::get_error_name(kr));
          return;
        }
      }

      human_interface_device.close();
    });

    hid_manager_.start({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
    });
  }

  ~hid_set_report(void) {
    hid_manager_.stop();
  }

private:
  krbn::hid_manager hid_manager_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  hid_set_report hid_set_report;
  CFRunLoopRun();
  return 0;
}
