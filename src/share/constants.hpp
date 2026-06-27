#pragma once

#include "json_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <pqrs/osx/launchctl.hpp>
#include <spdlog/fmt/fmt.h>
#include <string>
#include <string_view>
#include <thread>

namespace krbn {
class constants final {
public:
  static constexpr size_t unix_domain_stream_max_message_size = 32 * 1024;

  [[nodiscard]] static const std::filesystem::path& get_version_file_path() {
    static auto path = std::filesystem::path("/Library/Application Support/org.pqrs/Karabiner-Elements/version");
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_tmp_directory() {
    static auto path = std::filesystem::path("/Library/Application Support/org.pqrs/tmp");
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_rootonly_directory() {
    static auto path = get_tmp_directory() / "rootonly";
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_system_user_directory() {
    static auto path = get_tmp_directory() / "user";
    return path;
  }

  [[nodiscard]] static std::filesystem::path get_system_user_directory(uid_t uid) {
    return get_system_user_directory() / fmt::format("{0}", uid);
  }

  [[nodiscard]] static const std::filesystem::path& get_karabiner_core_service_socket_file_path() {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // "/Library/Application Support/org.pqrs/tmp/karabiner_core_service.sock" length is 69.

    static auto path = get_tmp_directory() / "karabiner_core_service.sock";
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_session_monitor_receiver_socket_file_path() {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // "/Library/Application Support/org.pqrs/tmp/rootonly/karabiner_session_monitor_receiver.sock" length is 90.

    static auto path = get_rootonly_directory() / std::filesystem::path("karabiner_session_monitor_receiver.sock");
    return path;
  }

  [[nodiscard]] static std::filesystem::path get_console_user_server_socket_file_path(uid_t uid) {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // "/Library/Application Support/org.pqrs/tmp/user/501/karabiner_console_user_server.sock" length is 85.

    return constants::get_system_user_directory(uid) / std::filesystem::path("karabiner_console_user_server.sock");
  }

  [[nodiscard]] static const std::filesystem::path& get_karabiner_machine_identifier_json_file_path() {
    static auto path = get_tmp_directory() / "karabiner_machine_identifier.json";
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_system_configuration_directory() {
    static auto path = std::filesystem::path("/Library/Application Support/org.pqrs/config");
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_system_app_icon_configuration_file_path() {
    static auto path = get_system_configuration_directory() / "karabiner_app_icon.json";
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_system_core_configuration_file_path() {
    static auto path = get_system_configuration_directory() / "karabiner.json";
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_system_environment_file_path() {
    static auto path = get_system_configuration_directory() / "karabiner_environment";
    return path;
  }

  [[nodiscard]] static const std::filesystem::path& get_user_configuration_directory() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::filesystem::path directory;

    if (!once) {
      once = true;
      if (auto xdg_config_home = std::getenv("XDG_CONFIG_HOME")) {
        directory = xdg_config_home;
      } else {
        if (auto home = std::getenv("HOME")) {
          directory = home;
          directory /= ".config";
        }
      }

      if (!directory.empty()) {
        directory /= "karabiner";
      }
    }

    return directory;
  }

  [[nodiscard]] static const std::filesystem::path& get_user_data_directory() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::filesystem::path directory;

    if (!once) {
      once = true;
      if (auto xdg_config_home = std::getenv("XDG_DATA_HOME")) {
        directory = xdg_config_home;
      } else {
        if (auto home = std::getenv("HOME")) {
          directory = home;
          directory /= ".local/share";
        }
      }

      if (!directory.empty()) {
        directory /= "karabiner";
      }
    }

    return directory;
  }

  [[nodiscard]] static const std::filesystem::path& get_user_core_configuration_file_path() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::filesystem::path file_path;

    if (!once) {
      once = true;
      auto directory = get_user_configuration_directory();
      if (!directory.empty()) {
        file_path = directory / "karabiner.json";
      }
    }

    return file_path;
  }

  [[nodiscard]] static const std::filesystem::path& get_user_core_configuration_automatic_backups_directory() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::filesystem::path directory;

    if (!once) {
      once = true;
      auto d = get_user_configuration_directory();
      if (!d.empty()) {
        directory = d / "automatic_backups";
      }
    }

    return directory;
  }

  [[nodiscard]] static const std::filesystem::path& get_user_complex_modifications_assets_directory() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::filesystem::path directory;

    if (!once) {
      once = true;
      auto d = get_user_configuration_directory();
      if (!d.empty()) {
        directory = d / "assets/complex_modifications";
      }
    }

    return directory;
  }

  [[nodiscard]] static const std::filesystem::path& get_user_log_directory() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::filesystem::path directory;

    if (!once) {
      once = true;
      auto d = get_user_data_directory();
      if (!d.empty()) {
        directory = d / "log";
      }
    }

    return directory;
  }

  [[nodiscard]] static const std::filesystem::path& get_user_tmp_directory() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::filesystem::path directory;

    if (!once) {
      once = true;
      auto d = get_user_data_directory();
      if (!d.empty()) {
        directory = d / "tmp";
      }
    }

    return directory;
  }

  [[nodiscard]] static const karabiner_machine_identifier& get_karabiner_machine_identifier() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static karabiner_machine_identifier identifier;

    if (!once) {
      once = true;

      auto file_path = get_karabiner_machine_identifier_json_file_path();
      std::ifstream input(file_path);
      if (input) {
        try {
          auto json = json_utility::parse_jsonc(input);
          identifier = karabiner_machine_identifier(json["karabiner_machine_identifier"].get<std::string>());
        } catch (std::exception& e) {
          logger::get_logger()->error("parse error in {0}: {1}", file_path.string(), e.what());
        }
      }

      if (identifier == karabiner_machine_identifier("")) {
        identifier = karabiner_machine_identifier("krbn-empty-machine-identifier");
      }
    }

    return identifier;
  }
};
} // namespace krbn
