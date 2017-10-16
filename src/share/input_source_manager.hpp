#pragma once

#include "cf_utility.hpp"
#include "input_source_utility.hpp"
#include "logger.hpp"
#include <Carbon/Carbon.h>

namespace krbn {
class input_source_manager final {
public:
  input_source_manager(void) {
    logger::get_logger().info("input_source_manager initialize");

    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                    this,
                                    static_enabled_input_sources_changed_callback,
                                    kTISNotifyEnabledKeyboardInputSourcesChanged,
                                    nullptr,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);

    enabled_input_sources_changed_callback();
  }

  ~input_source_manager(void) {
    logger::get_logger().info("input_source_manager terminate");

    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                       this,
                                       kTISNotifyEnabledKeyboardInputSourcesChanged,
                                       nullptr);
  }

private:
  class entry final {
  private:
    std::string language;
    std::string input_source_id;
    std::string input_mode_id;
  };

  static void static_enabled_input_sources_changed_callback(CFNotificationCenterRef center,
                                                            void* observer,
                                                            CFStringRef notification_name,
                                                            const void* observed_object,
                                                            CFDictionaryRef user_info) {
    auto self = static_cast<input_source_manager*>(observer);
    if (self) {
      self->enabled_input_sources_changed_callback();
    }
  }

  void enabled_input_sources_changed_callback(void) {
    if (auto properties = cf_utility::create_cfmutabledictionary()) {
      CFDictionarySetValue(properties, kTISPropertyInputSourceIsSelectCapable, kCFBooleanTrue);
      CFDictionarySetValue(properties, kTISPropertyInputSourceCategory, kTISCategoryKeyboardInputSource);

      if (auto input_sources = TISCreateInputSourceList(properties, false)) {
        for (CFIndex i = 0;; ++i) {
          auto s = cf_utility::get_value<TISInputSourceRef>(input_sources, i);
          if (!s) {
            break;
          }

          if (auto input_source_id = input_source_utility::get_input_source_id(s)) {
            logger::get_logger().info("input_source_id: {0}", *input_source_id);
          }
        }

        CFRelease(input_sources);
      }

      CFRelease(properties);
    }
  }
};
} // namespace krbn
