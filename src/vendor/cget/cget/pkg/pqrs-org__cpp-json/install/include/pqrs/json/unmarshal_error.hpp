#pragma once

#include <stdexcept>

namespace pqrs {
namespace json {
class unmarshal_error : public std::runtime_error {
public:
  unmarshal_error(const std::string& message) : std::runtime_error(message) {
  }
};
} // namespace json
} // namespace pqrs
