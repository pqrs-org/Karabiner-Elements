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
      logger = spdlog::stdout_logger_mt("karabiner", true);
      logger->set_pattern("[%l] %v");
      logger->set_level(spdlog::level::err);
    }
    return *logger;
  }
};
}

int main(int argc, char* argv[]) {
  krbn::thread_utility::register_main_thread();

  cxxopts::Options options("karabiner", "A command line utility of Karabiner-Elements.");

  options.add_options()("select-profile", "Select a profile by name.", cxxopts::value<std::string>());
  options.add_options()("help", "Print help.");

  try {
    options.parse(argc, argv);

    if (options.count("select-profile")) {
      auto& name = options["select-profile"].as<std::string>();

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
  } catch (const std::exception& e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return 2;
  }

  std::cout << options.help() << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  karabiner --select-profile 'Default profile'" << std::endl;
  std::cout << std::endl;

  return 1;
}
