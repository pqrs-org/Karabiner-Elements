#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "event_tap_manager.hpp"
#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "krbn_notification_center.hpp"
#include "logger.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include "spdlog_utility.hpp"
#include "system_preferences.hpp"
#include "types.hpp"
#include "virtual_hid_device_client.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>

namespace krbn {
class device_grabber final {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(void) : profile_(nlohmann::json()),
                         mode_(mode::observing),
                         suspended_(false) {
    client_connected_connection = virtual_hid_device_client_.client_connected.connect([&]() {
      logger::get_logger().info("virtual_hid_device_client_ is connected");

      update_virtual_hid_keyboard();
      update_virtual_hid_pointing();
    });

    client_disconnected_connection = virtual_hid_device_client_.client_disconnected.connect([&]() {
      logger::get_logger().info("virtual_hid_device_client_ is disconnected");

      stop_grabbing();
    });

    post_event_to_virtual_devices_manipulator_ = std::make_shared<manipulator::details::post_event_to_virtual_devices>();
    post_event_to_virtual_devices_manipulator_manager_.push_back_manipulator(std::shared_ptr<manipulator::details::base>(post_event_to_virtual_devices_manipulator_));

    complex_modifications_applied_event_queue_.enable_manipulator_environment_json_output(constants::get_manipulator_environment_json_file_path());

    // Connect manipulator_managers

    manipulator_managers_connector_.emplace_back_connection(simple_modifications_manipulator_manager_,
                                                            merged_input_event_queue_,
                                                            simple_modifications_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(complex_modifications_manipulator_manager_,
                                                            complex_modifications_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(fn_function_keys_manipulator_manager_,
                                                            fn_function_keys_applied_event_queue_);
    manipulator_managers_connector_.emplace_back_connection(post_event_to_virtual_devices_manipulator_manager_,
                                                            posted_event_queue_);

    input_event_arrived_connection = krbn_notification_center::get_instance().input_event_arrived.connect([&]() {
      manipulate();
    });

    // macOS 10.12 sometimes synchronize caps lock LED to internal keyboard caps lock state.
    // The behavior causes LED state mismatch because device_grabber does not change the caps lock state of physical keyboards.
    // Thus, we monitor the LED state and update it if needed.
    led_monitor_timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC),
        1.0 * NSEC_PER_SEC,
        0,
        ^{
          if (event_tap_manager_) {
            if (auto state = event_tap_manager_->get_caps_lock_state()) {
              update_caps_lock_led(*state);
            }
          }
        });

    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      logger::get_logger().error("{0}: failed to IOHIDManagerCreate", __PRETTY_FUNCTION__);
      return;
    }

    auto device_matching_dictionaries = iokit_utility::create_device_matching_dictionaries({
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_pointer),
    });
    if (device_matching_dictionaries) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }
  }

  ~device_grabber(void) {
    // Release manager_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      stop_grabbing();

      input_event_arrived_connection.disconnect();

      if (manager_) {
        IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
        CFRelease(manager_);
        manager_ = nullptr;
      }

      led_monitor_timer_ = nullptr;

      client_connected_connection.disconnect();
      client_disconnected_connection.disconnect();
    });
  }

  void start_grabbing(const std::string& user_core_configuration_file_path) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      mode_ = mode::grabbing;

      // We should call CGEventTapCreate after user is logged in.
      // So, we create event_tap_manager here.
      event_tap_manager_ = std::make_unique<event_tap_manager>(std::bind(&device_grabber::caps_lock_state_changed_callback,
                                                                         this,
                                                                         std::placeholders::_1),
                                                               std::bind(&device_grabber::event_tap_pointing_device_event_callback,
                                                                         this,
                                                                         std::placeholders::_1,
                                                                         std::placeholders::_2));

      configuration_monitor_ = std::make_unique<configuration_monitor>(user_core_configuration_file_path,
                                                                       [this](std::shared_ptr<core_configuration> core_configuration) {
                                                                         core_configuration_ = core_configuration;

                                                                         is_grabbable_callback_log_reducer_.reset();
                                                                         set_profile(core_configuration_->get_selected_profile());
                                                                         grab_devices();
                                                                       });

      virtual_hid_device_client_.connect();
    });
  }

  void stop_grabbing(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      configuration_monitor_ = nullptr;

      ungrab_devices();

      mode_ = mode::observing;

      event_tap_manager_ = nullptr;

      virtual_hid_device_client_.close();
    });
  }

  void grab_devices(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      for (const auto& it : hids_) {
        if ((it.second)->is_grabbable() == human_interface_device::grabbable_state::ungrabbable_permanently) {
          (it.second)->ungrab();
        } else {
          (it.second)->grab();
        }
      }

      enable_devices();
    });
  }

  void ungrab_devices(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      for (auto&& it : hids_) {
        (it.second)->ungrab();
      }

      logger::get_logger().info("Connected devices are ungrabbed");
    });
  }

  void suspend(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (!suspended_) {
        logger::get_logger().info("device_grabber::suspend");

        suspended_ = true;

        if (mode_ == mode::grabbing) {
          ungrab_devices();
        }
      }
    });
  }

  void resume(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (suspended_) {
        logger::get_logger().info("device_grabber::resume");

        suspended_ = false;

        if (mode_ == mode::grabbing) {
          grab_devices();
        }
      }
    });
  }

  void unset_profile(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      profile_ = core_configuration::profile(nlohmann::json());

      manipulator_managers_connector_.invalidate_manipulators();
    });
  }

  void set_system_preferences_values(const system_preferences::values& values) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      system_preferences_values_ = values;

      update_fn_function_keys_manipulators();
    });
  }

  void post_frontmost_application_changed_event(const std::string& bundle_identifier,
                                                const std::string& file_path) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      auto event = event_queue::queued_event::event::make_frontmost_application_changed_event(bundle_identifier,
                                                                                              file_path);
      merged_input_event_queue_.emplace_back_event(device_id(0),
                                                   mach_absolute_time(),
                                                   event,
                                                   event_type::single,
                                                   event);

      krbn_notification_center::get_instance().input_event_arrived();
    });
  }

  void post_input_source_changed_event(const input_source_identifiers& input_source_identifiers) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      auto event = event_queue::queued_event::event::make_input_source_changed_event(input_source_identifiers);
      merged_input_event_queue_.emplace_back_event(device_id(0),
                                                   mach_absolute_time(),
                                                   event,
                                                   event_type::single,
                                                   event);

      krbn_notification_center::get_instance().input_event_arrived();
    });
  }

private:
  enum class mode {
    observing,
    grabbing,
  };

  static void static_device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<device_grabber*>(context);
    if (!self) {
      return;
    }

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    // Skip if same device is already matched.
    // (Multiple usage device (e.g. usage::pointer and usage::mouse) will be matched twice.)
    if (auto registry_entry_id = iokit_utility::get_registry_entry_id(device)) {
      for (const auto& h : hids_) {
        if (auto e = h.second->get_registry_entry_id()) {
          if (*registry_entry_id == *e) {
            return;
          }
        }
      }
    }

    iokit_utility::log_matching_device(device);

    auto dev = std::make_unique<human_interface_device>(device);
    dev->set_is_grabbable_callback(std::bind(&device_grabber::is_grabbable_callback, this, std::placeholders::_1));
    dev->set_grabbed_callback(std::bind(&device_grabber::grabbed_callback, this, std::placeholders::_1));
    dev->set_ungrabbed_callback(std::bind(&device_grabber::ungrabbed_callback, this, std::placeholders::_1));
    dev->set_disabled_callback(std::bind(&device_grabber::disabled_callback, this, std::placeholders::_1));
    dev->set_value_callback(std::bind(&device_grabber::value_callback,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
    logger::get_logger().info("{0} is detected.", dev->get_name_for_log());

    dev->observe();

    hids_[device] = std::move(dev);

    output_devices_json();

    update_virtual_hid_pointing();

    // ----------------------------------------
    if (mode_ == mode::grabbing) {
      grab_devices();
    }
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<device_grabber*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback(device);
  }

  void device_removal_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    iokit_utility::log_removal_device(device);

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        logger::get_logger().info("{0} is removed.", dev->get_name_for_log());
        dev->set_removed();
        dev->ungrab();
        if (dev->is_pqrs_virtual_hid_keyboard()) {
          virtual_hid_device_client_.close();
          ungrab_devices();

          virtual_hid_device_client_.connect();
          grab_devices();
        }
        hids_.erase(it);
      }
    }

    output_devices_json();

    update_virtual_hid_pointing();

    // ----------------------------------------
    if (mode_ == mode::grabbing) {
      grab_devices();
    }
  }

  void manipulate(void) {
    manipulator_managers_connector_.manipulate();

    posted_event_queue_.clear_events();
    post_event_to_virtual_devices_manipulator_->post_events(virtual_hid_device_client_);
  }

  void value_callback(human_interface_device& device,
                      event_queue& event_queue) {
    if (device.get_disabled()) {
      // Do nothing
    } else {
      for (const auto& queued_event : event_queue.get_events()) {
        if (device.is_grabbed()) {
          merged_input_event_queue_.push_back_event(queued_event);
        } else {
          // device is ignored
          auto event = event_queue::queued_event::event::make_event_from_ignored_device_event();
          merged_input_event_queue_.emplace_back_event(queued_event.get_device_id(),
                                                       queued_event.get_time_stamp(),
                                                       event,
                                                       queued_event.get_event_type(),
                                                       queued_event.get_event());
        }
      }
    }

    krbn_notification_center::get_instance().input_event_arrived();
    // manipulator_managers_connector_.log_events_sizes(logger::get_logger());
  }

  void post_device_ungrabbed_event(device_id device_id) {
    auto event = event_queue::queued_event::event::make_device_ungrabbed_event();
    merged_input_event_queue_.emplace_back_event(device_id,
                                                 mach_absolute_time(),
                                                 event,
                                                 event_type::single,
                                                 event);

    krbn_notification_center::get_instance().input_event_arrived();
  }

  void post_caps_lock_state_changed_callback(bool caps_lock_state) {
    event_queue::queued_event::event event(event_queue::queued_event::event::type::caps_lock_state_changed, caps_lock_state);
    merged_input_event_queue_.emplace_back_event(device_id(0),
                                                 mach_absolute_time(),
                                                 event,
                                                 event_type::single,
                                                 event);

    krbn_notification_center::get_instance().input_event_arrived();
  }

  human_interface_device::grabbable_state is_grabbable_callback(human_interface_device& device) {
    if (is_ignored_device(device)) {
      // If we need to disable the built-in keyboard, we have to grab it.
      if (device.is_built_in_keyboard() && need_to_disable_built_in_keyboard()) {
        // Do nothing
      } else {
        logger::get_logger().info("{0} is ignored.", device.get_name_for_log());
        return human_interface_device::grabbable_state::ungrabbable_permanently;
      }
    }

    // ----------------------------------------
    // Ungrabbable while virtual_hid_device_client_ is not ready.

    if (!virtual_hid_device_client_.is_connected()) {
      std::string message = "virtual_hid_device_client is not connected yet. Please wait for a while.";
      is_grabbable_callback_log_reducer_.warn(message);
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    if (!virtual_hid_device_client_.is_virtual_hid_keyboard_ready()) {
      std::string message = "virtual_hid_keyboard is not ready. Please wait for a while.";
      is_grabbable_callback_log_reducer_.warn(message);
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    {
      bool found = false;
      for (const auto& it : hids_) {
        if ((it.second)->is_pqrs_virtual_hid_keyboard()) {
          found = true;
        }
      }
      if (!found) {
        std::string message = "virtual_hid_keyboard is not detected. Please wait for a while.";
        is_grabbable_callback_log_reducer_.warn(message);
        return human_interface_device::grabbable_state::ungrabbable_temporarily;
      }
    }

    // ----------------------------------------

    return human_interface_device::grabbable_state::grabbable;
  }

  void grabbed_callback(human_interface_device& device) {
    // set keyboard led
    if (event_tap_manager_) {
      bool state = false;
      if (auto s = event_tap_manager_->get_caps_lock_state()) {
        state = *s;
      }
      update_caps_lock_led(state);
    }

    update_virtual_hid_pointing();
  }

  void ungrabbed_callback(human_interface_device& device) {
    post_device_ungrabbed_event(device.get_device_id());
  }

  void disabled_callback(human_interface_device& device) {
    // Post device_ungrabbed event in order to release modifier_flags.
    post_device_ungrabbed_event(device.get_device_id());
  }

  void caps_lock_state_changed_callback(bool caps_lock_state) {
    post_caps_lock_state_changed_callback(caps_lock_state);
    update_caps_lock_led(caps_lock_state);
  }

  void event_tap_pointing_device_event_callback(CGEventType type, CGEventRef _Nullable event) {
    boost::optional<event_type> pseudo_event_type;
    boost::optional<event_queue::queued_event::event> pseudo_event;

    switch (type) {
      case kCGEventLeftMouseDown:
        pseudo_event_type = event_type::key_down;
        pseudo_event = event_queue::queued_event::event(pointing_button::button1);
        break;

      case kCGEventLeftMouseUp:
        pseudo_event_type = event_type::key_up;
        pseudo_event = event_queue::queued_event::event(pointing_button::button1);
        break;

      case kCGEventRightMouseDown:
        pseudo_event_type = event_type::key_down;
        pseudo_event = event_queue::queued_event::event(pointing_button::button2);
        break;

      case kCGEventRightMouseUp:
        pseudo_event_type = event_type::key_up;
        pseudo_event = event_queue::queued_event::event(pointing_button::button2);
        break;

      case kCGEventOtherMouseDown:
        pseudo_event_type = event_type::key_down;
        pseudo_event = event_queue::queued_event::event(pointing_button::button3);
        break;

      case kCGEventOtherMouseUp:
        pseudo_event_type = event_type::key_up;
        pseudo_event = event_queue::queued_event::event(pointing_button::button3);
        break;

      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:
      case kCGEventRightMouseDragged:
      case kCGEventOtherMouseDragged:
        pseudo_event_type = event_type::single;
        pseudo_event = event_queue::queued_event::event(event_queue::queued_event::event::type::pointing_x, 0);
        break;

      case kCGEventScrollWheel:
        pseudo_event_type = event_type::single;
        // Set non-zero value for `manipulator::details::base::unset_alone_if_needed`.
        pseudo_event = event_queue::queued_event::event(event_queue::queued_event::event::type::pointing_vertical_wheel, 1);
        break;

      case kCGEventNull:
      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventFlagsChanged:
      case kCGEventTabletPointer:
      case kCGEventTabletProximity:
      case kCGEventTapDisabledByTimeout:
      case kCGEventTapDisabledByUserInput:
        break;
    }

    if (pseudo_event_type && pseudo_event) {
      auto e = event_queue::queued_event::event::make_pointing_device_event_from_event_tap_event();
      merged_input_event_queue_.emplace_back_event(device_id(0),
                                                   mach_absolute_time(),
                                                   e,
                                                   *pseudo_event_type,
                                                   *pseudo_event);

      krbn_notification_center::get_instance().input_event_arrived();
    }
  }

  void update_caps_lock_led(bool caps_lock_state) {
    // Update LED.
    for (const auto& it : hids_) {
      if ((it.second)->is_grabbed()) {
        (it.second)->set_caps_lock_led_state(caps_lock_state ? led_state::on : led_state::off);
      }
    }
  }

  bool is_keyboard_connected(void) const {
    for (const auto& it : hids_) {
      if ((it.second)->is_keyboard()) {
        return true;
      }
    }
    return false;
  }

  bool is_pointing_device_grabbed(void) const {
    for (const auto& it : hids_) {
      if ((it.second)->is_pointing_device() &&
          (it.second)->is_grabbed()) {
        return true;
      }
    }
    return false;
  }

  void update_virtual_hid_keyboard(void) {
    if (virtual_hid_device_client_.is_connected()) {
      if (mode_ == mode::grabbing) {
        pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
        if (auto k = types::get_keyboard_type(profile_.get_virtual_hid_keyboard().get_keyboard_type())) {
          properties.keyboard_type = *k;
        }
        auto caps_lock_delay_milliseconds = profile_.get_virtual_hid_keyboard().get_caps_lock_delay_milliseconds();
        properties.caps_lock_delay_milliseconds = pqrs::karabiner_virtual_hid_device::milliseconds(caps_lock_delay_milliseconds);

        virtual_hid_device_client_.initialize_virtual_hid_keyboard(properties);
        return;
      }

      virtual_hid_device_client_.terminate_virtual_hid_keyboard();
    }
  }

  void update_virtual_hid_pointing(void) {
    if (virtual_hid_device_client_.is_connected()) {
      if (mode_ == mode::grabbing) {
        if (is_pointing_device_grabbed() ||
            manipulator_managers_connector_.needs_virtual_hid_pointing()) {
          virtual_hid_device_client_.initialize_virtual_hid_pointing();
          return;
        }
      }

      virtual_hid_device_client_.terminate_virtual_hid_pointing();
    }
  }

  bool is_ignored_device(const human_interface_device& device) const {
    if (core_configuration_) {
      return core_configuration_->get_selected_profile().get_device_ignore(device.get_connected_device().get_identifiers());
    }

    return false;
  }

  bool get_disable_built_in_keyboard_if_exists(const human_interface_device& device) const {
    if (core_configuration_) {
      return core_configuration_->get_selected_profile().get_device_disable_built_in_keyboard_if_exists(device.get_connected_device().get_identifiers());
    }
    return false;
  }

  bool need_to_disable_built_in_keyboard(void) const {
    for (const auto& it : hids_) {
      if (get_disable_built_in_keyboard_if_exists(*(it.second))) {
        return true;
      }
    }
    return false;
  }

  void enable_devices(void) {
    for (const auto& it : hids_) {
      if ((it.second)->is_built_in_keyboard() && need_to_disable_built_in_keyboard()) {
        (it.second)->disable();
      } else {
        (it.second)->enable();
      }
    }
  }

  void output_devices_json(void) const {
    connected_devices connected_devices;
    for (const auto& it : hids_) {
      if ((it.second)->is_pqrs_device()) {
        continue;
      }

      connected_devices.push_back_device(it.second->get_connected_device());
    }

    auto file_path = constants::get_devices_json_file_path();
    if (connected_devices.save_to_file(file_path)) {
      chmod(file_path, 0644);
    }
  }

  void set_profile(const core_configuration::profile& profile) {
    profile_ = profile;

    update_simple_modifications_manipulators();
    update_complex_modifications_manipulators();
    update_fn_function_keys_manipulators();

    update_virtual_hid_keyboard();
    update_virtual_hid_pointing();
  }

  void update_simple_modifications_manipulators(void) {
    simple_modifications_manipulator_manager_.invalidate_manipulators();

    for (const auto& device : profile_.get_devices()) {
      for (const auto& pair : device.get_simple_modifications().get_pairs()) {
        if (auto m = make_simple_modifications_manipulator(pair)) {
          auto c = make_device_if_condition(device);
          m->push_back_condition(c);
          simple_modifications_manipulator_manager_.push_back_manipulator(m);
        }
      }
    }

    for (const auto& pair : profile_.get_simple_modifications().get_pairs()) {
      if (auto m = make_simple_modifications_manipulator(pair)) {
        simple_modifications_manipulator_manager_.push_back_manipulator(m);
      }
    }
  }

  std::shared_ptr<manipulator::details::conditions::base> make_device_if_condition(const core_configuration::profile::device& device) const {
    return std::make_shared<manipulator::details::conditions::device>(device.get_identifiers());
  }

  std::shared_ptr<manipulator::details::base> make_simple_modifications_manipulator(const std::pair<core_configuration::profile::simple_modifications::definition, core_configuration::profile::simple_modifications::definition>& pair) const {
    if (pair.first.valid() && pair.second.valid()) {
      auto from_json = pair.first.to_json();
      from_json["modifiers"]["optional"] = "any";

      auto to_json = pair.second.to_json();

      return std::make_shared<manipulator::details::basic>(manipulator::details::from_event_definition(from_json),
                                                           manipulator::details::to_event_definition(to_json));
    }
    return nullptr;
  }

  void update_complex_modifications_manipulators(void) {
    complex_modifications_manipulator_manager_.invalidate_manipulators();

    for (const auto& rule : profile_.get_complex_modifications().get_rules()) {
      for (const auto& manipulator : rule.get_manipulators()) {
        auto m = krbn::manipulator::manipulator_factory::make_manipulator(manipulator.get_json(), manipulator.get_parameters());
        for (const auto& c : manipulator.get_conditions()) {
          m->push_back_condition(krbn::manipulator::manipulator_factory::make_condition(c.get_json()));
        }
        complex_modifications_manipulator_manager_.push_back_manipulator(m);
      }
    }
  }

  void update_fn_function_keys_manipulators(void) {
    fn_function_keys_manipulator_manager_.invalidate_manipulators();

    std::unordered_set<manipulator::details::event_definition::modifier> from_mandatory_modifiers;
    std::unordered_set<manipulator::details::event_definition::modifier> from_optional_modifiers({
        manipulator::details::event_definition::modifier::any,
    });
    std::unordered_set<manipulator::details::event_definition::modifier> to_modifiers;

    if (system_preferences_values_.get_keyboard_fn_state()) {
      // f1 -> f1
      // fn+f1 -> display_brightness_decrement

      from_mandatory_modifiers.insert(manipulator::details::event_definition::modifier::fn);
      to_modifiers.insert(manipulator::details::event_definition::modifier::fn);

    } else {
      // f1 -> display_brightness_decrement
      // fn+f1 -> f1

      // fn+f1 ... fn+f12 -> f1 .. f12

      for (const auto& key_code : std::vector<key_code>({
               key_code::f1,
               key_code::f2,
               key_code::f3,
               key_code::f4,
               key_code::f5,
               key_code::f6,
               key_code::f7,
               key_code::f8,
               key_code::f9,
               key_code::f10,
               key_code::f11,
               key_code::f12,
           })) {
        auto manipulator = std::make_shared<manipulator::details::basic>(manipulator::details::from_event_definition(
                                                                             key_code,
                                                                             {
                                                                                 manipulator::details::event_definition::modifier::fn,
                                                                             },
                                                                             {
                                                                                 manipulator::details::event_definition::modifier::any,
                                                                             }),
                                                                         manipulator::details::to_event_definition(
                                                                             key_code,
                                                                             {
                                                                                 manipulator::details::event_definition::modifier::fn,
                                                                             }));
        fn_function_keys_manipulator_manager_.push_back_manipulator(std::shared_ptr<manipulator::details::base>(manipulator));
      }
    }

    // from_modifiers+f1 -> display_brightness_decrement ...

    for (const auto& device : profile_.get_devices()) {
      for (const auto& pair : device.get_fn_function_keys().get_pairs()) {
        if (auto m = make_fn_function_keys_manipulator(pair,
                                                       from_mandatory_modifiers,
                                                       from_optional_modifiers,
                                                       to_modifiers)) {
          auto c = make_device_if_condition(device);
          m->push_back_condition(c);
          fn_function_keys_manipulator_manager_.push_back_manipulator(m);
        }
      }
    }

    for (const auto& pair : profile_.get_fn_function_keys().get_pairs()) {
      if (auto m = make_fn_function_keys_manipulator(pair,
                                                     from_mandatory_modifiers,
                                                     from_optional_modifiers,
                                                     to_modifiers)) {
        fn_function_keys_manipulator_manager_.push_back_manipulator(m);
      }
    }

    // fn+return_or_enter -> keypad_enter ...

    auto pairs = std::vector<std::pair<key_code, key_code>>({
        std::make_pair(key_code::return_or_enter, key_code::keypad_enter),
        std::make_pair(key_code::delete_or_backspace, key_code::delete_forward),
        std::make_pair(key_code::right_arrow, key_code::end),
        std::make_pair(key_code::left_arrow, key_code::home),
        std::make_pair(key_code::down_arrow, key_code::page_down),
        std::make_pair(key_code::up_arrow, key_code::page_up),
    });
    for (const auto& p : pairs) {
      auto manipulator = std::make_shared<manipulator::details::basic>(manipulator::details::from_event_definition(
                                                                           p.first,
                                                                           {
                                                                               manipulator::details::event_definition::modifier::fn,
                                                                           },
                                                                           {
                                                                               manipulator::details::event_definition::modifier::any,
                                                                           }),
                                                                       manipulator::details::to_event_definition(
                                                                           p.second,
                                                                           {
                                                                               manipulator::details::event_definition::modifier::fn,
                                                                           }));
      fn_function_keys_manipulator_manager_.push_back_manipulator(std::shared_ptr<manipulator::details::base>(manipulator));
    }
  }

  std::shared_ptr<manipulator::details::base> make_fn_function_keys_manipulator(const std::pair<core_configuration::profile::simple_modifications::definition, core_configuration::profile::simple_modifications::definition>& pair,
                                                                                const std::unordered_set<manipulator::details::event_definition::modifier>& from_mandatory_modifiers,
                                                                                const std::unordered_set<manipulator::details::event_definition::modifier>& from_optional_modifiers,
                                                                                const std::unordered_set<manipulator::details::event_definition::modifier>& to_modifiers) {
    if (pair.first.valid() && pair.second.valid()) {
      if (auto from_event = types::make_key_code(pair.first.get_value())) {
        if (auto to_event = types::make_key_code(pair.second.get_value())) {
          return std::make_shared<manipulator::details::basic>(manipulator::details::from_event_definition(
                                                                   *from_event,
                                                                   from_mandatory_modifiers,
                                                                   from_optional_modifiers),
                                                               manipulator::details::to_event_definition(
                                                                   *to_event,
                                                                   to_modifiers));
        }
      }
    }
    return nullptr;
  }

  virtual_hid_device_client virtual_hid_device_client_;
  boost::signals2::connection client_connected_connection;
  boost::signals2::connection client_disconnected_connection;

  std::unique_ptr<configuration_monitor> configuration_monitor_;
  std::shared_ptr<core_configuration> core_configuration_;

  std::unique_ptr<event_tap_manager> event_tap_manager_;
  IOHIDManagerRef _Nullable manager_;

  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;

  core_configuration::profile profile_;
  system_preferences::values system_preferences_values_;

  manipulator::manipulator_managers_connector manipulator_managers_connector_;
  boost::signals2::connection input_event_arrived_connection;

  event_queue merged_input_event_queue_;

  manipulator::manipulator_manager simple_modifications_manipulator_manager_;
  event_queue simple_modifications_applied_event_queue_;

  manipulator::manipulator_manager complex_modifications_manipulator_manager_;
  event_queue complex_modifications_applied_event_queue_;

  manipulator::manipulator_manager fn_function_keys_manipulator_manager_;
  event_queue fn_function_keys_applied_event_queue_;

  std::shared_ptr<manipulator::details::post_event_to_virtual_devices> post_event_to_virtual_devices_manipulator_;
  manipulator::manipulator_manager post_event_to_virtual_devices_manipulator_manager_;
  event_queue posted_event_queue_;

  std::unique_ptr<gcd_utility::main_queue_timer> led_monitor_timer_;

  mode mode_;

  spdlog_utility::log_reducer is_grabbable_callback_log_reducer_;

  bool suspended_;
};
} // namespace krbn
