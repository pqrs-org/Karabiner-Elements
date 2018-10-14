#pragma once

// `krbn::async_file_writer` can be used safely in a multi-threaded environment.

#include "dispatcher_utility.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace krbn {
class async_file_writer final {
public:
  async_file_writer(const async_file_writer&) = delete;
  async_file_writer(void) = delete;

  static void enqueue(const std::string& file_path,
                      const std::string& body,
                      mode_t parent_directory_mode,
                      mode_t file_mode) {
    dispatcher_utility::enqueue_to_file_writer_dispatcher([file_path, body, parent_directory_mode, file_mode] {
      try {
        filesystem::create_directory_with_intermediate_directories(filesystem::dirname(file_path),
                                                                   parent_directory_mode);

        std::string tmp_file_path = file_path + ".tmp";

        unlink(tmp_file_path.c_str());

        std::ofstream output(tmp_file_path);
        if (output) {
          output << body;

          unlink(file_path.c_str());
          rename(tmp_file_path.c_str(), file_path.c_str());

          chmod(file_path.c_str(), file_mode);
        } else {
          logger::get_logger().error("async_file_writer failed to open: {0}", file_path);
        }

      } catch (std::exception& e) {
        logger::get_logger().error("async_file_writer error: {0}", e.what());
      }
    });
  }

  static void wait(void) {
    auto wait = pqrs::dispatcher::make_wait();

    dispatcher_utility::enqueue_to_file_writer_dispatcher([wait] {
      wait->notify();
    });

    wait->wait_notice();
  }
};
} // namespace krbn
