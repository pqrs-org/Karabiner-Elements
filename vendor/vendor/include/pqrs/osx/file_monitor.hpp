#pragma once

// pqrs::osx::file_monitor v1.10.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::file_monitor` can be used safely in a multi-threaded environment.

// Limitation:
//
// pqrs::osx::file_monitor signals `file_changed` slot just after the file is closed.
// Thus, we cannot use file_monitor to observe /var/log/xxx.log since
// these files are not closed while the owner process is running.

#include <CoreServices/CoreServices.h>
#include <algorithm>
#include <atomic>
#include <nod/nod.hpp>
#include <ostream>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/filesystem.hpp>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pqrs {
namespace osx {
class file_monitor final : public dispatcher::extra::dispatcher_client {
public:
  enum class availability {
    unavailable,
    available,
  };

  friend std::ostream& operator<<(std::ostream& stream, availability value) {
    switch (value) {
      case availability::unavailable:
        return stream << "unavailable";
      case availability::available:
        return stream << "available";
    }
  }

  // Signals (invoked from the dispatcher thread)

  nod::signal<void(const std::string& watched_directory, availability availability)> watched_directory_availability_changed;
  nod::signal<void(const std::string& watched_file, availability availability)> watched_file_availability_changed;
  nod::signal<void(const std::string& changed_file_path, std::shared_ptr<std::vector<uint8_t>> changed_file_body)> file_changed;
  nod::signal<void(const std::string&)> error_occurred;

  // Methods

  file_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
               const std::vector<std::string>& files) : dispatcher_client(weak_dispatcher),
                                                        files_(files) {
    queue_ = dispatch_queue_create("org.pqrs.osx.file_monitor", DISPATCH_QUEUE_SERIAL);

    for (const auto& f : files) {
      watched_directories_.insert(filesystem::dirname(f));
    }

    dispatch_sync(queue_, ^{
      impl::file_monitors_manager::insert(this);
    });
  }

  ~file_monitor() override {
    // Stop dispatcher work first so no queued dispatcher task can access `this`
    // while we tear down the FSEvent stream on `queue_`.
    detach_from_dispatcher();

    // Run the stream cleanup on `queue_` itself to serialize against `static_stream_callback`.
    dispatch_sync(queue_, ^{
      unregister_stream(false);
      impl::file_monitors_manager::erase(this);
    });

    dispatch_release(queue_);
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      register_stream();
    });
  }

  bool ready() const noexcept {
    return ready_.load();
  }

  void enqueue_file_changed(const std::string& file_path) {
    enqueue_to_dispatcher([this, file_path] {
      if (auto it = file_bodies_.find(file_path);
          it != std::end(file_bodies_)) {
        auto changed_file_body = it->second;
        enqueue_to_dispatcher([this, file_path, changed_file_body] {
          file_changed(file_path,
                       changed_file_body);
        });
      }
    });
  }

  static std::shared_ptr<std::vector<uint8_t>> read_file(const std::string& path) {
    std::ifstream ifstream(path);
    if (!ifstream) {
      return nullptr;
    }

    ifstream.seekg(0, std::fstream::end);
    if (!ifstream) {
      return nullptr;
    }

    auto size = ifstream.tellg();
    if (size < std::streampos(0)) {
      return nullptr;
    }

    ifstream.seekg(0, std::fstream::beg);
    if (!ifstream) {
      return nullptr;
    }

    auto buffer = std::make_shared<std::vector<uint8_t>>(static_cast<size_t>(size));
    if (size > std::streampos(0)) {
      ifstream.read(reinterpret_cast<char*>(buffer->data()),
                    static_cast<std::streamsize>(size));
      if (!ifstream) {
        return nullptr;
      }
    }

    return buffer;
  }

private:
  // This method is executed in the dispatcher thread.
  void register_stream() {
    // Skip if already started.

    if (stream_) {
      return;
    }

    auto directories = make_stream_directories();
    if (!directories) {
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

    update_watched_directory_availabilities();

    for (const auto& file_path : files_) {
      update_stream_file_paths(file_path);

      auto [updated, file_body, availability] = update_file_bodies(file_path);
      if (availability) {
        emit_watched_file_availability_changed(file_path,
                                               *availability);
      }
      if (updated) {
        emit_file_changed(file_path,
                          file_body);
      }
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
    flags |= kFSEventStreamCreateFlagMarkSelf;
    flags |= kFSEventStreamCreateFlagFileEvents;

    stream_ = FSEventStreamCreate(kCFAllocatorDefault,
                                  static_stream_callback,
                                  &context,
                                  *directories,
                                  kFSEventStreamEventIdSinceNow,
                                  0.1, // 100 ms
                                  flags);
    if (!stream_) {
      enqueue_to_dispatcher([this] {
        error_occurred("FSEventStreamCreate is failed.");
      });
    } else {
      FSEventStreamSetDispatchQueue(stream_,
                                    queue_);
      if (!FSEventStreamStart(stream_)) {
        enqueue_to_dispatcher([this] {
          error_occurred("FSEventStreamStart is failed.");
        });
      } else {
        ready_ = true;
      }
    }
  }

  // This method is executed in the dispatcher thread.
  void unregister_stream(bool emit_unavailable_signal = true) {
    ready_ = false;

    if (emit_unavailable_signal) {
      for (const auto& [file_path, file_body] : file_bodies_) {
        if (file_body) {
          emit_watched_file_availability_changed(file_path,
                                                 availability::unavailable);
        }
      }

      for (const auto& [directory_path, current_availability] : directory_availabilities_) {
        if (current_availability == availability::available) {
          emit_watched_directory_availability_changed(directory_path,
                                                      availability::unavailable);
        }
      }
    }

    stream_file_paths_.clear();
    directory_availabilities_.clear();
    file_bodies_.clear();

    if (stream_) {
      FSEventStreamStop(stream_);
      FSEventStreamInvalidate(stream_);
      FSEventStreamRelease(stream_);
      stream_ = nullptr;
    }
  }

  struct fs_event {
    std::string file_path;
    FSEventStreamEventFlags flags;
  };

  static void static_stream_callback(ConstFSEventStreamRef stream,
                                     void* client_callback_info,
                                     size_t num_events,
                                     void* event_paths,
                                     const FSEventStreamEventFlags event_flags[],
                                     const FSEventStreamEventId event_ids[]) {
    auto self = reinterpret_cast<file_monitor*>(client_callback_info);
    if (!self) {
      return;
    }

    if (!impl::file_monitors_manager::alive(self)) {
      return;
    }

    auto fs_events = std::make_shared<std::vector<fs_event>>();

    auto paths = static_cast<const char**>(event_paths);
    for (size_t i = 0; i < num_events; ++i) {
      if (paths[i]) {
        fs_events->push_back({paths[i],
                              event_flags[i]});
      }
    }

    self->enqueue_to_dispatcher([self, fs_events] {
      self->stream_callback(fs_events);
    });
  }

  // This method is executed in the dispatcher thread.
  void stream_callback(std::shared_ptr<std::vector<fs_event>> fs_events) {
    update_watched_directory_availabilities();

    for (const auto& e : *fs_events) {
      if (e.flags & (kFSEventStreamEventFlagRootChanged |
                     kFSEventStreamEventFlagKernelDropped |
                     kFSEventStreamEventFlagUserDropped)) {
        // re-register stream
        unregister_stream(true);
        register_stream();

      } else {
        // FSEvents passes realpathed file path to callback.
        // Thus, we should to convert it to file path in `files_`.

        std::optional<std::string> changed_file_path;

        if (auto realpath = filesystem::realpath(e.file_path)) {
          if (auto it = std::ranges::find_if(files_,
                                             [&](const auto& path) {
                                               return *realpath == filesystem::realpath(path);
                                             });
              it != std::end(files_)) {
            stream_file_paths_[e.file_path] = *it;
            changed_file_path = *it;
          }
        } else {
          // file_path might be removed.
          // (`realpath` fails if file does not exist.)

          if (auto it = stream_file_paths_.find(e.file_path);
              it != std::end(stream_file_paths_)) {
            changed_file_path = it->second;
            stream_file_paths_.erase(it);
          }
        }

        if (changed_file_path) {
          auto file_path = *changed_file_path;
          auto [updated, file_body, availability] = update_file_bodies(file_path);
          if (availability) {
            emit_watched_file_availability_changed(file_path,
                                                   *availability);
          }
          if (updated) {
            bool own_event = e.flags & kFSEventStreamEventFlagOwnEvent;
            if (!own_event) {
              emit_file_changed(file_path,
                                file_body);
            }
          }
        }
      }
    }
  }

  void emit_file_changed(const std::string& file_path,
                         std::shared_ptr<std::vector<uint8_t>> changed_file_body) {
    enqueue_to_dispatcher([this, file_path, changed_file_body] {
      file_changed(file_path,
                   changed_file_body);
    });
  }

  void emit_watched_file_availability_changed(const std::string& file_path,
                                              availability availability) {
    enqueue_to_dispatcher([this, file_path, availability] {
      watched_file_availability_changed(file_path,
                                        availability);
    });
  }

  void emit_watched_directory_availability_changed(const std::string& directory_path,
                                                   availability availability) {
    enqueue_to_dispatcher([this, directory_path, availability] {
      watched_directory_availability_changed(directory_path,
                                             availability);
    });
  }

  void update_stream_file_paths(const std::string& file_path) {
    std::erase_if(stream_file_paths_,
                  [&](const auto& pair) {
                    return pair.second == file_path;
                  });

    if (auto realpath = filesystem::realpath(file_path)) {
      stream_file_paths_[*realpath] = file_path;
    }
  }

  void update_watched_directory_availabilities() {
    for (const auto& directory_path : watched_directories_) {
      auto current_available = static_cast<bool>(filesystem::realpath(directory_path));
      auto it = directory_availabilities_.find(directory_path);
      auto previous_available = it != std::end(directory_availabilities_) && it->second == availability::available;

      if (current_available != previous_available) {
        auto availability = current_available ? availability::available : availability::unavailable;
        directory_availabilities_[directory_path] = availability;
        emit_watched_directory_availability_changed(directory_path,
                                                    availability);
        if (current_available) {
          reevaluate_watched_files_in_directory(directory_path);
        }
      }
    }
  }

  cf::cf_ptr<CFMutableArrayRef> make_stream_directories() const {
    auto directories = cf::make_cf_mutable_array();
    if (!directories) {
      return nullptr;
    }

    std::unordered_set<std::string> stream_directories;
    for (const auto& directory_path : watched_directories_) {
      stream_directories.insert(find_stream_directory(directory_path));
    }

    for (const auto& directory_path : stream_directories) {
      if (auto directory = cf::make_cf_string(directory_path)) {
        CFArrayAppendValue(*directories, *directory);
      }
    }

    return directories;
  }

  std::string find_stream_directory(std::string directory_path) const {
    while (!directory_path.empty()) {
      if (filesystem::realpath(directory_path)) {
        return directory_path;
      }

      auto parent = filesystem::dirname(directory_path);
      if (parent == directory_path) {
        break;
      }
      directory_path = parent;
    }

    return ".";
  }

  void reevaluate_watched_files_in_directory(const std::string& directory_path) {
    for (const auto& file_path : files_) {
      if (filesystem::dirname(file_path) != directory_path) {
        continue;
      }

      update_stream_file_paths(file_path);

      auto [updated, file_body, availability] = update_file_bodies(file_path);
      if (availability) {
        emit_watched_file_availability_changed(file_path,
                                               *availability);
      }
      if (updated) {
        emit_file_changed(file_path,
                          file_body);
      }
    }
  }

  // This method is executed in the dispatcher thread.
  std::tuple<bool, std::shared_ptr<std::vector<uint8_t>>, std::optional<availability>> update_file_bodies(const std::string& file_path) {
    auto file_body = read_file(file_path);
    auto it = file_bodies_.find(file_path);
    auto previous_available = it != std::end(file_bodies_) && static_cast<bool>(it->second);
    if (it != std::end(file_bodies_)) {
      if (it->second && file_body) {
        if (*(it->second) == *(file_body)) {
          // file_body is not changed
          return {false, nullptr, std::nullopt};
        }
      } else if (!it->second && !file_body) {
        // file_body is not changed
        return {false, nullptr, std::nullopt};
      }
    }
    auto current_available = static_cast<bool>(file_body);
    auto availability = current_available != previous_available
                            ? std::optional(current_available ? availability::available
                                                              : availability::unavailable)
                            : std::nullopt;
    file_bodies_[file_path] = file_body;

    return {true, file_body, availability};
  }

  std::vector<std::string> files_;
  std::unordered_set<std::string> watched_directories_;
  dispatch_queue_t queue_{};
  FSEventStreamRef stream_{};
  std::atomic<bool> ready_{false};
  std::unordered_map<std::string, availability> directory_availabilities_;
  // FSEventStreamEventPath -> file in files_
  // {
  //   "/Users/.../target/sub1/file1_1": "target/sub1/file1_1",
  //   "/Users/.../target/sub1/file1_2": "target/sub1/file1_2",
  // }
  std::unordered_map<std::string, std::string> stream_file_paths_;
  std::unordered_map<std::string, std::shared_ptr<std::vector<uint8_t>>> file_bodies_;

  class impl {
  public:
#include "file_monitor/impl/file_monitors_manager.hpp"
  };
};
} // namespace osx
} // namespace pqrs
