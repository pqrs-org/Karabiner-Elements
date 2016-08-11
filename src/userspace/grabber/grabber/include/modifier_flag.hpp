#pragma once

#include <cstdint>
#include <string>

class modifier_flag {
public:
  modifier_flag(uint8_t bit, const std::string& name, const std::string& symbol) : bit_(bit),
                                                                                   name_(name),
                                                                                   symbol_(symbol) {}

  uint8_t get_bit(void) const { return bit_; }
  const std::string& get_name(void) const { return name_; }
  const std::string& get_symbol(void) const { return symbol_; }

private:
  uint8_t bit_;
  std::string name_;
  std::string symbol_;
};
