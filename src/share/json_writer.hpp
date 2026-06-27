#pragma once

#include "json_utility.hpp"
#include "logger.hpp"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <system_error>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace krbn {
class json_writer final {
public:
  template <typename T>
  static void save_to_file(const T& json,
                           const std::filesystem::path& file_path,
                           mode_t file_mode) {
    std::lock_guard<std::mutex> lock(get_mutex());

    try {
      auto body = json_utility::dump(json);

      auto parent_directory = file_path.parent_path();
      if (!parent_directory.empty()) {
        std::error_code error_code;
        std::filesystem::create_directories(parent_directory,
                                            error_code);
      }

      auto tmp_file_path = file_path;
      tmp_file_path += ".tmp";

      unlink(tmp_file_path.c_str());

      std::ofstream output(tmp_file_path);
      if (output) {
        output << body;
        output.close();

        if (!output) {
          logger::get_logger()->error("json_writer failed to write: {0}",
                                      tmp_file_path.string());
          return;
        }

        std::error_code error_code;
        std::filesystem::rename(tmp_file_path,
                                file_path,
                                error_code);
        if (error_code) {
          logger::get_logger()->error("json_writer rename failed: {0}",
                                      error_code.message());
          return;
        }

        if (chmod(file_path.c_str(),
                  file_mode) != 0) {
          logger::get_logger()->error("json_writer chmod failed: {0}",
                                      file_path.string());
        }
      } else {
        logger::get_logger()->error("json_writer failed to open: {0}",
                                    tmp_file_path.string());
      }

    } catch (std::exception& e) {
      logger::get_logger()->error("json_writer error: {0}", e.what());
    }
  }

private:
  [[nodiscard]] static std::mutex& get_mutex() {
    static std::mutex mutex;
    return mutex;
  }
};
} // namespace krbn
