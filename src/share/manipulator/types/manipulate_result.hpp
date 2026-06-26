#pragma once

namespace krbn::manipulator {
enum class manipulate_result {
  passed,
  manipulated,
  needs_wait_until_time_stamp,
};
} // namespace krbn::manipulator
