#pragma once

namespace krbn::core_configuration {

enum class error_handling {
  loose,  // ignore errors
  strict, // throw exception
};

} // namespace krbn::core_configuration
