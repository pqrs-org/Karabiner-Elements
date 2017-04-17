#pragma once

#include "types.hpp"

namespace krbn {
namespace manipulator {
class manipulated_keys final {
public:
  class manipulated_key final {
  public:
    manipulated_key(device_id device_id,
                    key_code from_key_code,
                    key_code to_key_code) : device_id_(device_id),
                                            from_key_code_(from_key_code),
                                            to_key_code_(to_key_code) {
    }

    device_id get_device_id(void) const { return device_id_; }

    key_code get_from_key_code(void) const { return from_key_code_; }

    key_code get_to_key_code(void) const { return to_key_code_; }

    bool operator==(const manipulated_key& other) const {
      return get_device_id() == other.get_device_id() &&
             get_from_key_code() == other.get_from_key_code() &&
             get_to_key_code() == other.get_to_key_code();
    }

  private:
    device_id device_id_;
    key_code from_key_code_;
    key_code to_key_code_;
  };

  manipulated_keys(const manipulated_keys&) = delete;

  manipulated_keys(void) {
  }

  void emplace_back(device_id device_id,
                    key_code from_key_code,
                    key_code to_key_code) {
    manipulated_keys_.emplace_back(device_id, from_key_code, to_key_code);
  }

  void erase(device_id device_id,
             key_code from_key_code) {
    manipulated_keys_.erase(std::remove_if(std::begin(manipulated_keys_),
                                           std::end(manipulated_keys_),
                                           [&](const manipulated_key& v) {
                                             return v.get_device_id() == device_id &&
                                                    v.get_from_key_code() == from_key_code;
                                           }),
                            std::end(manipulated_keys_));
  }

  void clear(void) {
    manipulated_keys_.clear();
  }

  const std::vector<manipulated_key>& get_manipulated_keys(void) const {
    return manipulated_keys_;
  }

  boost::optional<key_code> find(device_id device_id,
                                 key_code from_key_code) {
    for (const auto& v : manipulated_keys_) {
      if (v.get_device_id() == device_id &&
          v.get_from_key_code() == from_key_code) {
        return v.get_to_key_code();
      }
    }
    return boost::none;
  }

private:
  std::vector<manipulated_key> manipulated_keys_;
};
} // namespace manipulator
} // namespace krbn
