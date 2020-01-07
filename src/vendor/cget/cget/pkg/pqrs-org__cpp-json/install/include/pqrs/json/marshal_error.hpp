#pragma once

#include <stdexcept>

namespace pqrs {
namespace json {
class marshal_error : public std::runtime_error {
public:
  marshal_error(const std::string& message) : std::runtime_error(message) {
  }
};
} // namespace json
} // namespace pqrs
