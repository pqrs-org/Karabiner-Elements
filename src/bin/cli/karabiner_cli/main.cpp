#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "cxxopts/cxxopts.hpp"
#include <iostream>

namespace {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("karabiner_cli", true);
      logger->set_pattern("[%l] %v");
      logger->set_level(spdlog::level::err);
    }
    return *logger;
  }
};
}

int main(int argc, char* argv[]) {
  krbn::thread_utility::register_main_thread();

  cxxopts::Options options("karabiner_cli", "A command line utility of Karabiner-Elements.");

  options.add_options()("select-profile", "Select a profile by name.", cxxopts::value<std::string>());
  options.add_options()("copy-current-profile-to-system-default-profile", "Copy the current profile to system default profile.");
  options.add_options()("remove-system-default-profile", "Remove the system default profile.");
  options.add_options()("help", "Print help.");

  try {
    options.parse(argc, argv);

    {
      std::string key = "select-profile";
      if (options.count(key)) {
        auto& name = options[key].as<std::string>();

        krbn::configuration_monitor monitor(logger::get_logger(),
                                            krbn::constants::get_user_core_configuration_file_path(),
                                            [&name](std::shared_ptr<krbn::core_configuration> core_configuration) {
                                              auto& profiles = core_configuration->get_profiles();
                                              for (size_t i = 0; i < profiles.size(); ++i) {
                                                if (profiles[i].get_name() == name) {
                                                  core_configuration->select_profile(i);
                                                  core_configuration->save_to_file(krbn::constants::get_user_core_configuration_file_path());
                                                  exit(0);
                                                }
                                              }
                                              logger::get_logger().error("`{0}` is not found.", name);
                                              exit(2);
                                            });
        CFRunLoopRun();
      }
    }

    {
      std::string key = "copy-current-profile-to-system-default-profile";
      if (options.count(key)) {
        if (getuid() != 0) {
          logger::get_logger().error("--{0} requires root privilege.", key);
          return 1;
        }

        krbn::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_system_configuration_directory(), 0755);
        std::ifstream ifstream(krbn::constants::get_user_core_configuration_file_path());
        if (!ifstream) {
          logger::get_logger().error("Failed to open {0}", krbn::constants::get_user_core_configuration_file_path());
          return 1;
        }
        std::ofstream ofstream(krbn::constants::get_system_core_configuration_file_path());
        if (!ofstream) {
          logger::get_logger().error("Failed to open {0}", krbn::constants::get_system_core_configuration_file_path());
          return 1;
        }
        ofstream << ifstream.rdbuf();
        return 0;
      }
    }

    {
      std::string key = "remove-system-default-profile";
      if (options.count(key)) {
        if (getuid() != 0) {
          logger::get_logger().error("--{0} requires root privilege.", key);
          return 1;
        }

        if (!krbn::filesystem::exists(krbn::constants::get_system_core_configuration_file_path())) {
          logger::get_logger().error("{0} is not found.", krbn::constants::get_system_core_configuration_file_path());
          return 1;
        }
        if (unlink(krbn::constants::get_system_core_configuration_file_path()) != 0) {
          logger::get_logger().error("Failed to unlink {0}.");
          return 1;
        }
        return 0;
      }
    }

  } catch (const cxxopts::OptionException& e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return 2;
  }

  std::cout << options.help() << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  karabiner_cli --select-profile 'Default profile'" << std::endl;
  std::cout << std::endl;

  return 1;
}
