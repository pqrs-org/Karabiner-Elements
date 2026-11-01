#pragma once

namespace krbn::manipulator {
enum class manipulate_result {
  passed,
  manipulated,
  manipulated_without_cancelling_to_if_alone,
  needs_wait_until_time_stamp,
};
} // namespace krbn::manipulator
