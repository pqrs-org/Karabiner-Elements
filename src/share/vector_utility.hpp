#pragma once

#include <vector>

namespace krbn {
namespace vector_utility {
template <typename T>
inline void move_element(std::vector<T>& v, size_t source_index, size_t destination_index) {
  if (source_index >= v.size() ||
      destination_index > v.size() // destination_index == v.size() when moving to the end.
  ) {
    return;
  }

  if (source_index < destination_index) {
    for (size_t i = source_index; i < destination_index - 1; ++i) {
      std::swap(v[i], v[i + 1]);
    }
  } else if (destination_index < source_index) {
    size_t i = source_index - 1;
    while (i >= destination_index) {
      std::swap(v[i], v[i + 1]);

      if (i == 0) {
        break;
      }

      --i;
    }
  }
}
} // namespace vector_utility
} // namespace krbn
