#include "dispatcher_utility.hpp"
#include "human_interface_device.hpp"
#include <csignal>
#include <pqrs/osx/iokit_hid_manager.hpp>

namespace {
class dump_hid_report final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  dump_hid_report(const dump_hid_report&) = delete;

  dump_hid_report(void) : dispatcher_client() {
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

    hid_manager_->device_detected.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      auto hid = std::make_shared<krbn::human_interface_device>(*device_ptr,
                                                                registry_entry_id);
      hids_[registry_entry_id] = hid;
      auto manufacturer = hid->find_manufacturer();
      auto product = hid->find_product();

      hid->report_arrived.connect([this, manufacturer, product](auto&& type,
                                                                auto&& report_id,
                                                                auto&& report) {
        report_arrived(manufacturer, product, type, report_id, report);
      });

      hid->async_enable_report_callback();
      hid->async_open();
      hid->async_schedule();
    });

    hid_manager_->device_removed.connect([this](auto&& registry_entry_id) {
      hids_.erase(registry_entry_id);
    });

    hid_manager_->async_start();
  }

  virtual ~dump_hid_report(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
    });
  }

private:
  void report_arrived(boost::optional<std::string> manufacturer,
                      boost::optional<std::string> product,
                      IOHIDReportType type,
                      uint32_t report_id,
                      std::shared_ptr<std::vector<uint8_t>> report) const {
    // Logitech Unifying Receiver sends a lot of null report. We ignore them.
    if (manufacturer) {
      if (product) {
        if (*manufacturer == "Logitech" && *product == "USB Receiver") {
          if (report_id == 0) {
            return;
          }
        }
      }
    }

    krbn::logger::get_logger().info("report_length: {0}", report->size());
    std::cout << "  report_id: " << std::dec << report_id << std::endl;
    for (CFIndex i = 0; i < report->size(); ++i) {
      std::cout << "  key[" << i << "]: 0x" << std::hex << static_cast<int>((*report)[i]) << std::dec << std::endl;
    }
  }

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

  auto d = std::make_unique<dump_hid_report>();

  // ------------------------------------------------------------

  global_wait->wait_notice();

  // ------------------------------------------------------------

  d = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  std::cout << "finished" << std::endl;

  return 0;
}
