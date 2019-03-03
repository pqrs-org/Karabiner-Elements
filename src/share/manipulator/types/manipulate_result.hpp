#pragma once

namespace krbn {
namespace manipulator {
enum class manipulate_result {
  passed,
  manipulated,
  needs_wait_until_time_stamp,
};
} // namespace manipulator
} // namespace krbn
