#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
enum class device_state : uint32_t {
  seized,
  observed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    device_state,
    {
        {device_state::seized, "seized"},
        {device_state::observed, "observed"},
    });
} // namespace krbn
