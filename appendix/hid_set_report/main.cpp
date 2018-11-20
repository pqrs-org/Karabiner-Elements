#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device.hpp"
#include "dispatcher_utility.hpp"
#include "human_interface_device.hpp"
#include <csignal>
#include <pqrs/osx/iokit_hid_manager.hpp>

namespace {
class hid_set_report final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  hid_set_report(const hid_set_report&) = delete;

  hid_set_report(void) : dispatcher_client() {
    std::vector<pqrs::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_keyboard),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(weak_dispatcher_,
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      auto hid = std::make_shared<krbn::human_interface_device>(*device_ptr,
                                                                registry_entry_id);
      hids_[registry_entry_id] = hid;
      auto device_name = hid->get_name_for_log();
      auto weak_hid = std::weak_ptr<krbn::human_interface_device>(hid);

      hid->opened.connect([device_name, weak_hid] {
        if (auto hid = weak_hid.lock()) {
          pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input keyboard_input;
          auto report = std::make_shared<std::vector<uint8_t>>(sizeof(keyboard_input));
          memcpy(&((*report)[0]), &keyboard_input, sizeof(keyboard_input));

          hid->async_set_report(kIOHIDReportTypeInput,
                                (*report)[0],
                                report);
          krbn::logger::get_logger().info("async_set_report is called {0}", device_name);
        }
      });

      hid->open_failed.connect([device_name](auto&& error_code) {
        krbn::logger::get_logger().error("failed to open {0}", device_name);
      });

      hid->async_open();
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      hids_.erase(registry_entry_id);
    });

    hid_manager_->async_start();
  }

  virtual ~hid_set_report(void) {
    hid_manager_ = nullptr;
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

  auto r = std::make_unique<hid_set_report>();

  // ------------------------------------------------------------

  global_wait->wait_notice();

  // ------------------------------------------------------------

  r = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  std::cout << "finished" << std::endl;

  return 0;
}
