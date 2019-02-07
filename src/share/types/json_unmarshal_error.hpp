#pragma once

#include <stdexcept>

namespace krbn {
class json_unmarshal_error : public std::runtime_error {
public:
  json_unmarshal_error(const std::string& message) : std::runtime_error(message) {
  }
};
} // namespace krbn
