#pragma once

#include "boost_defs.hpp"

#include "async_sequential_dispatcher.hpp"
#include "logger.hpp"
#include <boost/optional.hpp>
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
          boost::optional<mode_t> mode) : file_path_(file_path),
                                          body_(body),
                                          mode_(mode) {
    }

    const std::string& get_file_path(void) const {
      return file_path_;
    }

    const std::string& get_body(void) const {
      return body_;
    }

    boost::optional<mode_t> get_mode(void) const {
      return mode_;
    }

  private:
    std::string file_path_;
    std::string body_;
    boost::optional<mode_t> mode_;
  };

  async_sequential_file_writer(void) : async_sequential_dispatcher_(std::bind(&async_sequential_file_writer::write,
                                                                              this,
                                                                              std::placeholders::_1)) {
  }

  void push_back(const std::string& file_path,
                 const std::string& body,
                 boost::optional<mode_t> mode) {
    async_sequential_dispatcher_.push_back(std::make_shared<entry>(file_path, body, mode));
  }

  void wait(void) {
    async_sequential_dispatcher_.wait();
  }

private:
  void write(const entry& entry) {
    try {
      std::string tmp_file_path = entry.get_file_path() + ".tmp";

      unlink(tmp_file_path.c_str());

      std::ofstream output(tmp_file_path);
      if (output) {
        output << entry.get_body();

        unlink(entry.get_file_path().c_str());
        rename(tmp_file_path.c_str(), entry.get_file_path().c_str());

        if (auto mode = entry.get_mode()) {
          chmod(entry.get_file_path().c_str(), *mode);
        }
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
