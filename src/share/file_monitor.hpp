#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <utility>
#include <vector>

class file_monitor final {
public:
  // [
  //   {directory, [file, file, ...]}
  //   {directory, [file, file, ...]}
  //   ...
  // ]
  file_monitor(const std::vector<std::pair<std::string, std::vector<std::string>>>& targets) : directories_(nullptr) {
    directories_ = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

    for (const auto& target : targets) {
      auto directory = CFStringCreateWithCString(kCFAllocatorDefault,
                                                 target.first.c_str(),
                                                 kCFStringEncodingUTF8);
      CFArrayAppendValue(directories_, directory);
      CFRelease(directory);
    }
  }

  ~file_monitor(void) {
    if (directories_) {
      CFRelease(directories_);
      directories_ = nullptr;
    }
  }

private:
  CFMutableArrayRef directories_;
};
