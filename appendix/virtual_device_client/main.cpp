#include "dispatcher_utility.hpp"
#include "logger.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "time_utility.hpp"
#include "virtual_hid_device_client.hpp"

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  if (getuid() != 0) {
    krbn::logger::get_logger()->error("virtual_device_client requires root privilege.");
  }

  auto console_user_server_client = std::make_shared<krbn::console_user_server_client>();

  auto virtual_hid_device_client = std::make_shared<krbn::virtual_hid_device_client>();
  krbn::manipulator::details::post_event_to_virtual_devices_detail::queue queue;

  virtual_hid_device_client->client_connected.connect([&] {
    std::cout << "connected" << std::endl;

    pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
    virtual_hid_device_client->async_initialize_virtual_hid_keyboard(properties);
  });

  virtual_hid_device_client->virtual_hid_keyboard_ready.connect([&] {
    std::cout << "virtual_hid_keyboard_ready" << std::endl;

    {
      auto time_stamp = krbn::time_utility::mach_absolute_time_point();
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardSpacebar),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);
    }
    {
      auto time_stamp = krbn::time_utility::mach_absolute_time_point();
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardSpacebar),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);
    }

    {
      auto time_stamp = krbn::time_utility::mach_absolute_time_point();

      // Put `Bc`.

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardLeftShift),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardB),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardB),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardLeftShift),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardC),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardLeftShift),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardC),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);

      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardLeftShift),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);
    }

    {
      auto time_stamp = krbn::time_utility::mach_absolute_time_point() +
                        krbn::time_utility::to_absolute_time_duration(std::chrono::milliseconds(1000));
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardA),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);
    }
    {
      auto time_stamp = krbn::time_utility::mach_absolute_time_point() +
                        krbn::time_utility::to_absolute_time_duration(std::chrono::milliseconds(2000));
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardA),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);
    }

    {
      auto time_stamp = krbn::time_utility::mach_absolute_time_point();
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardB),
                                   krbn::event_type::key_down,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);
    }
    {
      auto time_stamp = krbn::time_utility::mach_absolute_time_point();
      queue.emplace_back_key_event(krbn::hid_usage_page::keyboard_or_keypad,
                                   krbn::hid_usage(kHIDUsage_KeyboardB),
                                   krbn::event_type::key_up,
                                   time_stamp);
      queue.async_post_events(virtual_hid_device_client,
                              console_user_server_client);
    }
  });

  virtual_hid_device_client->async_connect();

  // ------------------------------------------------------------

  CFRunLoopRun();

  // ------------------------------------------------------------

  virtual_hid_device_client = nullptr;
  console_user_server_client = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
