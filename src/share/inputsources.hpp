#pragma once

#include "boost_defs.hpp"
#include <Carbon/Carbon.h>

namespace krbn {
class inputsources final {
public:
  static void select(const std::string& id) {
    cf_string new_inputsource_id(id);
    auto found = false;
    
    auto inputsources = TISCreateInputSourceList(NULL, false);
    auto count = CFArrayGetCount(inputsources);
    
    for (auto i = 0; i < count; ++i) {
      auto inputsource = (TISInputSourceRef)CFArrayGetValueAtIndex(inputsources, i);
      auto inputsource_id = (CFStringRef)TISGetInputSourceProperty(inputsource, kTISPropertyInputSourceID);
      
      if (inputsource_id == new_inputsource_id) {
        OSStatus status = TISSelectInputSource(inputsource);
        if (status != noErr)
          logger::get_logger().error("inputsources::select: {0}, failed: {1}", id, status);
        found = true;
        break;
      }
    }
    
    if (!found) {
      std::stringstream ss;
      for (auto i = 0; i < count; ++i) {
        auto inputsource = (TISInputSourceRef)CFArrayGetValueAtIndex(inputsources, i);
        auto inputsource_id = * cf_utility::to_string((CFStringRef)TISGetInputSourceProperty(inputsource, kTISPropertyInputSourceID));
        if (i > 0)
          ss << ", ";
        ss << '"' << inputsource_id << '"';
      }
      logger::get_logger().error("inputsources::select: {0}, no source found among available [{1}]", id, ss.str());
    }

    CFRelease(inputsources);
  }
};
} // namespace krbn
