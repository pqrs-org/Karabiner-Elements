#pragma once

#include "complex_modifications_assets_file.hpp"
#include <dirent.h>
#include <unistd.h>

namespace krbn {
class complex_modifications_assets_manager final {
public:
  void reload(const std::string& directory, bool load_system_example_file = true) {
    files_.clear();

    // Load system example file.
    if (load_system_example_file) {
      const std::string file_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/complex_modifications_rules_example.json";
      try {
        files_.emplace_back(file_path);
      } catch (std::exception& e) {
        logger::get_logger()->error("Error in {0}: {1}", file_path, e.what());
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
            logger::get_logger()->error("Error in {0}: {1}", file_path, e.what());
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

  const std::vector<complex_modifications_assets_file>& get_files(void) const {
    return files_;
  }

private:
  std::vector<complex_modifications_assets_file> files_;
};
} // namespace krbn
