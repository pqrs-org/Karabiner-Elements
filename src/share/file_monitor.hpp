#pragma once

#include "filesystem.hpp"
#include <CoreServices/CoreServices.h>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

class file_monitor final {
public:
  typedef std::function<void(const std::string& file_path)> callback;

  // [
  //   {directory, [file, file, ...]}
  //   {directory, [file, file, ...]}
  //   ...
  // ]
  file_monitor(spdlog::logger& logger,
               const std::vector<std::pair<std::string, std::vector<std::string>>>& targets,
               const callback& callback) : logger_(logger),
                                           callback_(callback),
                                           directories_(CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks)),
                                           stream_(nullptr) {
    if (directories_) {
      for (const auto& target : targets) {
        if (auto directory = CFStringCreateWithCString(kCFAllocatorDefault,
                                                       target.first.c_str(),
                                                       kCFStringEncodingUTF8)) {
          CFArrayAppendValue(directories_, directory);
          CFRelease(directory);

          for (const auto& file : target.second) {
            auto f = file;
            filesystem::normalize_file_path(f);
            files_.push_back(f);
          }
        }
      }
      register_stream();
    }
  }

  ~file_monitor(void) {
    unregister_stream();

    if (directories_) {
      CFRelease(directories_);
      directories_ = nullptr;
    }
  }

private:
  void register_stream(void) {
    FSEventStreamContext context{0};
    context.info = this;

    // kFSEventStreamCreateFlagWatchRoot and kFSEventStreamCreateFlagFileEvents are required in the following case.
    // (When directory == ~/.config/karabiner-elements, file == ~/.config/karabiner-elements/xxx.json)
    //
    // $ mkdir ~/.config/karabiner-elements
    // $ touch ~/.config/karabiner-elements/xxx.json
    // $ mv ~/.config/karabiner-elements ~/.config/karabiner-elements.back
    // $ ln -s ~/file-synchronisation-service/karabiner.d/configuration ~/.karabiner.d/
    // $ touch ~/.config/karabiner-elements/xxx.json

    auto flags = FSEventStreamCreateFlags(0);
    flags |= kFSEventStreamCreateFlagWatchRoot;
    flags |= kFSEventStreamCreateFlagIgnoreSelf;
    flags |= kFSEventStreamCreateFlagFileEvents;

    stream_ = FSEventStreamCreate(kCFAllocatorDefault,
                                  static_stream_callback,
                                  &context,
                                  directories_,
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
    auto self = reinterpret_cast<file_monitor*>(client_callback_info);
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
    for (size_t i = 0; i < num_events; ++i) {
      if (event_flags[i] & kFSEventStreamEventFlagRootChanged) {
        logger_.info("the configuration directory is updated.");
        // re-register stream
        unregister_stream();
        register_stream();

      } else {
        std::string path = event_paths[i];
        filesystem::normalize_file_path(path);

        for (const auto& file : files_) {
          if (file == path) {
            if (callback_) {
              callback_(path);
            }
          }
        }
      }
    }
  }

  spdlog::logger& logger_;
  callback callback_;

  CFMutableArrayRef directories_;
  std::vector<std::string> files_;
  FSEventStreamRef stream_;
};
