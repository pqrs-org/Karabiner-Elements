#pragma once

namespace krbn {
enum class event_origin {
  none,
  grabbed_device,
  observed_device,
  virtual_device,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    event_origin,
    {
        {event_origin::none, "none"},
        {event_origin::grabbed_device, "grabbed_device"},
        {event_origin::observed_device, "observed_device"},
        {event_origin::virtual_device, "virtual_device"},
    });
} // namespace krbn
