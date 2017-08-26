#include "logger.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include "time_utility.hpp"
#include "virtual_hid_device_client.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  if (getuid() != 0) {
    krbn::logger::get_logger().error("virtual_device_client requires root privilege.");
  }

  auto virtual_hid_device_client_ptr = std::make_unique<krbn::virtual_hid_device_client>();
  krbn::manipulator::details::post_event_to_virtual_devices::queue queue;

  virtual_hid_device_client_ptr->client_connected.connect([&]() {
    std::cout << "connected" << std::endl;

    pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
    virtual_hid_device_client_ptr->initialize_virtual_hid_keyboard(properties);
  });

  virtual_hid_device_client_ptr->virtual_hid_keyboard_ready.connect([&]() {
    std::cout << "virtual_hid_keyboard_ready" << std::endl;

    {
      auto time_stamp = mach_absolute_time();
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardSpacebar),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);
    }
    {
      auto time_stamp = mach_absolute_time();
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardSpacebar),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);
    }

    {
      auto time_stamp = mach_absolute_time();

      // Put `Bc`.

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardLeftShift),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardB),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardB),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardLeftShift),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardC),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardLeftShift),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardC),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardLeftShift),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);
    }

    {
      auto time_stamp = mach_absolute_time() + krbn::time_utility::nano_to_absolute(NSEC_PER_SEC);
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardA),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);
    }
    {
      auto time_stamp = mach_absolute_time() + krbn::time_utility::nano_to_absolute(2 * NSEC_PER_SEC);
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardA),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);
    }

    {
      auto time_stamp = mach_absolute_time();
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardB),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);
    }
    {
      auto time_stamp = mach_absolute_time();
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardB),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.post_events(*virtual_hid_device_client_ptr);
    }
  });

  virtual_hid_device_client_ptr->connect();

  CFRunLoopRun();
  return 0;
}
