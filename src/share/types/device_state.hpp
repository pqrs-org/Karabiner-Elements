#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
enum class device_state : uint32_t {
  grabbed,
  ungrabbed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    device_state,
    {
        {device_state::grabbed, "grabbed"},
        {device_state::ungrabbed, "ungrabbed"},
    });
} // namespace krbn
