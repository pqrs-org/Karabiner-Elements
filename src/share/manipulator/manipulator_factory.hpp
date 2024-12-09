#pragma once

#include "core_configuration/core_configuration.hpp"
#include "manipulator/manipulators/base.hpp"
#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/manipulators/mouse_basic/mouse_basic.hpp"
#include "manipulator/manipulators/mouse_motion_to_scroll/mouse_motion_to_scroll.hpp"
#include "manipulator/manipulators/nop.hpp"
#include "manipulator/types.hpp"
#include <memory>

namespace krbn {
namespace manipulator {
namespace manipulator_factory {
inline gsl::not_null<std::shared_ptr<manipulators::base>> make_manipulator(const nlohmann::json& json,
                                                                           gsl::not_null<std::shared_ptr<const core_configuration::details::complex_modifications_parameters>> parameters) {
  auto it = json.find("type");
  if (it == std::end(json)) {
    throw pqrs::json::unmarshal_error(fmt::format("`type` must be specified: {0}", pqrs::json::dump_for_error_message(json)));
  }

  auto type = it->get<std::string>();

  if (type == "basic") {
    return std::make_shared<manipulators::basic::basic>(json,
                                                        parameters);
  } else if (type == "mouse_basic") {
    return std::make_shared<manipulators::mouse_basic::mouse_basic>(json,
                                                                    parameters);

  } else if (type == "mouse_motion_to_scroll") {
    return std::make_shared<manipulators::mouse_motion_to_scroll::mouse_motion_to_scroll>(json,
                                                                                          parameters);
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", type));
  }
}
} // namespace manipulator_factory
} // namespace manipulator
} // namespace krbn
