#include "basic.hpp"
#include "device_grabber.hpp"
#include "types.hpp"
#include "logger.hpp"

using namespace krbn::manipulator::details;

std::pair<uint32_t, uint32_t> basic::get_vendor_product_id_by_device_id(krbn::device_id id) {
  uint32_t vid = 0;
  uint32_t pid = 0;
  
  auto hid = krbn::device_grabber::get_grabber()->get_hid_by_id(id);
  if (hid) {
    auto hid_ptr = (*hid).lock();
    
    vid = static_cast<uint32_t>(hid_ptr->get_vendor_id().value_or(krbn::vendor_id::zero));
    pid = static_cast<uint32_t>(hid_ptr->get_product_id().value_or(krbn::product_id::zero));
  }
  
  return std::make_pair(vid, pid);
}

bool basic::is_bypass_vendor_product_id_check() {
  bool not_to_check = this->vendor_id_ == 0 && this->product_id_ == 0;
  
  if (not_to_check) {
    krbn::logger::get_logger().debug("bypass vendor/product id check");
  }
  return not_to_check;
}

bool basic::is_vendor_product_id_matched(uint32_t vendor_id, uint32_t product_id) {
  bool is_matched = (vendor_id == this->vendor_id_ && product_id == this->product_id_);
  
  if (is_matched) {
    krbn::logger::get_logger().debug("vendor/product id matched");
  } else {
    krbn::logger::get_logger().debug("vendor/product id NOT matched: [{}, {}], [{}, {}]", vendor_id, product_id, this->vendor_id_, this->product_id_);
  }
  
  return is_matched;
}

#if 0
void basic::manipulate(event_queue::queued_event& front_input_event,
                       const event_queue& input_event_queue,
                       event_queue& output_event_queue,
                       uint64_t time_stamp) {
  
  uint32_t _device_id = static_cast<uint32_t>(front_input_event.get_device_id());
  uint32_t _key_code = static_cast<uint32_t>(*front_input_event.get_event().get_key_code());
  
  auto& logger = krbn::logger::get_logger();
  logger.info("in manipulate(): device_id: {0:#x}, key_code: {1:#x}", _device_id, _key_code);
  
  if (!front_input_event.get_valid()) {
    return;
  }
  
  bool is_target = false;
  
  if (auto key_code = front_input_event.get_event().get_key_code()) {
    if (from_.get_key_code() == key_code) {
      is_target = true;
    }
  }
  if (auto pointing_button = front_input_event.get_event().get_pointing_button()) {
    if (from_.get_pointing_button() == pointing_button) {
      is_target = true;
    }
  }
  
  auto vp_id = get_vendor_product_id_by_device_id(front_input_event.get_device_id());
  if (!is_bypass_vendor_product_id_check() && !is_vendor_product_id_matched(vp_id.first, vp_id.second)) {
    is_target = false;
  }
  
  /*
  auto hid = krbn::device_grabber::get_grabber()->get_hid_by_id(front_input_event.get_device_id());
  bool is_built_in_kb = hid && (*hid).lock()->is_built_in_keyboard();
  //logger.info("get_hid_by_id({}): {}", _device_id, is_built_in_kb);
  
  if (_key_code == kHIDUsage_KeyboardRightAlt && !is_built_in_kb) {
    is_target = false;
  }
  
  if (_key_code == kHIDUsage_KeyboardRightControl && is_built_in_kb) {
    is_target = false;
  }
  
  if ((_key_code == kHIDUsage_KeyboardLeftAlt || _key_code == kHIDUsage_KeyboardLeftGUI) && is_built_in_kb) {
    is_target = false;
  }
  */
  
  if (is_target) {
    std::unordered_set<modifier_flag> from_modifiers;
    
    if (front_input_event.get_event_type() == event_type::key_down) {
      
      if (!valid_) {
        is_target = false;
      }
      
      if (auto modifiers = from_.test_modifiers(output_event_queue.get_modifier_flag_manager())) {
        from_modifiers = *modifiers;
      } else {
        is_target = false;
      }
      
      if (is_target) {
        manipulated_original_events_.emplace_back(front_input_event.get_device_id(),
                                                  front_input_event.get_original_event(),
                                                  from_modifiers);
      }
      
    } else {
      // event_type::key_up
      
      // Check original_event in order to determine the correspond key_down is manipulated.
      
      auto it = std::find_if(std::begin(manipulated_original_events_),
                             std::end(manipulated_original_events_),
                             [&](const auto& manipulated_original_event) {
                               return manipulated_original_event.get_device_id() == front_input_event.get_device_id() &&
                               manipulated_original_event.get_original_event() == front_input_event.get_original_event();
                             });
      if (it != std::end(manipulated_original_events_)) {
        from_modifiers = it->get_from_modifiers();
        manipulated_original_events_.erase(it);
      } else {
        is_target = false;
      }
    }
    
    if (is_target) {
      front_input_event.set_valid(false);
      
      uint64_t time_stamp_delay = 0;
      bool persist_from_modifier_manipulation = false;
      
      // Release from_modifiers
      
      if (front_input_event.get_event_type() == event_type::key_down) {
        for (const auto& m : from_modifiers) {
          if (auto key_code = types::get_key_code(m)) {
            event_queue::queued_event event(front_input_event.get_device_id(),
                                            front_input_event.get_time_stamp() + time_stamp_delay++,
                                            event_queue::queued_event::event(*key_code),
                                            event_type::key_up,
                                            front_input_event.get_original_event(),
                                            true);
            output_event_queue.push_back_event(event);
          }
        }
      }
      
      
      
      // Send events
      krbn::logger::get_logger().info("Key is replaced");
      for (size_t i = 0; i < to_.size(); ++i) {
        if (auto event = to_[i].to_event()) {
          if (front_input_event.get_event_type() == event_type::key_down) {
            output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                  front_input_event.get_time_stamp() + time_stamp_delay++,
                                                  *event,
                                                  event_type::key_down,
                                                  front_input_event.get_original_event());
            
            if (i != to_.size() - 1) {
              output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                    front_input_event.get_time_stamp() + time_stamp_delay++,
                                                    *event,
                                                    event_type::key_up,
                                                    front_input_event.get_original_event());
              
              if (auto key_code = event->get_key_code()) {
                if (types::get_modifier_flag(*key_code) != modifier_flag::zero) {
                  persist_from_modifier_manipulation = true;
                }
              }
            }
            
          } else {
            // event_type::key_up
            
            if (i == to_.size() - 1) {
              output_event_queue.emplace_back_event(front_input_event.get_device_id(),
                                                    front_input_event.get_time_stamp() + time_stamp_delay++,
                                                    *event,
                                                    event_type::key_up,
                                                    front_input_event.get_original_event());
            }
          }
        }
      }
      
      // Restore from_modifiers
      
      if ((front_input_event.get_event_type() == event_type::key_down) ||
          (front_input_event.get_event_type() == event_type::key_up && persist_from_modifier_manipulation)) {
        for (const auto& m : from_modifiers) {
          if (auto key_code = types::get_key_code(m)) {
            event_queue::queued_event event(front_input_event.get_device_id(),
                                            front_input_event.get_time_stamp() + time_stamp_delay++,
                                            event_queue::queued_event::event(*key_code),
                                            event_type::key_down,
                                            front_input_event.get_original_event(),
                                            !persist_from_modifier_manipulation);
            output_event_queue.push_back_event(event);
          }
        }
      }
      
      output_event_queue.increase_time_stamp_delay(time_stamp_delay - 1);
    }
  }
}
#endif
