#pragma once

#include "logger.hpp"
#include <string>

namespace krbn {
class manipulator_environment final {
public:
  class frontmost_application final {
  public:
    frontmost_application(void) : update_count_(0) {
    }

    const std::string& get_bundle_identifier(void) const {
      return bundle_identifier_;
    }
    void set_bundle_identifier(const std::string& value) {
      //logger::get_logger().info("bundle_identifier_ {0}", value);
      bundle_identifier_ = value;
      ++update_count_;
    }

    const std::string& get_file_path(void) const {
      return file_path_;
    }
    void set_file_path(const std::string& value) {
      //logger::get_logger().info("file_path_ {0}", value);
      file_path_ = value;
      ++update_count_;
    }

    uint32_t get_update_count(void) const {
      return update_count_;
    }

  private:
    std::string bundle_identifier_;
    std::string file_path_;
    uint32_t update_count_;
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

  int get_variable(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != std::end(variables_)) {
      return it->second;
    }
    return 0;
  }

  void set_variable(const std::string& name, int value) {
    logger::get_logger().info("set_variable {0} {1}", name, value);
    variables_[name] = value;
  }

private:
  frontmost_application frontmost_application_;
  std::unordered_map<std::string, int> variables_;
};
} // namespace krbn
