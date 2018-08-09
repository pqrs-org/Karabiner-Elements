#pragma once

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include <CoreServices/CoreServices.h>
#include <boost/signals2.hpp>
#include <utility>
#include <vector>

namespace krbn {
class file_monitor final {
public:
  // Signals

  boost::signals2::signal<void(const std::string& file_path)> file_changed;

  // Methods

  file_monitor(const std::vector<std::string>& files) : directories_(cf_utility::create_cfmutablearray()),
                                                        stream_(nullptr) {
    run_loop_thread_ = std::make_unique<cf_utility::run_loop_thread>();

    std::vector<std::string> directories;
    for (const auto& f : files) {
      auto directory = filesystem::dirname(f);
      if (std::none_of(std::begin(directories),
                       std::end(directories),
                       [&](auto&& d) {
                         return directory == d;
                       })) {
        directories.push_back(directory);
      }

      // Do not apply `realpath` here since `realpath(f)` will be failed if `f` does not exist.
      files_.push_back(f);
    }

    for (const auto& d : directories) {
      if (auto directory = cf_utility::create_cfstring(d)) {
        if (directories_) {
          CFArrayAppendValue(directories_, directory);
        }
        CFRelease(directory);
      }
    }
  }

  ~file_monitor(void) {
    unregister_stream();

    if (directories_) {
      CFRelease(directories_);
      directories_ = nullptr;
    }

    run_loop_thread_ = nullptr;
  }

  void start(void) {
    register_stream();
  }

  void enqueue_file_changed(const std::string& file_path) {
    run_loop_thread_->enqueue(^{
      call_file_changed_slots(file_path);
    });
  }

private:
  void register_stream(void) {
    std::lock_guard<std::mutex> lock(stream_mutex_);

    // Skip if already started.

    if (stream_) {
      return;
    }

    // ----------------------------------------
    // File System Events API does not call the callback if the root directory and files are moved at the same time.
    //
    // Example:
    //
    //   FSEventStreamCreate(... ,{"target/file1", "target/file2"})
    //
    //   $ mkdir target.new
    //   $ echo  target.new/file1
    //   $ mv    target.new target
    //
    //   In this case, the callback will not be called.
    //
    // Thus, we should signal manually once.

    for (const auto& file : files_) {
      run_loop_thread_->enqueue(^{
        call_file_changed_slots(file);
      });
    }

    // ----------------------------------------

    FSEventStreamContext context{0};
    context.info = this;

    // kFSEventStreamCreateFlagWatchRoot and kFSEventStreamCreateFlagFileEvents are required in the following case.
    // (When directory == ~/.karabiner.d/configuration, file == ~/.karabiner.d/configuration/xxx.json)
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
                                  directories_,
                                  kFSEventStreamEventIdSinceNow,
                                  0.1, // 100 ms
                                  flags);
    if (!stream_) {
      logger::get_logger().error("FSEventStreamCreate error @ {0}", __PRETTY_FUNCTION__);
    } else {
      FSEventStreamScheduleWithRunLoop(stream_,
                                       run_loop_thread_->get_run_loop(),
                                       kCFRunLoopDefaultMode);
      if (!FSEventStreamStart(stream_)) {
        logger::get_logger().error("FSEventStreamStart error @ {0}", __PRETTY_FUNCTION__);
      }
    }
  }

  void unregister_stream(void) {
    std::lock_guard<std::mutex> lock(stream_mutex_);

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
        logger::get_logger().info("The configuration directory is updated.");
        // re-register stream
        unregister_stream();
        register_stream();

      } else {
        call_file_changed_slots(event_paths[i]);
      }
    }
  }

  void call_file_changed_slots(const std::string& file_path) const {
    // FSEvents passes realpathed file path to callback.
    // Thus, we have to compare realpathed file paths.

    if (auto path = filesystem::realpath(file_path)) {
      if (filesystem::exists(*path)) {
        if (std::any_of(std::begin(files_),
                        std::end(files_),
                        [&](auto&& p) {
                          return *path == filesystem::realpath(p);
                        })) {
          file_changed(*path);
        }
      }
    }
  }

  CFMutableArrayRef directories_;
  std::vector<std::string> files_;
  std::unique_ptr<cf_utility::run_loop_thread> run_loop_thread_;

  FSEventStreamRef stream_;
  std::mutex stream_mutex_;
};
} // namespace krbn
