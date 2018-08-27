#include "hid_manager.hpp"

namespace {
class dump_hid_report final {
public:
  dump_hid_report(const dump_hid_report&) = delete;

  dump_hid_report(void) {
    std::vector<std::pair<krbn::hid_usage_page, krbn::hid_usage>> targets({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_mouse),
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_pointer),
    });

    hid_manager_ = std::make_unique<krbn::hid_manager>(targets);

    hid_manager_->device_detected.connect([this](auto&& weak_hid) {
      if (auto hid = weak_hid.lock()) {
        hid->report_arrived.connect([this, weak_hid](auto&& type,
                                                     auto&& report_id,
                                                     auto&& report,
                                                     auto&& report_length) {
          if (auto hid = weak_hid.lock()) {
            report_arrived(hid, type, report_id, report, report_length);
          }
        });

        hid->async_enable_report_callback();
        hid->async_open();
        hid->async_schedule();
      }
    });

    hid_manager_->async_start();
  }

  ~dump_hid_report(void) {
    hid_manager_ = nullptr;
  }

private:
  void report_arrived(std::shared_ptr<krbn::human_interface_device> hid,
                      IOHIDReportType type,
                      uint32_t report_id,
                      uint8_t* report,
                      CFIndex report_length) const {
    // Logitech Unifying Receiver sends a lot of null report. We ignore them.
    if (auto manufacturer = hid->find_manufacturer()) {
      if (auto product = hid->find_product()) {
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

  std::unique_ptr<krbn::hid_manager> hid_manager_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto d = std::make_unique<dump_hid_report>();

  CFRunLoopRun();

  d = nullptr;

  return 0;
}
