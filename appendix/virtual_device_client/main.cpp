#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include "time_utility.hpp"
#include "virtual_hid_device_client.hpp"

namespace {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_color_mt("virtual_device_client");
    }
    return *logger;
  }
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  if (getuid() != 0) {
    logger::get_logger().error("virtual_device_client requires root privilege.");
  }

  auto virtual_hid_device_client_ptr = std::make_unique<krbn::virtual_hid_device_client>(logger::get_logger());
  krbn::manipulator::details::post_event_to_virtual_devices::queue queue(*virtual_hid_device_client_ptr);

  virtual_hid_device_client_ptr->client_connected.connect([&]() {
    std::cout << "connected" << std::endl;

    pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
    virtual_hid_device_client_ptr->initialize_virtual_hid_keyboard(properties);
  });

  virtual_hid_device_client_ptr->virtual_hid_keyboard_ready.connect([&]() {
    std::cout << "virtual_hid_keyboard_ready" << std::endl;

    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event event;
      event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardSpacebar);
      event.value = 1;
      auto time_stamp = mach_absolute_time();
      queue.emplace_back_event(event, time_stamp);
    }
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event event;
      event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardSpacebar);
      event.value = 0;
      auto time_stamp = mach_absolute_time();
      queue.emplace_back_event(event, time_stamp);
    }

    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event event;
      event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardA);
      event.value = 1;
      auto time_stamp = mach_absolute_time() + krbn::time_utility::nano_to_absolute(NSEC_PER_SEC);
      queue.emplace_back_event(event, time_stamp);
    }
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event event;
      event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardA);
      event.value = 0;
      auto time_stamp = mach_absolute_time() + krbn::time_utility::nano_to_absolute(3 * NSEC_PER_SEC);
      queue.emplace_back_event(event, time_stamp);
    }

    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event event;
      event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardB);
      event.value = 1;
      auto time_stamp = mach_absolute_time();
      queue.emplace_back_event(event, time_stamp);
    }
    {
      pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event event;
      event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardB);
      event.value = 0;
      auto time_stamp = mach_absolute_time();
      queue.emplace_back_event(event, time_stamp);
    }
  });

  virtual_hid_device_client_ptr->connect();

  CFRunLoopRun();
  return 0;
}
