#pragma once

#include "boost_defs.hpp"

#include <boost/blank.hpp>
#include <boost/functional/hash.hpp>
#include <boost/variant.hpp>
#include <utility>
#include <vector>

namespace boost {
std::size_t hash_value(const boost::blank& v) {
  return 0;
}
} // namespace boost

namespace std {
template <typename T, typename A>
struct hash<std::vector<T, A>> {
  std::size_t operator()(const std::vector<T, A>& v) const {
    return boost::hash<std::vector<T, A>>(v.begin(), v.end());
  }
};

template <typename A, typename B>
struct hash<std::pair<A, B>> {
  std::size_t operator()(const std::pair<A, B>& v) const {
    return boost::hash<std::pair<A, B>>(v);
  }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct hash<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>> {
  std::size_t hash_value(const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>& v) {
    return boost::hash<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>>(v);
  }
};

template <>
struct hash<boost::blank> {
  std::size_t operator()(const boost::blank& v) const {
    return hash_value(v);
  }
};
} // namespace std
