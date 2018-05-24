#pragma once

#include "boost_defs.hpp"

#include "filesystem.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <boost/functional/hash.hpp>
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

    frontmost_application(const nlohmann::json& json) {
      if (json.is_object()) {
        for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (key == "bundle_identifier") {
            if (value.is_string()) {
              bundle_identifier_ = value.get<std::string>();
            } else {
              logger::get_logger().error("bundle_identifier should be string: {0}", json.dump());
            }
          } else if (key == "file_path") {
            if (value.is_string()) {
              file_path_ = value.get<std::string>();
            } else {
              logger::get_logger().error("file_path should be string: {0}", json.dump());
            }
          } else {
            logger::get_logger().error("json error: Unknown key: {0} in {1}", key, json.dump());
          }
        }
      } else {
        logger::get_logger().error("frontmost_application should be object: {0}", json.dump());
      }
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

    friend size_t hash_value(const frontmost_application& value) {
      size_t h = 0;
      boost::hash_combine(h, value.bundle_identifier_);
      boost::hash_combine(h, value.file_path_);
      return h;
    }

  private:
    std::string bundle_identifier_;
    std::string file_path_;
  };

  manipulator_environment(const manipulator_environment&) = delete;

  manipulator_environment(void) {
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"frontmost_application", frontmost_application_.to_json()},
        {"input_source_identifiers", input_source_identifiers_.to_json()},
        {"variables", variables_},
        {"keyboard_type", keyboard_type_},
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

  const input_source_identifiers& get_input_source_identifiers(void) const {
    return input_source_identifiers_;
  }

  void set_input_source_identifiers(const input_source_identifiers& value) {
    input_source_identifiers_ = value;
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

  const std::string& get_keyboard_type(void) const {
    return keyboard_type_;
  }

  void set_keyboard_type(const std::string& value) {
    keyboard_type_ = value;
    save_to_file();
  }

private:
  void save_to_file(void) const {
    if (!output_json_file_path_.empty()) {
      filesystem::create_directory_with_intermediate_directories(filesystem::dirname(output_json_file_path_), 0755);
      json_utility::save_to_file(to_json(), output_json_file_path_, 0644);
    }
  }

  std::string output_json_file_path_;
  frontmost_application frontmost_application_;
  input_source_identifiers input_source_identifiers_;
  std::unordered_map<std::string, int> variables_;
  std::string keyboard_type_;
};

inline std::ostream& operator<<(std::ostream& stream, const manipulator_environment::frontmost_application& value) {
  stream << value.get_bundle_identifier() << "," << value.get_file_path();
  return stream;
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::manipulator_environment::frontmost_application> final {
  std::size_t operator()(const krbn::manipulator_environment::frontmost_application& v) const {
    return hash_value(v);
  }
};
} // namespace std
