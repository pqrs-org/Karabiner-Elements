#pragma once

#include "async_sequential_dispatcher.hpp"
#include "logger.hpp"
#include <fstream>

namespace krbn {
class async_sequential_file_writer final {
public:
  async_sequential_file_writer(void) : async_sequential_dispatcher_(std::bind(&async_sequential_file_writer::write,
                                                                              this,
                                                                              std::placeholders::_1)) {
  }

  void push_back(const std::string& file_path,
                 const std::string& body) {
  }

private:
  void write(const std::pair<std::string, std::string> pair) {
    try {
      const auto& file_path = pair.first;
      const auto& body = pair.second;

      std::string tmp_file_path = file_path + ".tmp";

      unlink(tmp_file_path.c_str());

      std::ofstream output(tmp_file_path);
      if (output) {
        output << body;

        unlink(file_path.c_str());
        rename(tmp_file_path.c_str(), file_path.c_str());
      } else {
        logger::get_logger().error("json_utility::save_to_file failed to open: {0}", file_path);
      }

    } catch (std::exception& e) {
      logger::get_logger().error("json_utility::save_to_file error: {0}", e.what());
    }
  }

  async_sequential_dispatcher<std::pair<std::string, std::string>> async_sequential_dispatcher_;
};
} // namespace krbn
