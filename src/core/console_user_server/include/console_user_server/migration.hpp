#pragma once

namespace krbn {
namespace console_user_server {
class migration final {
public:
  static void migrate_v1(void) {
    // Move karabiner.json.
    // ~/.karabiner.d/configuration/karabiner.json -> ~/.config/karabiner/karabiner.json

    auto old_file_path = get_user_core_configuration_file_path_v1();
    auto new_file_path = constants::get_user_core_configuration_file_path();

    if (!old_file_path.empty() && !new_file_path.empty()) {
      if (pqrs::filesystem::exists(old_file_path) && !pqrs::filesystem::exists(new_file_path)) {
        auto new_directory = pqrs::filesystem::dirname(new_file_path);
        pqrs::filesystem::create_directory_with_intermediate_directories(new_directory, 0700);
        if (pqrs::filesystem::exists(new_directory)) {
          rename(old_file_path.c_str(), new_file_path.c_str());
        }
      }
    }

    // Remove old logs and directories.
    system("/bin/rm    ~/.karabiner.d/log/*         2>/dev/null");
    system("/bin/rmdir ~/.karabiner.d/log           2>/dev/null");
    system("/bin/rmdir ~/.karabiner.d/configuration 2>/dev/null");
    system("/bin/rmdir ~/.karabiner.d               2>/dev/null");
  }

private:
  static std::string get_user_core_configuration_file_path_v1(void) {
    std::string file_path;

    if (auto home = std::getenv("HOME")) {
      file_path = home;
      file_path += "/.karabiner.d/configuration/karabiner.json";
    }

    return file_path;
  }
};
} // namespace console_user_server
} // namespace krbn
