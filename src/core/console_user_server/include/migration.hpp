#pragma once

class migration final {
public:
  static void migrate_v1(void) {
    // Move karabiner.json.
    // ~/.karabiner.d/configuration/karabiner.json -> ~/.config/karabiner/karabiner.json

    auto old_file_path = get_core_configuration_file_path_v1();
    auto new_file_path = constants::get_core_configuration_file_path();

    if (!old_file_path.empty() && new_file_path) {
      if (filesystem::exists(old_file_path) && !filesystem::exists(new_file_path)) {
        auto new_directory = filesystem::dirname(new_file_path);
        filesystem::create_directory_with_intermediate_directories(new_directory, 0700);
        if (filesystem::exists(new_directory)) {
          rename(old_file_path.c_str(), new_file_path);
        }
      }
    }

    // Remove old logs and directories.
    system("/bin/rm ~/.karabiner.d/log/*");
    system("/bin/rmdir ~/.karabiner.d/log");
    system("/bin/rmdir ~/.karabiner.d/configuration");
    system("/bin/rmdir ~/.karabiner.d");
  }

private:
  static std::string get_core_configuration_file_path_v1(void) {
    std::string file_path;

    if (auto home = std::getenv("HOME")) {
      file_path = home;
      file_path += "/.karabiner.d/configuration/karabiner.json";
    }

    return file_path;
  }
};
