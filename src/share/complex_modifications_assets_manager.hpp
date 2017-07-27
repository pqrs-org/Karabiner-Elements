#pragma once

#include "boost_defs.hpp"

#include "constants.hpp"
#include "core_configuration.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <dirent.h>
#include <unistd.h>

namespace krbn {
class complex_modifications_assets_manager final {
public:
  class file final {
  public:
    file(const std::string& file_path) : file_path_(file_path) {
      std::ifstream stream(file_path);
      if (!stream) {
        throw std::runtime_error(std::string("Failed to open ") + file_path);
      } else {
        auto json = nlohmann::json::parse(stream);

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
    }

    const std::string& get_file_path(void) const {
      return file_path_;
    }

    const std::string& get_title(void) const {
      return title_;
    }

    const std::vector<core_configuration::profile::complex_modifications::rule>& get_rules(void) const {
      return rules_;
    }

    void push_back_rule_to_core_configuration_profile(core_configuration::profile& profile,
                                                      size_t index) {
      if (index < rules_.size()) {
        profile.push_back_complex_modifications_rule(rules_[index]);
      }
    }

    void unlink_file(void) const {
      unlink(file_path_.c_str());
    }

    bool is_user_file(void) const {
      return boost::starts_with(file_path_, constants::get_user_complex_modifications_assets_directory());
    }

  private:
    std::string file_path_;
    std::string title_;
    std::vector<core_configuration::profile::complex_modifications::rule> rules_;
  };

  void reload(const std::string& directory, bool load_system_example_file = true) {
    files_.clear();

    // Load system example file.
    if (load_system_example_file) {
      const std::string file_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/complex_modifications_rules_example.json";
      try {
        files_.emplace_back(file_path);
      } catch (std::exception& e) {
        logger::get_logger().error("Error in {0}: {1}", file_path, e.what());
      }
    }

    // Load user files.
    DIR* dir = opendir(directory.c_str());
    if (dir) {
      while (auto entry = readdir(dir)) {
        if (entry->d_type == DT_REG ||
            entry->d_type == DT_LNK) {
          auto file_path = directory + "/" + entry->d_name;

          try {
            files_.emplace_back(file_path);

          } catch (std::exception& e) {
            logger::get_logger().error("Error in {0}: {1}", file_path, e.what());
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
