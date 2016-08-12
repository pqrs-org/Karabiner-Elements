#pragma once

#include <cstdint>
#include <string>

class modifier_flag {
public:
  modifier_flag(const std::string& name, const std::string& symbol) : name_(name),
                                                                      symbol_(symbol),
                                                                      count_(0) {}

  const std::string& get_name(void) const { return name_; }
  const std::string& get_symbol(void) const { return symbol_; }

  bool pressed(void) const { return count_ > 0; }

  void reset(void) {
    count_ = 0;
  }

  void increase(void) {
    ++count_;
  }

  void decrease(void) {
    --count_;
  }

private:
  std::string name_;
  std::string symbol_;
  int count_;
};
