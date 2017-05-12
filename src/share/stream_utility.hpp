#pragma once

#include <ostream>
#include <string>

namespace krbn {
class stream_utility final {
public:
  template <typename E>
  static std::ostream& output_enum(std::ostream& stream, const E& value) {
    stream << static_cast<typename std::underlying_type<E>::type>(value);
    return stream;
  }

  // For std::vector, etc.

  template <typename E, template <class T, class A> class container>
  static std::ostream& output_enums(std::ostream& stream, const container<E, std::allocator<E>>& values) {
    bool first = true;
    stream << "[";
    for (const auto& v : values) {
      if (first) {
        first = false;
      } else {
        stream << ",";
      }
      stream << v;
    }
    stream << "]";
    return stream;
  }

  // For std::unordered_set, etc.

  template <typename E, template <class T, class H, class K, class A> class container>
  static std::ostream& output_enums(std::ostream& stream, const container<E, std::hash<E>, std::equal_to<E>, std::allocator<E>>& values) {
    bool first = true;
    stream << "[";
    for (const auto& v : values) {
      if (first) {
        first = false;
      } else {
        stream << ",";
      }
      stream << v;
    }
    stream << "]";
    return stream;
  }
};
} // namespace krbn
