#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hid.hpp>
#include <pqrs/json.hpp>

namespace pqrs {
namespace hid {
namespace country_code {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  if (!j.is_number()) {
    using namespace std::string_literals;
    throw json::unmarshal_error("json must be number, but is `"s + j.dump() + "`"s);
  }

  value = value_t(j.get<uint64_t>());
}
} // namespace country_code

namespace product_id {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  if (!j.is_number()) {
    using namespace std::string_literals;
    throw json::unmarshal_error("json must be number, but is `"s + j.dump() + "`"s);
  }

  value = value_t(j.get<uint64_t>());
}
} // namespace product_id

namespace usage_page {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  if (!j.is_number()) {
    using namespace std::string_literals;
    throw json::unmarshal_error("json must be number, but is `"s + j.dump() + "`"s);
  }

  value = value_t(j.get<int32_t>());
}
} // namespace usage_page

namespace usage {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  if (!j.is_number()) {
    using namespace std::string_literals;
    throw json::unmarshal_error("json must be number, but is `"s + j.dump() + "`"s);
  }

  value = value_t(j.get<int32_t>());
}
} // namespace usage

namespace vendor_id {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  if (!j.is_number()) {
    using namespace std::string_literals;
    throw json::unmarshal_error("json must be number, but is `"s + j.dump() + "`"s);
  }

  value = value_t(j.get<uint64_t>());
}
} // namespace vendor_id
} // namespace hid
} // namespace pqrs
