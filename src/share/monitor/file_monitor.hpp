#pragma once

// `krbn::file_monitor` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include "dispatcher.hpp"
#include "file_utility.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include <CoreServices/CoreServices.h>
#include <boost/signals2.hpp>
#include <utility>
#include <vector>

namespace krbn {
class file_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(const std::string& changed_file_path, std::shared_ptr<std::vector<uint8_t>> changed_file_body)> file_changed;

  // Methods

  file_monitor(const std::vector<std::string>& files) : dispatcher_client(),
                                                        files_(files),
                                                        directories_(cf_utility::create_cfmutablearray()),
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

  virtual ~file_monitor(void) {
    detach_from_dispatcher([this] {
      unregister_stream();
    });

    run_loop_thread_->terminate();
    run_loop_thread_ = nullptr;

    if (directories_) {
      CFRelease(directories_);
      directories_ = nullptr;
    }
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      register_stream();
    });
  }

  void enqueue_file_changed(const std::string& file_path) {
    enqueue_to_dispatcher([this, file_path] {
      call_file_changed_slots(file_path);
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void register_stream(void) {
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
      call_file_changed_slots(file);
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
                                       kCFRunLoopCommonModes);
      if (!FSEventStreamStart(stream_)) {
        logger::get_logger().error("FSEventStreamStart error @ {0}", __PRETTY_FUNCTION__);
      }

      run_loop_thread_->wake();
    }
  }

  // This method is executed in the dispatcher thread.
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
    struct event {
      std::string file_path;
      FSEventStreamEventFlags flags;
    };
    auto events = std::make_shared<std::vector<event>>(num_events);

    for (size_t i = 0; i < num_events; ++i) {
      if (event_paths[i]) {
        events->push_back({event_paths[i],
                           event_flags[i]});
      }
    }

    enqueue_to_dispatcher([this, events] {
      for (auto e : *events) {
        if (e.flags & (kFSEventStreamEventFlagRootChanged |
                       kFSEventStreamEventFlagKernelDropped |
                       kFSEventStreamEventFlagUserDropped)) {
          logger::get_logger().info("The configuration directory is updated.");

          // re-register stream
          unregister_stream();
          register_stream();

        } else {
          // FSEvents passes realpathed file path to callback.
          // Thus, we should to convert it to file path in `files_`.

          boost::optional<std::string> changed_file_path;

          if (auto realpath = filesystem::realpath(e.file_path)) {
            auto it = std::find_if(std::begin(files_),
                                   std::end(files_),
                                   [&](auto&& p) {
                                     return *realpath == filesystem::realpath(p);
                                   });
            if (it != std::end(files_)) {
              stream_file_paths_[e.file_path] = *it;
              changed_file_path = *it;
            }
          } else {
            // file_path might be removed.
            // (`realpath` fails if file does not exist.)

            auto it = stream_file_paths_.find(e.file_path);
            if (it != std::end(stream_file_paths_)) {
              changed_file_path = it->second;
              stream_file_paths_.erase(it);
            }
          }

          if (changed_file_path) {
            call_file_changed_slots(*changed_file_path);
          }
        }
      }
    });
  }

  // This method is executed in the dispatcher thread.
  void call_file_changed_slots(const std::string& file_path) {
    if (std::any_of(std::begin(files_),
                    std::end(files_),
                    [&](auto&& p) {
                      return file_path == p;
                    })) {
      auto file_body = file_utility::read_file(file_path);
      auto it = file_bodies_.find(file_path);
      if (it != std::end(file_bodies_)) {
        if (it->second && file_body) {
          if (*(it->second) == *(file_body)) {
            // file_body is not changed
            return;
          }
        } else if (!it->second && !file_body) {
          // file_body is not changed
          return;
        }
      }
      file_bodies_[file_path] = file_body;

      enqueue_to_dispatcher([this, file_path] {
        file_changed(file_path,
                     file_utility::read_file(file_path));
      });
    }
  }

  std::vector<std::string> files_;

  std::unique_ptr<cf_utility::run_loop_thread> run_loop_thread_;
  CFMutableArrayRef directories_;
  FSEventStreamRef stream_;
  // FSEventStreamEventPath -> file in files_
  // {
  //   "/Users/tekezo/.../target/sub1/file1_1": "target/sub1/file1_1",
  //   "/Users/tekezo/.../target/sub1/file1_2": "target/sub1/file1_2",
  // }
  std::unordered_map<std::string, std::string> stream_file_paths_;
  std::unordered_map<std::string, std::shared_ptr<std::vector<uint8_t>>> file_bodies_;
};
} // namespace krbn
