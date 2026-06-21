#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/input_source.hpp>
#include <utility>

namespace pqrs::osx::input_source_selector::impl {
struct entry {
public:
  // You have to call this method in main thread since pqrs::osx::input_source requires it.
  explicit entry(cf::cf_ptr<TISInputSourceRef> input_source_ptr) {
    if (input_source_ptr) {
      properties_ = input_source::properties(*input_source_ptr);
    }

    input_source_ = std::move(input_source_ptr);
  }

  [[nodiscard]] const input_source::properties& get_properties() const noexcept {
    return properties_;
  }

  void select() const {
    if (input_source_) {
      input_source::select(*input_source_);
    }
  }

private:
  cf::cf_ptr<TISInputSourceRef> input_source_;
  input_source::properties properties_;
};
} // namespace pqrs::osx::input_source_selector::impl
