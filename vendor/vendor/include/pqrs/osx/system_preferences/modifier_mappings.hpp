#pragma once

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/number.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/kern_return.hpp>
#include <vector>

namespace pqrs {
namespace osx {
namespace system_preferences {

struct modifier_mapping {
  pqrs::hid::usage_pair src;
  pqrs::hid::usage_pair dst;
};

inline std::vector<modifier_mapping> get_modifier_mappings(CFDictionaryRef _Nonnull matching_dictionary) {
  std::vector<modifier_mapping> result;

  CFRetain(matching_dictionary);
  if (auto service = IOServiceGetMatchingService(type_safe::get(iokit_mach_port::null),
                                                 matching_dictionary)) {
    if (auto cf_hid_event_service_properties = IORegistryEntryCreateCFProperty(service,
                                                                               CFSTR("HIDEventServiceProperties"),
                                                                               kCFAllocatorDefault,
                                                                               0)) {
      if (CFDictionaryGetTypeID() == CFGetTypeID(cf_hid_event_service_properties)) {
        auto hid_event_service_properties = static_cast<CFDictionaryRef>(cf_hid_event_service_properties);
        if (auto cf_pairs = CFDictionaryGetValue(hid_event_service_properties, CFSTR("HIDKeyboardModifierMappingPairs"))) {
          if (CFArrayGetTypeID() == CFGetTypeID(cf_pairs)) {
            auto pairs = static_cast<CFArrayRef>(cf_pairs);
            auto size = CFArrayGetCount(pairs);
            for (CFIndex i = 0; i < size; ++i) {
              if (auto pair = cf::get_cf_array_value<CFDictionaryRef>(pairs, i)) {
                if (CFDictionaryGetTypeID() == CFGetTypeID(pair)) {
                  if (auto src = pqrs::cf::make_number<int64_t>(CFDictionaryGetValue(pair, CFSTR("HIDKeyboardModifierMappingSrc")))) {
                    if (auto dst = pqrs::cf::make_number<int64_t>(CFDictionaryGetValue(pair, CFSTR("HIDKeyboardModifierMappingDst")))) {
                      auto s = std::bit_cast<uint64_t>(*src);
                      auto d = std::bit_cast<uint64_t>(*dst);

                      if (s == d) {
                        continue;
                      }

                      auto src_usage_page = pqrs::hid::usage_page::value_t((s >> 32) & 0xffff);
                      auto src_usage = pqrs::hid::usage::value_t(s & 0xffff);
                      auto dst_usage_page = pqrs::hid::usage_page::value_t((d >> 32) & 0xffff);
                      auto dst_usage = pqrs::hid::usage::value_t(d & 0xffff);

                      // When the Globe Key is modified, two entries like the following are added:
                      // src: page=0x00FF usage=0x0003   dst: page=0x0007 usage=0x0029
                      // src: page=0xFF01 usage=0x0003   dst: page=0x0007 usage=0x0029
                      //
                      // This seems to be because the fn key is defined under both
                      // `apple_vendor_keyboard::function` and `apple_vendor_top_case::keyboard_fn`.
                      // Among these, the `page=0xFF01 usage=0x0003` entry is not kept in sync correctly.
                      // Even if the Globe Key setting is reverted, this entry is not removed and remains as leftover data.
                      //
                      // Therefore, we need to reference only the `page=0x00FF usage=0x0003` entry for the Globe Key setting,
                      // and ignore the `page=0xFF01 usage=0x0003` one.

                      if (src_usage_page == hid::usage_page::apple_vendor_keyboard &&
                          src_usage == hid::usage::apple_vendor_keyboard::function) {
                        continue;
                      }

                      result.push_back(modifier_mapping{
                          .src = pqrs::hid::usage_pair(src_usage_page,
                                                       src_usage),
                          .dst = pqrs::hid::usage_pair(dst_usage_page,
                                                       dst_usage),
                      });
                    }
                  }
                }
              }
            }
          }
        }
      }

      CFRelease(cf_hid_event_service_properties);
    }

    IOObjectRelease(service);
  }

  return result;
}

} // namespace system_preferences
} // namespace osx
} // namespace pqrs
