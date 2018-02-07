#pragma once

#include "boost_defs.hpp"

#include <boost/functional/hash.hpp>
#include <vector>

namespace std {
template <typename T, typename A>
struct hash<std::vector<T, A>> {
  std::size_t operator()(const std::vector<T, A>& v) const {
    return boost::hash_range(v.begin(), v.end());
  }
};
} // namespace std
