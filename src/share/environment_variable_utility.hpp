#pragma once

#include "constants.hpp"
#include "logger.hpp"
#include <pqrs/environment_variable.hpp>

namespace krbn {
namespace environment_variable_utility {

inline std::vector<std::pair<std::string, std::string>> load_custom_environment_variables(void) {
  std::vector<std::pair<std::string, std::string>> result;

  pqrs::environment_variable::load_environment_variables_from_file(
      constants::get_system_environment_file_path(),
      [&result](auto&& name, auto&& value) {
        result.push_back(std::make_pair(std::string(name),
                                        std::string(value)));
      });

  return result;
}

inline void log(std::vector<std::pair<std::string, std::string>>& variables) {
  for (const auto& [name, value] : variables) {
    logger::get_logger()->info("setenv: {0} = {1}",
                               name,
                               value);
  }
}

} // namespace environment_variable_utility
} // namespace krbn
