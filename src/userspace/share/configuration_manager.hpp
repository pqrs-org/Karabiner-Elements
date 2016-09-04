#pragma once

#include "configuration_core.hpp"
#include <CoreServices/CoreServices.h>
#include <memory>

class configuration_manager final {
public:
  configuration_manager(spdlog::logger& logger, const std::string& dot_karabiner_directory) : logger_(logger),
                                                                                              dot_karabiner_directory_(dot_karabiner_directory),
                                                                                              path_(nullptr),
                                                                                              paths_(nullptr),
                                                                                              stream_(nullptr) {
    // monitor ~/.karabiner.d
    path_ = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
                                            dot_karabiner_directory_.c_str(),
                                            kCFStringEncodingUTF8,
                                            kCFAllocatorDefault);
    paths_ = CFArrayCreate(kCFAllocatorDefault,
                           reinterpret_cast<const void**>(&path_),
                           1,
                           nullptr);

    FSEventStreamContext context{0};
    context.info = this;

    stream_ = FSEventStreamCreate(kCFAllocatorDefault,
                                  static_stream_callback,
                                  &context,
                                  paths_,
                                  kFSEventStreamEventIdSinceNow,
                                  0.1, // 100 ms
                                  kFSEventStreamCreateFlagIgnoreSelf);
    if (!stream_) {
      logger_.error("FSEventStreamCreate error @ {0}", __PRETTY_FUNCTION__);
    } else {
      FSEventStreamScheduleWithRunLoop(stream_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
      if (!FSEventStreamStart(stream_)) {
        logger_.error("FSEventStreamStart error @ {0}", __PRETTY_FUNCTION__);
      }
    }
  }

  ~configuration_manager(void) {
    if (paths_) {
      CFRelease(paths_);
      paths_ = nullptr;
    }
    if (path_) {
      CFRelease(path_);
      path_ = nullptr;
    }
  }

private:
  static void static_stream_callback(ConstFSEventStreamRef stream,
                                     void* client_callback_info,
                                     size_t num_events,
                                     void* event_paths,
                                     const FSEventStreamEventFlags event_flags[],
                                     const FSEventStreamEventId event_ids[]) {
    auto self = reinterpret_cast<configuration_manager*>(client_callback_info);
    if (self) {
      self->stream_callback();
    }
  }

  void stream_callback(void) {
    logger_.info("stream_callback");
  }

  spdlog::logger& logger_;
  std::string dot_karabiner_directory_;

  CFStringRef path_;
  CFArrayRef paths_;
  FSEventStreamRef stream_;

  std::unique_ptr<configuration_core> configuration_core_;
};
