#pragma once

#include "async_file_writer.hpp"
#include "logger.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/filesystem.hpp>
#include <unistd.h>

namespace krbn {
class json_writer final {
public:
  static void async_save_to_file(const nlohmann::json& json,
                                 const std::string& file_path,
                                 mode_t parent_directory_mode,
                                 mode_t file_mode) {
    async_file_writer::enqueue(file_path,
                               json.dump(4),
                               parent_directory_mode,
                               file_mode);
  }

  static void sync_save_to_file(const nlohmann::json& json,
                                const std::string& file_path,
                                mode_t parent_directory_mode,
                                mode_t file_mode) {
    async_save_to_file(json,
                       file_path,
                       parent_directory_mode,
                       file_mode);

    async_file_writer::wait();
  }
};
} // namespace krbn
