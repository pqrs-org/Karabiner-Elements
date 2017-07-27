#pragma once

#include "boost_defs.hpp"

#include <boost/algorithm/string/trim_all.hpp>
#include <sstream>

namespace krbn {
class shell_utility final {
public:
  static std::string make_background_command(const std::string& command) {
    auto trimmed_command = boost::trim_all_copy(command);
    if (trimmed_command.empty()) {
      return "";
    }

    if (trimmed_command.back() != '&') {
      trimmed_command += " &";
    }

    return trimmed_command;
  }
};
} // namespace krbn
