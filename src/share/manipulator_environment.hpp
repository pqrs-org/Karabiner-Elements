#pragma once

#include <string>
#include "logger.hpp"

namespace krbn {
class manipulator_environment final {
public:
  class frontmost_application final {
  public:
    const std::string& get_bundle_identifier(void) const {
      return bundle_identifier_;
    }
    void set_bundle_identifier(const std::string& value) {
      //logger::get_logger().info("bundle_identifier_ {0}", value);
      bundle_identifier_ = value;
    }

    const std::string& get_file_path(void) const {
      return file_path_;
    }
    void set_file_path(const std::string& value) {
      //logger::get_logger().info("file_path_ {0}", value);
      file_path_ = value;
    }

  private:
    std::string bundle_identifier_;
    std::string file_path_;
  };

  manipulator_environment(const manipulator_environment&) = delete;

  manipulator_environment(void) {
  }

  frontmost_application& get_frontmost_application(void) {
    return frontmost_application_;
  }

  const frontmost_application& get_frontmost_application(void) const {
    return frontmost_application_;
  }

private:
  frontmost_application frontmost_application_;
};
} // namespace krbn
