#pragma once

#include "configuration_core.hpp"
#include "filesystem.hpp"
#include "grabber_client.hpp"
#include "keyboard_event_output_manager.hpp"
#include <CoreServices/CoreServices.h>
#include <memory>

class configuration_manager final {
public:
  configuration_manager(spdlog::logger& logger,
                        const std::string& configuration_directory,
                        grabber_client& grabber_client,
                        keyboard_event_output_manager& keyboard_event_output_manager) : logger_(logger),
                                                                                        configuration_directory_(configuration_directory),
                                                                                        configuration_core_file_path_(configuration_directory_ + "/karabiner.json"),
                                                                                        grabber_client_(grabber_client),
                                                                                        keyboard_event_output_manager_(keyboard_event_output_manager),
                                                                                        path_(nullptr),
                                                                                        paths_(nullptr),
                                                                                        stream_(nullptr) {
    filesystem::normalize_file_path(configuration_directory_);
    filesystem::normalize_file_path(configuration_core_file_path_);

    mkdir(configuration_directory_.c_str(), 0700);

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

    reload_configuration_core();
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
    for (size_t i = 0; i < num_events; ++i) {
      if (event_flags[i] & kFSEventStreamEventFlagRootChanged) {
        logger_.info("the configuration directory is updated.");
        // re-register stream
        unregister_stream();
        register_stream();

      } else {
        std::string path = event_paths[i];
        filesystem::normalize_file_path(path);

        if (configuration_core_file_path_ == path) {
          logger_.info("karabiner.json is updated.");
          reload_configuration_core();
        }
      }
    }
  }

  void reload_configuration_core(void) {
    auto new_ptr = std::make_unique<configuration_core>(logger_, configuration_core_file_path_);
    // skip if karabiner.json is broken.
    if (configuration_core_ && !new_ptr->is_loaded()) {
      return;
    }

    configuration_core_ = std::move(new_ptr);
    logger_.info("configuration_core_ was loaded.");

    grabber_client_.clear_simple_modifications();
    for (const auto& pair : configuration_core_->get_current_profile_simple_modifications()) {
      grabber_client_.add_simple_modification(pair.first, pair.second);
    }

    keyboard_event_output_manager_.clear_fn_function_keys();
    for (const auto& pair : configuration_core_->get_current_profile_fn_function_keys()) {
      keyboard_event_output_manager_.add_fn_function_key(pair.first, pair.second);
    }
  }

  spdlog::logger& logger_;
  std::string configuration_directory_;
  std::string configuration_core_file_path_;
  grabber_client& grabber_client_;
  keyboard_event_output_manager& keyboard_event_output_manager_;

  CFStringRef path_;
  CFArrayRef paths_;
  FSEventStreamRef stream_;

  std::unique_ptr<configuration_core> configuration_core_;
};
