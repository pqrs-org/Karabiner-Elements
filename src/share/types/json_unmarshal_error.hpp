#pragma once

#include <exception>

namespace krbn {
class json_unmarshal_error : public std::exception {
};
} // namespace krbn
