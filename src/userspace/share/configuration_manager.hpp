#pragma once

#include "configuration_core.hpp"
#include <CoreServices/CoreServices.h>
#include <memory>

class configuration_manager final {
public:
  configuration_manager(spdlog::logger& logger, const std::string& configuration_directory) : logger_(logger),
                                                                                              configuration_directory_(configuration_directory),
                                                                                              path_(nullptr),
                                                                                              paths_(nullptr),
                                                                                              stream_(nullptr) {
    // monitor ~/.karabiner.d
    path_ = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
                                            configuration_directory_.c_str(),
                                            kCFStringEncodingUTF8,
                                            kCFAllocatorDefault);
    paths_ = CFArrayCreate(kCFAllocatorDefault,
                           reinterpret_cast<const void**>(&path_),
                           1,
                           nullptr);

    register_stream();
  }

  ~configuration_manager(void) {
    unregister_stream();

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
  void register_stream(void) {
    FSEventStreamContext context{0};
    context.info = this;

    // kFSEventStreamCreateFlagWatchRoot and kFSEventStreamCreateFlagFileEvents are required in the following case.
    //
    // $ mkdir ~/.karabiner.d/configuration
    // $ touch ~/.karabiner.d/configuration/xxx.json
    // $ mv ~/.karabiner.d/configuration ~/.karabiner.d/configuration.back
    // $ ln -s ~/file-synchronisation-service/karabiner.d/configuration ~/.karabiner.d/
    // $ touch ~/.karabiner.d/configuration/xxx.json
    auto flags = FSEventStreamCreateFlags(0);
    flags |= kFSEventStreamCreateFlagWatchRoot;
    flags |= kFSEventStreamCreateFlagIgnoreSelf;
    flags |= kFSEventStreamCreateFlagFileEvents;

    stream_ = FSEventStreamCreate(kCFAllocatorDefault,
                                  static_stream_callback,
                                  &context,
                                  paths_,
                                  kFSEventStreamEventIdSinceNow,
                                  0.1, // 100 ms
                                  flags);
    if (!stream_) {
      logger_.error("FSEventStreamCreate error @ {0}", __PRETTY_FUNCTION__);
    } else {
      FSEventStreamScheduleWithRunLoop(stream_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
      if (!FSEventStreamStart(stream_)) {
        logger_.error("FSEventStreamStart error @ {0}", __PRETTY_FUNCTION__);
      }
    }
  }

  void unregister_stream(void) {
    if (stream_) {
      FSEventStreamStop(stream_);
      FSEventStreamInvalidate(stream_);
      FSEventStreamRelease(stream_);
      stream_ = nullptr;
    }
  }

  static void static_stream_callback(ConstFSEventStreamRef stream,
                                     void* client_callback_info,
                                     size_t num_events,
                                     void* event_paths,
                                     const FSEventStreamEventFlags event_flags[],
                                     const FSEventStreamEventId event_ids[]) {
    auto self = reinterpret_cast<configuration_manager*>(client_callback_info);
    if (self) {
      self->stream_callback(num_events,
                            static_cast<const char**>(event_paths),
                            event_flags,
                            event_ids);
    }
  }

  void stream_callback(size_t num_events,
                       const char* event_paths[],
                       const FSEventStreamEventFlags event_flags[],
                       const FSEventStreamEventId event_ids[]) {
    logger_.info("stream_callback num_events:{0}", num_events);
    for (size_t i = 0; i < num_events; ++i) {
      if (event_flags[i] & kFSEventStreamEventFlagRootChanged) {
        logger_.info("  root changed");
        // re-register stream
        unregister_stream();
        register_stream();
      } else {
        logger_.info("  file changed");
        logger_.info("  {0}", event_paths[i]);
      }
    }
  }

  spdlog::logger& logger_;
  std::string configuration_directory_;

  CFStringRef path_;
  CFArrayRef paths_;
  FSEventStreamRef stream_;

  std::unique_ptr<configuration_core> configuration_core_;
};
