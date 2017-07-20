#pragma once

#include "filesystem.hpp"
#include "logger.hpp"
#include <fstream>
#include <json/json.hpp>
#include <string>

namespace krbn {
class manipulator_environment final {
public:
  class frontmost_application final {
  public:
    frontmost_application(void) : update_count_(0) {
    }

    nlohmann::json to_json(void) const {
      return nlohmann::json({
          {"bundle_identifier", bundle_identifier_},
          {"file_path", file_path_},
          {"update_count", update_count_},
      });
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

  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"frontmost_application", frontmost_application_.to_json()},
        {"variables", variables_},
    });
  }

  void enable_json_output(const std::string& output_json_file_path) {
    output_json_file_path_ = output_json_file_path;
  }

  void disable_json_output(void) {
    output_json_file_path_.clear();
  }

  const frontmost_application& get_frontmost_application(void) const {
    return frontmost_application_;
  }

  void set_frontmost_application_bundle_identifier(const std::string& value) {
    frontmost_application_.set_bundle_identifier(value);
    save_to_file();
  }

  void set_frontmost_application_file_path(const std::string& value) {
    frontmost_application_.set_file_path(value);
    save_to_file();
  }

  int get_variable(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != std::end(variables_)) {
      return it->second;
    }
    return 0;
  }

  void set_variable(const std::string& name, int value) {
    // logger::get_logger().info("set_variable {0} {1}", name, value);
    variables_[name] = value;
    save_to_file();
  }

private:
  void save_to_file(void) const {
    if (!output_json_file_path_.empty()) {
      filesystem::create_directory_with_intermediate_directories(filesystem::dirname(output_json_file_path_), 0755);

      std::ofstream output(output_json_file_path_);
      if (output) {
        output << std::setw(4) << to_json() << std::endl;
      } else {
        logger::get_logger().warn("Failed to open {0}", output_json_file_path_);
      }
    }
  }

  std::string output_json_file_path_;
  frontmost_application frontmost_application_;
  std::unordered_map<std::string, int> variables_;
};
} // namespace krbn
