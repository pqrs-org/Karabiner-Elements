#pragma once

#include "filesystem.hpp"
#include "logger.hpp"
#include <fstream>
#include <iostream>
#include <json/json.hpp>
#include <string>

namespace krbn {
class manipulator_environment final {
public:
  class frontmost_application final {
  public:
    frontmost_application(void) {
    }

    frontmost_application(const std::string& bundle_identifier,
                          const std::string& file_path) : bundle_identifier_(bundle_identifier),
                                                          file_path_(file_path) {
    }

    nlohmann::json to_json(void) const {
      return nlohmann::json({
          {"bundle_identifier", bundle_identifier_},
          {"file_path", file_path_},
      });
    }

    const std::string& get_bundle_identifier(void) const {
      return bundle_identifier_;
    }

    const std::string& get_file_path(void) const {
      return file_path_;
    }

    bool operator==(const frontmost_application& other) const {
      return bundle_identifier_ == other.bundle_identifier_ &&
             file_path_ == other.file_path_;
    }

  private:
    std::string bundle_identifier_;
    std::string file_path_;
  };

  class input_source final {
  public:
    input_source(void) {
    }

    input_source(const std::string& language,
                 const std::string& input_source_id,
                 const std::string& input_mode_id) : language_(language),
                                                     input_source_id_(input_source_id),
                                                     input_mode_id_(input_mode_id) {
    }

    nlohmann::json to_json(void) const {
      return nlohmann::json({
          {"language", language_},
          {"input_source_id", input_source_id_},
          {"input_mode_id", input_mode_id_},
      });
    }

    const std::string& get_language(void) const {
      return language_;
    }

    const std::string& get_input_source_id(void) const {
      return input_source_id_;
    }

    const std::string& get_input_mode_id(void) const {
      return input_mode_id_;
    }

    bool operator==(const input_source& other) const {
      return language_ == other.language_ &&
             input_source_id_ == other.input_source_id_ &&
             input_mode_id_ == other.input_mode_id_;
    }

  private:
    std::string language_;
    std::string input_source_id_;
    std::string input_mode_id_;
  };

  manipulator_environment(const manipulator_environment&) = delete;

  manipulator_environment(void) {
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"frontmost_application", frontmost_application_.to_json()},
        {"input_source", input_source_.to_json()},
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

  void set_frontmost_application(const frontmost_application& value) {
    frontmost_application_ = value;
    save_to_file();
  }

  void set_input_source(const input_source& value) {
    input_source_ = value;
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
  input_source input_source_;
  std::unordered_map<std::string, int> variables_;
};

inline std::ostream& operator<<(std::ostream& stream, const manipulator_environment::frontmost_application& value) {
  stream << value.get_bundle_identifier() << "," << value.get_file_path();
  return stream;
}
} // namespace krbn
