#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <pqrs/gsl.hpp>
#include <system_error>
#include <vector>

namespace pqrs::osx::xpc {

using peer_id = uint64_t;
using completion = std::function<void(std::expected<pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>>,
                                                    std::error_code>)>;

} // namespace pqrs::osx::xpc
