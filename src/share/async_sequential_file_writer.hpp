#pragma once

#include "async_sequential_dispatcher.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace krbn {
class async_sequential_file_writer final {
public:
  class entry final {
  public:
    entry(const std::string& file_path,
          const std::string& body,
          mode_t parent_directory_mode,
          mode_t file_mode) : file_path_(file_path),
                              body_(body),
                              parent_directory_mode_(parent_directory_mode),
                              file_mode_(file_mode) {
    }

    const std::string& get_file_path(void) const {
      return file_path_;
    }

    const std::string& get_body(void) const {
      return body_;
    }

    mode_t get_parent_directory_mode(void) const {
      return parent_directory_mode_;
    }

    mode_t get_file_mode(void) const {
      return file_mode_;
    }

  private:
    std::string file_path_;
    std::string body_;
    mode_t parent_directory_mode_;
    mode_t file_mode_;
  };

  async_sequential_file_writer(void) : async_sequential_dispatcher_(std::bind(&async_sequential_file_writer::write,
                                                                              this,
                                                                              std::placeholders::_1)) {
  }

  void push_back(const std::string& file_path,
                 const std::string& body,
                 mode_t parent_directory_mode,
                 mode_t file_mode) {
    async_sequential_dispatcher_.push_back(std::make_shared<entry>(file_path,
                                                                   body,
                                                                   parent_directory_mode,
                                                                   file_mode));
  }

  void wait(void) {
    async_sequential_dispatcher_.wait();
  }

private:
  void write(const entry& entry) {
    try {
      filesystem::create_directory_with_intermediate_directories(filesystem::dirname(entry.get_file_path()),
                                                                 entry.get_parent_directory_mode());

      std::string tmp_file_path = entry.get_file_path() + ".tmp";

      unlink(tmp_file_path.c_str());

      std::ofstream output(tmp_file_path);
      if (output) {
        output << entry.get_body();

        unlink(entry.get_file_path().c_str());
        rename(tmp_file_path.c_str(), entry.get_file_path().c_str());

        chmod(entry.get_file_path().c_str(), entry.get_file_mode());
      } else {
        logger::get_logger().error("json_utility::save_to_file failed to open: {0}", entry.get_file_path());
      }

    } catch (std::exception& e) {
      logger::get_logger().error("json_utility::save_to_file error: {0}", e.what());
    }
  }

  async_sequential_dispatcher<entry> async_sequential_dispatcher_;
};
} // namespace krbn
