#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include "cxxopts/cxxopts.hpp"
#pragma clang diagnostic pop

#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include <iostream>

namespace {
void select_profile(const std::string& name) {
  krbn::configuration_monitor monitor(krbn::constants::get_user_core_configuration_file_path(),
                                      [&name](std::shared_ptr<krbn::core_configuration> core_configuration) {
                                        auto& profiles = core_configuration->get_profiles();
                                        for (size_t i = 0; i < profiles.size(); ++i) {
                                          if (profiles[i].get_name() == name) {
                                            core_configuration->select_profile(i);
                                            core_configuration->save_to_file_synchronously(krbn::constants::get_user_core_configuration_file_path());
                                            return;
                                          }
                                        }
                                        krbn::logger::get_logger().error("`{0}` is not found.", name);
                                      });
}

int copy_current_profile_to_system_default_profile(void) {
  krbn::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_system_configuration_directory(), 0755);
  std::ifstream ifstream(krbn::constants::get_user_core_configuration_file_path());
  if (!ifstream) {
    krbn::logger::get_logger().error("Failed to open {0}", krbn::constants::get_user_core_configuration_file_path());
    return 1;
  }
  std::ofstream ofstream(krbn::constants::get_system_core_configuration_file_path());
  if (!ofstream) {
    krbn::logger::get_logger().error("Failed to open {0}", krbn::constants::get_system_core_configuration_file_path());
    return 1;
  }
  ofstream << ifstream.rdbuf();
  return 0;
}

int remove_system_default_profile(void) {
  if (!krbn::filesystem::exists(krbn::constants::get_system_core_configuration_file_path())) {
    krbn::logger::get_logger().error("{0} is not found.", krbn::constants::get_system_core_configuration_file_path());
    return 1;
  }
  if (unlink(krbn::constants::get_system_core_configuration_file_path()) != 0) {
    krbn::logger::get_logger().error("Failed to unlink {0}.");
    return 1;
  }
  return 0;
}
} // namespace

int main(int argc, const char** argv) {
  krbn::thread_utility::register_main_thread();

  {
    auto l = spdlog::stdout_color_mt("karabiner_cli");
    l->set_pattern("[%l] %v");
    l->set_level(spdlog::level::err);
    krbn::logger::set_logger(l);
  }

  cxxopts::Options options("karabiner_cli", "A command line utility of Karabiner-Elements.");

  options.add_options()("select-profile", "Select a profile by name.", cxxopts::value<std::string>());
  options.add_options()("copy-current-profile-to-system-default-profile", "Copy the current profile to system default profile.");
  options.add_options()("remove-system-default-profile", "Remove the system default profile.");
  options.add_options()("help", "Print help.");

  try {
    auto parse_result = options.parse(argc, argv);

    {
      std::string key = "select-profile";
      if (parse_result.count(key)) {
        select_profile(parse_result[key].as<std::string>());
        return 0;
      }
    }

    {
      std::string key = "copy-current-profile-to-system-default-profile";
      if (parse_result.count(key)) {
        if (getuid() != 0) {
          krbn::logger::get_logger().error("--{0} requires root privilege.", key);
          return 1;
        }
        return copy_current_profile_to_system_default_profile();
      }
    }

    {
      std::string key = "remove-system-default-profile";
      if (parse_result.count(key)) {
        if (getuid() != 0) {
          krbn::logger::get_logger().error("--{0} requires root privilege.", key);
          return 1;
        }
        return remove_system_default_profile();
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
