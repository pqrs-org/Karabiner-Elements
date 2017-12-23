#pragma once

#include "boost_defs.hpp"

#include "logger.hpp"
#include <boost/optional.hpp>
#include <fstream>
#include <json/json.hpp>
#include <unistd.h>

namespace krbn {
class json_utility final {
public:
  template <typename T>
  static boost::optional<T> find_optional(const nlohmann::json& json,
                                          const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      try {
        return it->get<T>();
      } catch (std::exception&) {
      }
    }

    return boost::none;
  }

  static const nlohmann::json* find_array(const nlohmann::json& json,
                                          const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      if (it->is_array()) {
        return &(*it);
      }
    }
    return nullptr;
  }

  static const nlohmann::json* find_object(const nlohmann::json& json,
                                           const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      if (it->is_object()) {
        return &(*it);
      }
    }
    return nullptr;
  }

  static const nlohmann::json* find_json(const nlohmann::json& json,
                                         const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      return &(*it);
    }
    return nullptr;
  }

  static nlohmann::json find_copy(const nlohmann::json& json,
                                  const std::string& key,
                                  const nlohmann::json& fallback_value) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      return *it;
    }
    return fallback_value;
  }

  static bool save_to_file(const nlohmann::json& json,
                           const std::string& file_path) {
    try {
      std::string tmp_file_path = file_path + ".tmp";

      unlink(tmp_file_path.c_str());

      std::ofstream output(tmp_file_path);
      if (output) {
        output << std::setw(4) << json << std::endl;

        unlink(file_path.c_str());
        rename(tmp_file_path.c_str(), file_path.c_str());
        return true;
      } else {
        logger::get_logger().error("json_utility::save_to_file failed to open: {0}", file_path);
      }

    } catch (std::exception& e) {
      logger::get_logger().error("json_utility::save_to_file error: {0}", e.what());
    }

    return false;
  }
};
} // namespace krbn
