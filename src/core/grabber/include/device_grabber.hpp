#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "event_manipulator.hpp"
#include "event_tap_manager.hpp"
#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include "manipulator.hpp"
#include "spdlog_utility.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>

class device_grabber final {
public:
  device_grabber(const device_grabber&) = delete;

  device_grabber(virtual_hid_device_client& virtual_hid_device_client,
                 manipulator::event_manipulator& event_manipulator) : virtual_hid_device_client_(virtual_hid_device_client),
                                                                      event_manipulator_(event_manipulator),
                                                                      mode_(mode::observing),
                                                                      is_grabbable_callback_log_reducer_(logger::get_logger()),
                                                                      suspended_(false) {
    virtual_hid_device_client_disconnected_connection = virtual_hid_device_client_.client_disconnected.connect(
        boost::bind(&device_grabber::virtual_hid_device_client_disconnected_callback, this));

    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      logger::get_logger().error("{0}: failed to IOHIDManagerCreate", __PRETTY_FUNCTION__);
      return;
    }

    auto device_matching_dictionaries = iokit_utility::create_device_matching_dictionaries({
        std::make_pair(kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard),
        // std::make_pair(kHIDPage_Consumer, kHIDUsage_Csmr_ConsumerControl),
        // std::make_pair(kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse),
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

      if (manager_) {
        IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
        CFRelease(manager_);
        manager_ = nullptr;
      }

      virtual_hid_device_client_disconnected_connection.disconnect();
    });
  }

  void start_grabbing(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      mode_ = mode::grabbing;

      event_manipulator_.reset();

      // We should call CGEventTapCreate after user is logged in.
      // So, we create event_tap_manager here.
      event_tap_manager_ = std::make_unique<event_tap_manager>(std::bind(&device_grabber::caps_lock_state_changed_callback, this, std::placeholders::_1));
    });
  }

  void stop_grabbing(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      ungrab_devices();

      mode_ = mode::observing;

      event_manipulator_.reset();

      event_tap_manager_ = nullptr;
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

  void clear_device_configurations(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      device_configurations_.clear();
    });
  }

  void add_device_configuration(const krbn::device_identifiers_struct& device_identifiers_struct,
                                const krbn::device_configuration_struct& device_configuration_struct) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      device_configurations_.push_back(std::make_pair(device_identifiers_struct, device_configuration_struct));
    });
  }

  void complete_device_configurations(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      for (auto&& it : hids_) {
        (it.second)->set_disable_built_in_keyboard_if_exists(get_disable_built_in_keyboard_if_exists(*(it.second)));
      }

      grab_devices();
      output_devices_json();
    });
  }

private:
  enum class mode {
    observing,
    grabbing,
  };

  void virtual_hid_device_client_disconnected_callback(void) {
    stop_grabbing();
  }

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

    iokit_utility::log_matching_device(logger::get_logger(), device);

    auto dev = std::make_unique<human_interface_device>(logger::get_logger(), device);
    dev->set_disable_built_in_keyboard_if_exists(get_disable_built_in_keyboard_if_exists(*dev));
    dev->set_is_grabbable_callback(std::bind(&device_grabber::is_grabbable_callback, this, std::placeholders::_1));
    dev->set_grabbed_callback(std::bind(&device_grabber::grabbed_callback, this, std::placeholders::_1));
    dev->set_ungrabbed_callback(std::bind(&device_grabber::ungrabbed_callback, this, std::placeholders::_1));
    dev->set_disabled_callback(std::bind(&device_grabber::disabled_callback, this, std::placeholders::_1));
    dev->set_value_callback(std::bind(&device_grabber::value_callback,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      std::placeholders::_4,
                                      std::placeholders::_5,
                                      std::placeholders::_6));
    dev->observe();

    hids_[device] = std::move(dev);

    output_devices_json();

    if (is_pointing_device_connected()) {
      event_manipulator_.initialize_virtual_hid_pointing();
    } else {
      event_manipulator_.terminate_virtual_hid_pointing();
    }

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

    iokit_utility::log_removal_device(logger::get_logger(), device);

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        hids_.erase(it);
      }
    }

    output_devices_json();

    if (is_pointing_device_connected()) {
      event_manipulator_.initialize_virtual_hid_pointing();
    } else {
      event_manipulator_.terminate_virtual_hid_pointing();
    }

    event_manipulator_.stop_key_repeat();

    // ----------------------------------------
    if (mode_ == mode::grabbing) {
      enable_devices();
    }
  }

  void value_callback(human_interface_device& device,
                      IOHIDValueRef _Nonnull value,
                      IOHIDElementRef _Nonnull element,
                      uint32_t usage_page,
                      uint32_t usage,
                      CFIndex integer_value) {
    if (!device.is_grabbed()) {
      return;
    }

    auto device_registry_entry_id = manipulator::device_registry_entry_id(device.get_registry_entry_id());
    auto timestamp = IOHIDValueGetTimeStamp(value);

    if (auto key_code = krbn::types::get_key_code(usage_page, usage)) {
      bool pressed = integer_value;
      event_manipulator_.handle_keyboard_event(device_registry_entry_id,
                                               timestamp,
                                               *key_code,
                                               pressed);

    } else if (auto pointing_button = krbn::types::get_pointing_button(usage_page, usage)) {
      event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                               timestamp,
                                               krbn::pointing_event::button,
                                               *pointing_button,
                                               integer_value);

    } else {
      switch (usage_page) {
      case kHIDPage_GenericDesktop:
        if (usage == kHIDUsage_GD_X) {
          event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                                   timestamp,
                                                   krbn::pointing_event::x,
                                                   boost::none,
                                                   integer_value);
        }
        if (usage == kHIDUsage_GD_Y) {
          event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                                   timestamp,
                                                   krbn::pointing_event::y,
                                                   boost::none,
                                                   integer_value);
        }
        if (usage == kHIDUsage_GD_Wheel) {
          event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                                   timestamp,
                                                   krbn::pointing_event::vertical_wheel,
                                                   boost::none,
                                                   integer_value);
        }
        break;

      case kHIDPage_Consumer:
        if (usage == kHIDUsage_Csmr_ACPan) {
          event_manipulator_.handle_pointing_event(device_registry_entry_id,
                                                   timestamp,
                                                   krbn::pointing_event::horizontal_wheel,
                                                   boost::none,
                                                   integer_value);
        }
        break;

      default:
        break;
      }
    }

    // reset modifier_flags state if all keys are released.
    if (get_all_devices_pressed_keys_count() == 0) {
      event_manipulator_.reset_modifier_flag_state();
      event_manipulator_.reset_pointing_button_state();
      event_manipulator_.stop_key_repeat();
    }
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

    auto ready_state = event_manipulator_.is_ready();
    if (ready_state != manipulator::event_manipulator::ready_state::ready) {
      std::string message = "event_manipulator_ is not ready. ";
      switch (ready_state) {
      case manipulator::event_manipulator::ready_state::ready:
        break;
      case manipulator::event_manipulator::ready_state::virtual_hid_device_client_is_not_ready:
        message += "(virtual_hid_device_client is not ready) ";
        break;
      case manipulator::event_manipulator::ready_state::virtual_hid_keyboard_is_not_ready:
        message += "(virtual_hid_keyboard is not ready) ";
        break;
      }
      message += "Please wait for a while.";
      is_grabbable_callback_log_reducer_.warn(message);
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }
    return human_interface_device::grabbable_state::grabbable;
  }

  void grabbed_callback(human_interface_device& device) {
    // set keyboard led
    if (event_tap_manager_) {
      caps_lock_state_changed_callback(event_tap_manager_->get_caps_lock_state());
    }
  }

  void ungrabbed_callback(human_interface_device& device) {
    // stop key repeat
    event_manipulator_.stop_key_repeat();
  }

  void disabled_callback(human_interface_device& device) {
    // stop key repeat
    event_manipulator_.stop_key_repeat();
  }

  void caps_lock_state_changed_callback(bool caps_lock_state) {
    event_manipulator_.set_caps_lock_state(caps_lock_state);

    // Update LED.
    for (const auto& it : hids_) {
      if ((it.second)->is_grabbed()) {
        (it.second)->set_caps_lock_led_state(caps_lock_state ? krbn::led_state::on : krbn::led_state::off);
      }
    }
  }

  size_t get_all_devices_pressed_keys_count(void) {
    size_t total = 0;
    for (const auto& it : hids_) {
      total += (it.second)->get_pressed_keys_count();
    }
    return total;
  }

  bool is_keyboard_connected(void) {
    for (const auto& it : hids_) {
      if ((it.second)->is_keyboard()) {
        return true;
      }
    }
    return false;
  }

  bool is_pointing_device_connected(void) {
    for (const auto& it : hids_) {
      if ((it.second)->is_pointing_device()) {
        return true;
      }
    }
    return false;
  }

  boost::optional<const krbn::device_configuration_struct&> find_device_configuration_struct(const human_interface_device& device) {
    if (auto vendor_id = device.get_vendor_id()) {
      if (auto product_id = device.get_product_id()) {
        bool is_keyboard = device.is_keyboard();
        bool is_pointing_device = device.is_pointing_device();

        for (const auto& d : device_configurations_) {
          if (d.first.vendor_id == *vendor_id &&
              d.first.product_id == *product_id &&
              d.first.is_keyboard == is_keyboard &&
              d.first.is_pointing_device == is_pointing_device) {
            return d.second;
          }
        }
      }
    }
    return boost::none;
  }

  bool is_ignored_device(const human_interface_device& device) {
    if (auto s = find_device_configuration_struct(device)) {
      return s->ignore;
    }

    if (device.is_pointing_device()) {
      return true;
    }

    if (auto vendor_id = device.get_vendor_id()) {
      if (auto product_id = device.get_product_id()) {
        // Touch Bar on MacBook Pro 2016
        if (*vendor_id == krbn::vendor_id(0x05ac) && *product_id == krbn::product_id(0x8600)) {
          return true;
        }
      }
    }

    return false;
  }

  bool get_disable_built_in_keyboard_if_exists(const human_interface_device& device) {
    if (auto s = find_device_configuration_struct(device)) {
      return s->disable_built_in_keyboard_if_exists;
    }
    return false;
  }

  bool need_to_disable_built_in_keyboard(void) {
    for (const auto& it : hids_) {
      if ((it.second)->get_disable_built_in_keyboard_if_exists()) {
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

  void output_devices_json(void) {
    nlohmann::json json;
    json = nlohmann::json::array();

    for (const auto& it : hids_) {
      if ((it.second)->is_pqrs_device()) {
        continue;
      }

      nlohmann::json j({
          {"identifiers", {}},
          {"descriptions", {}},
      });
      if (auto vendor_id = (it.second)->get_vendor_id()) {
        j["identifiers"]["vendor_id"] = static_cast<uint32_t>(*vendor_id);
      } else {
        j["identifiers"]["vendor_id"] = 0;
      }
      if (auto product_id = (it.second)->get_product_id()) {
        j["identifiers"]["product_id"] = static_cast<uint32_t>(*product_id);
      } else {
        j["identifiers"]["product_id"] = 0;
      }
      j["identifiers"]["is_keyboard"] = (it.second)->is_keyboard();
      j["identifiers"]["is_pointing_device"] = (it.second)->is_pointing_device();
      if (auto manufacturer = (it.second)->get_manufacturer()) {
        j["descriptions"]["manufacturer"] = boost::trim_copy(*manufacturer);
      } else {
        j["descriptions"]["manufacturer"] = "";
      }
      if (auto product = (it.second)->get_product()) {
        j["descriptions"]["product"] = boost::trim_copy(*product);
      } else {
        j["descriptions"]["product"] = "";
      }
      j["ignore"] = is_ignored_device(*(it.second));
      j["is_built_in_keyboard"] = (it.second)->is_built_in_keyboard();

      if (!j.empty()) {
        json.push_back(j);
      }
    }

    std::ofstream stream(constants::get_devices_json_file_path());
    if (stream) {
      stream << std::setw(4) << json << std::endl;
      chmod(constants::get_devices_json_file_path(), 0644);
    }
  }

  virtual_hid_device_client& virtual_hid_device_client_;
  manipulator::event_manipulator& event_manipulator_;

  boost::signals2::connection virtual_hid_device_client_disconnected_connection;

  std::unique_ptr<event_tap_manager> event_tap_manager_;
  IOHIDManagerRef _Nullable manager_;

  std::vector<std::pair<krbn::device_identifiers_struct, krbn::device_configuration_struct>> device_configurations_;

  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;

  mode mode_;

  spdlog_utility::log_reducer is_grabbable_callback_log_reducer_;

  bool suspended_;
};
