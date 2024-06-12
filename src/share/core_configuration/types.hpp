#pragma once

namespace krbn {
namespace core_configuration {

enum class error_handling {
  loose,  // ignore errors
  strict, // throw exception
};

}
} // namespace krbn
