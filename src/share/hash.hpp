#pragma once

#include <pqrs/hash.hpp>
#include <utility>
#include <vector>

namespace std {
template <typename T, typename A>
struct hash<std::vector<T, A>> final {
  std::size_t operator()(const std::vector<T, A>& value) const {
    std::size_t h = 0;

    for (const auto& v : value) {
      pqrs::hash_combine(h, std::hash<T>{}(v));
    }

    return h;
  }
};

template <typename A, typename B>
struct hash<std::pair<A, B>> final {
  std::size_t operator()(const std::pair<A, B>& value) const {
    std::size_t h = 0;

    pqrs::hash_combine(h, value.first);
    pqrs::hash_combine(h, value.second);

    return h;
  }
};
} // namespace std
