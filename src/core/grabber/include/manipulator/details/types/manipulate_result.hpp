#pragma once

namespace krbn {
namespace manipulator {
namespace details {
enum class manipulate_result {
  passed,
  manipulated,
  needs_wait_until_time_stamp,
};
} // namespace details
} // namespace manipulator
} // namespace krbn
