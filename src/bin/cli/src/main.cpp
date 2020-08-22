#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <cxxopts.hpp>
#pragma clang diagnostic pop

#include "complex_modifications_assets_file.hpp"
#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "grabber_client.hpp"
#include "json_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "monitor/configuration_monitor.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <pqrs/thread_wait.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace {
void apply_core_configuration_function(const std::function<void(std::shared_ptr<krbn::core_configuration::core_configuration>)>& function) {
  auto wait = pqrs::make_thread_wait();
  krbn::configuration_monitor monitor(krbn::constants::get_user_core_configuration_file_path(),
                                      geteuid());

  monitor.core_configuration_updated.connect([wait, function](auto&& weak_core_configuration) {
    if (auto core_configuration = weak_core_configuration.lock()) {
      function(core_configuration);
    }
    wait->notify();
  });

  monitor.async_start();

  wait->wait_notice();
}

void select_profile(const std::string& name) {
  apply_core_configuration_function([name](auto core_configuration) {
    auto& profiles = core_configuration->get_profiles();
    for (size_t i = 0; i < profiles.size(); ++i) {
      if (profiles[i].get_name() == name) {
        core_configuration->select_profile(i);
        core_configuration->sync_save_to_file();
        return;
      }
    }
    krbn::logger::get_logger()->error("`{0}` is not found.", name);
  });
}

void show_current_profile_name(void) {
  apply_core_configuration_function([](auto core_configuration) {
    std::cout << core_configuration->get_selected_profile().get_name() << std::endl;
  });
}

void list_profile_names(void) {
  apply_core_configuration_function([](auto core_configuration) {
    for (const auto& profile : core_configuration->get_profiles()) {
      std::cout << profile.get_name() << std::endl;
    }
  });
}

void set_variables(const std::string& variables) {
  try {
    auto json = krbn::json_utility::parse_jsonc(variables);

    auto wait = pqrs::make_thread_wait();

    krbn::grabber_client client;
    client.async_start();
    client.async_set_variables(json, [&wait] {
      wait->notify();
    });

    wait->wait_notice();
  } catch (std::exception& e) {
    std::cerr << "set-variables error:" << std::endl
              << e.what() << std::endl;
  }
}

int copy_current_profile_to_system_default_profile(void) {
  pqrs::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_system_configuration_directory(), 0755);
  pqrs::filesystem::copy(krbn::constants::get_user_core_configuration_file_path(),
                         krbn::constants::get_system_core_configuration_file_path());
  return 0;
}

int remove_system_default_profile(void) {
  if (!pqrs::filesystem::exists(krbn::constants::get_system_core_configuration_file_path())) {
    krbn::logger::get_logger()->error("{0} is not found.", krbn::constants::get_system_core_configuration_file_path());
    return 1;
  }
  if (unlink(krbn::constants::get_system_core_configuration_file_path()) != 0) {
    krbn::logger::get_logger()->error("Failed to unlink {0}.");
    return 1;
  }
  return 0;
}
} // namespace

int main(int argc, char** argv) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  int exit_code = 0;

  {
    auto l = spdlog::stdout_color_mt("karabiner_cli");
    l->set_pattern("[%l] %v");
    l->set_level(spdlog::level::err);
    krbn::logger::set_logger(l);
  }

  cxxopts::Options options("karabiner_cli", "A command line utility of Karabiner-Elements.");

  options.add_options()("select-profile", "Select a profile by name.", cxxopts::value<std::string>());
  options.add_options()("show-current-profile-name", "Show current profile name");
  options.add_options()("list-profile-names", "Show all profile names");
  options.add_options()("set-variables", "Json string: {[key: string]: number}", cxxopts::value<std::string>());
  options.add_options()("copy-current-profile-to-system-default-profile", "Copy the current profile to system default profile.");
  options.add_options()("remove-system-default-profile", "Remove the system default profile.");
  options.add_options()("lint-complex-modifications", "Check complex_modifications.json",
                        cxxopts::value<std::string>(),
                        "complex_modifications.json");
  options.add_options()("version", "Displays version.");
  options.add_options()("version-number", "Displays version_number.");
  options.add_options()("help", "Print help.");

  try {
    auto parse_result = options.parse(argc, argv);

    {
      std::string key = "select-profile";
      if (parse_result.count(key)) {
        select_profile(parse_result[key].as<std::string>());
        goto finish;
      }
    }

    {
      std::string key = "show-current-profile-name";
      if (parse_result.count(key)) {
        show_current_profile_name();
        goto finish;
      }
    }

    {
      std::string key = "list-profile-names";
      if (parse_result.count(key)) {
        list_profile_names();
        goto finish;
      }
    }

    {
      std::string key = "set-variables";
      if (parse_result.count(key)) {
        set_variables(parse_result[key].as<std::string>());
        goto finish;
      }
    }

    {
      std::string key = "copy-current-profile-to-system-default-profile";
      if (parse_result.count(key)) {
        if (geteuid() != 0) {
          krbn::logger::get_logger()->error("--{0} requires root privilege.", key);
          exit_code = 1;
          goto finish;
        }
        exit_code = copy_current_profile_to_system_default_profile();
        goto finish;
      }
    }

    {
      std::string key = "remove-system-default-profile";
      if (parse_result.count(key)) {
        if (geteuid() != 0) {
          krbn::logger::get_logger()->error("--{0} requires root privilege.", key);
          exit_code = 1;
          goto finish;
        }
        exit_code = remove_system_default_profile();
        goto finish;
      }
    }

    {
      std::string key = "lint-complex-modifications";
      if (parse_result.count(key)) {
        auto file_path = parse_result[key].as<std::string>();
        try {
          auto assets_file = krbn::complex_modifications_assets_file(file_path);
          auto error_messages = assets_file.lint();
          if (error_messages.empty()) {
            std::cout << "ok" << std::endl;

          } else {
            exit_code = 1;

            for (const auto& e : error_messages) {
              std::cout << e << std::endl;
            }
          }
          goto finish;

        } catch (std::exception& e) {
          exit_code = 1;
          std::cout << e.what() << std::endl;
          goto finish;
        }
      }
    }

    {
      std::string key = "version";
      if (parse_result.count(key)) {
        std::cout << karabiner_version << std::endl;
        goto finish;
      }
    }

    {
      std::string key = "version-number";
      if (parse_result.count(key)) {
        int n = 0;
        std::string number;

        for (const auto& c : std::string_view(karabiner_version)) {
          if (c == '.') {
            if (!number.empty()) {
              n += stoi(number);
              number.clear();
            }
            n *= 100;

          } else {
            number += c;
          }
        }

        if (!number.empty()) {
          n += stoi(number);
          number.clear();
        }

        std::cout << n << std::endl;
        goto finish;
      }
    }

  } catch (const cxxopts::OptionException& e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit_code = 2;
    goto finish;
  }

  std::cout << options.help() << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  karabiner_cli --select-profile 'Default profile'" << std::endl;
  std::cout << "  karabiner_cli --show-current-profile-name" << std::endl;
  std::cout << "  karabiner_cli --list-profile-names" << std::endl;
  std::cout << "  karabiner_cli --set-variables '{\"cli_flag1\":1, \"cli_flag2\":2}'" << std::endl;
  std::cout << std::endl;

  exit_code = 1;

finish:
  return exit_code;
}
