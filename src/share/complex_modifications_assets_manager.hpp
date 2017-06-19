#pragma once

#include "core_configuration.hpp"
#include <dirent.h>

namespace krbn {
class complex_modifications_assets_manager final {
public:
  class file final {
  public:
    file(const nlohmann::json& json) {
      {
        const std::string key = "title";
        if (json.find(key) != json.end() && json[key].is_string()) {
          title_ = json[key];
        }
      }
      {
        const std::string key = "rules";
        if (json.find(key) != json.end() && json[key].is_array()) {
          core_configuration::profile::complex_modifications::parameters parameters;
          for (const auto& j : json[key]) {
            rules_.emplace_back(j, parameters);
          }
        }
      }
    }

    const std::string& get_title(void) const {
      return title_;
    }

    const std::vector<core_configuration::profile::complex_modifications::rule>& get_rules(void) const {
      return rules_;
    }

  private:
    std::string title_;
    std::vector<core_configuration::profile::complex_modifications::rule> rules_;
  };

  void reload(const std::string& directory) {
    files_.clear();

    DIR* dir = opendir(directory.c_str());
    if (dir) {
      while (auto entry = readdir(dir)) {
        if (entry->d_type == DT_REG ||
            entry->d_type == DT_LNK) {
          auto file_path = directory + "/" + entry->d_name;
          std::ifstream stream(file_path);
          if (stream) {
            try {
              auto json = nlohmann::json::parse(stream);
              files_.emplace_back(json);

            } catch (std::exception& e) {
              logger::get_logger().error("parse error in {0}: {1}", file_path, e.what());
            }
          }
        }
      }
      closedir(dir);
    }

    // Sort by title_
    std::sort(std::begin(files_),
              std::end(files_),
              [](auto& a, auto& b) {
                return a.get_title() < b.get_title();
              });
  }

  const std::vector<file>& get_files(void) const {
    return files_;
  }

private:
  std::vector<file> files_;
};
} // namespace krbn
