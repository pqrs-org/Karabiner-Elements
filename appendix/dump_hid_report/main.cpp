#include "hid_manager.hpp"

namespace {
class dump_hid_report final {
public:
  dump_hid_report(const dump_hid_report&) = delete;

  dump_hid_report(void) : hid_manager_({
                              std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
                              std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_mouse),
                              std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_pointer),
                          }) {
    hid_manager_.device_detected.connect([this](auto&& human_interface_device) {
      human_interface_device->report_arrived.connect([this](auto&& human_interface_device,
                                                            auto&& type,
                                                            auto&& report_id,
                                                            auto&& report,
                                                            auto&& report_length) {
        report_arrived(human_interface_device, type, report_id, report, report_length);
      });
      human_interface_device->enable_report_callback();

      auto r = human_interface_device->open();
      if (r != kIOReturnSuccess) {
        krbn::logger::get_logger().error("failed to open {0}", human_interface_device->get_name_for_log());
        return;
      }
      human_interface_device->schedule();
    });

    hid_manager_.start();
  }

  ~dump_hid_report(void) {
    hid_manager_.stop();
  }

private:
  void report_arrived(krbn::human_interface_device& device,
                      IOHIDReportType type,
                      uint32_t report_id,
                      uint8_t* report,
                      CFIndex report_length) {
    // Logitech Unifying Receiver sends a lot of null report. We ignore them.
    if (auto manufacturer = device.find_manufacturer()) {
      if (auto product = device.find_product()) {
        if (*manufacturer == "Logitech" && *product == "USB Receiver") {
          if (report_id == 0) {
            return;
          }
        }
      }
    }

    krbn::logger::get_logger().info("report_length: {0}", report_length);
    std::cout << "  report_id: " << std::dec << report_id << std::endl;
    for (CFIndex i = 0; i < report_length; ++i) {
      std::cout << "  key[" << i << "]: 0x" << std::hex << static_cast<int>(report[i]) << std::dec << std::endl;
    }
  }

  krbn::hid_manager hid_manager_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  dump_hid_report d;
  CFRunLoopRun();
  return 0;
}
