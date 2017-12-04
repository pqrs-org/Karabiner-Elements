#pragma once

#include "boost_defs.hpp"

#include <boost/optional.hpp>
#include <json/json.hpp>

namespace krbn {
class json_utility final {
public:
  template <typename T>
  static boost::optional<T> find_optional(const nlohmann::json& json,
                                          const std::string& key) {
    try {
      auto it = json.find(key);
      if (it != std::end(json)) {
        return it->get<T>();
      }
    } catch (std::exception&) {
    }

    return boost::none;
  }
};
} // namespace krbn
