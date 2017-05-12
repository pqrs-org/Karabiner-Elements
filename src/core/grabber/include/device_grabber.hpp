#pragma once

#include "boost_defs.hpp"

#include "configuration_monitor.hpp"
#include "constants.hpp"
//#include "event_manipulator.hpp"
//#include "event_tap_manager.hpp"
//#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
//#include "iokit_utility.hpp"
#include "virtual_hid_device_client.hpp"
#include "physical_keyboard_repeat_detector.hpp"
#include "pressed_physical_keys_counter.hpp"
//#include "types.hpp"

#include <IOKit/hid/IOHIDManager.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>


namespace krbn {
  
  namespace manipulator {
    class event_manipulator;
  }
  
class event_queue;
class event_tap_manager;
//class human_interface_device;
//class virtual_hid_device_client;
//class pressed_physical_keys_counter;
//class physical_keyboard_repeat_detector;
  
class device_grabber final {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(virtual_hid_device_client& virtual_hid_device_client,
                 manipulator::event_manipulator& event_manipulator);
  
  
  ~device_grabber(void);

  void start_grabbing(const std::string& user_core_configuration_file_path);

  void stop_grabbing(void) ;
  void grab_devices(void);
  
  void ungrab_devices(void) ;
  
  void suspend(void) ;
  void resume(void);
private:
  enum class mode {
    observing,
    grabbing,
  };

  void virtual_hid_device_client_disconnected_callback(void);
  static void static_device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device);
  void device_matching_callback(IOHIDDeviceRef _Nonnull device);
  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) ;
  void device_removal_callback(IOHIDDeviceRef _Nonnull device);
  void value_callback(human_interface_device& device,
                      event_queue& event_queue) ;
  
  human_interface_device::grabbable_state is_grabbable_callback(human_interface_device& device) ;
  void grabbed_callback(human_interface_device& device);
  void ungrabbed_callback(human_interface_device& device) ;
  void disabled_callback(human_interface_device& device) ;
  void caps_lock_state_changed_callback(bool caps_lock_state);
  void update_caps_lock_led(bool caps_lock_state) ;
  bool is_keyboard_connected(void);
  bool is_pointing_device_connected(void) ;
  boost::optional<const core_configuration::profile::device&> find_device_configuration(const human_interface_device& device);
  
  bool is_ignored_device(const human_interface_device& device);
  bool get_disable_built_in_keyboard_if_exists(const human_interface_device& device) ;
  bool need_to_disable_built_in_keyboard(void) ;
  void enable_devices(void);
  void output_devices_json(void) ;

  virtual_hid_device_client& virtual_hid_device_client_;
  manipulator::event_manipulator& event_manipulator_;

  boost::signals2::connection virtual_hid_device_client_disconnected_connection;

  std::unique_ptr<configuration_monitor> configuration_monitor_;
  std::shared_ptr<core_configuration> core_configuration_;

  std::unique_ptr<event_tap_manager> event_tap_manager_;
  IOHIDManagerRef _Nullable manager_;
  physical_keyboard_repeat_detector physical_keyboard_repeat_detector_;
  pressed_physical_keys_counter pressed_physical_keys_counter_;

  event_queue simple_modifications_applied_event_queue_;

  std::unique_ptr<gcd_utility::main_queue_timer> led_monitor_timer_;

  mode mode_;

  spdlog_utility::log_reducer is_grabbable_callback_log_reducer_;

  bool suspended_;
    
public:
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;
  
  static device_grabber * _Nullable grabber;
  static device_grabber * _Nullable get_grabber() { return grabber; }
};
} // namespace krbn
