#pragma once

// `krbn::exprtk_utility::expression_wrapper` can be used safely in a multi-threaded environment.

// With `exprtk_disable_enhanced_features`, the binary size is reduced by about 4 MB.
// And for simple calculations, such as those used in game_pad_stick_converter, there is no speed reduction.
#define exprtk_disable_enhanced_features
#define exprtk_disable_rtl_io
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops

#include "logger.hpp"
#include <exprtk/exprtk.hpp>
#include <iostream>
#include <pqrs/gsl.hpp>

namespace krbn {
namespace exprtk_utility {

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;
typedef exprtk::loop_runtime_check loop_runtime_check_t;
typedef parser_t::unknown_symbol_resolver unknown_symbol_resolver_t;

inline bool is_valid_variable_name(const std::string& name) {
  symbol_table_t t;
  return t.create_variable(name, 0.0);
}

inline bool is_string_variable_name(const std::string& name) {
  return name.ends_with("_string");
}

// Treat undefined variables as 0.
struct zeroing_unknown_symbol_resolver : public unknown_symbol_resolver_t {
  zeroing_unknown_symbol_resolver(void)
      : unknown_symbol_resolver_t(unknown_symbol_resolver_t::e_usrmode_extended) {
  }

  bool process(const std::string& name,
               symbol_table_t& primary,
               std::string& error_message) override {
    // With `create_variable` and `create_stringvar`,
    // exprtk manages the variable's storage internally.
    // So unlike `add_variable` and `add_stringvar`,
    // we don't need to manage the backing storage ourselves.
    if (is_string_variable_name(name)) {
      if (!primary.create_stringvar(name, "")) {
        error_message = "failed to create string variable: " + name;
        return false;
      }
    } else {
      if (!primary.create_variable(name, 0)) {
        error_message = "failed to create variable: " + name;
        return false;
      }
    }
    return true;
  }
};

class expression_wrapper final {
public:
  expression_wrapper(const expression_wrapper&) = delete;

  expression_wrapper(const std::string& expression_string)
      : expression_string_(expression_string) {
    symbol_table_.add_constants(); // pi, epsilon and inf
    expression_.register_symbol_table(symbol_table_);

    loop_rtc_.loop_set = loop_runtime_check_t::e_all_loops;
    loop_rtc_.max_loop_iterations = 100;
    parser_.register_loop_runtime_check(loop_rtc_);

    parser_.enable_unknown_symbol_resolver(&zeroing_unknown_symbol_resolver_);

    parser_.compile(expression_string_, expression_);
  }

  bool set_variable(const std::string& name,
                    double value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_string_variable_name(name)) {
      return false;
    }

    set_variable_(name, value);
    return true;
  }

  bool set_variable(const std::string& name,
                    const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!is_string_variable_name(name)) {
      return false;
    }

    set_string_variable_(name, value);
    return true;
  }

  double value(void) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
      return expression_.value();
    } catch (std::exception& e) {
      logger::get_logger()->error("exprtk error: {0}", e.what());
    }

    return NAN;
  }

private:
  void set_variable_(const std::string& name,
                     double value) {
    if (auto p = symbol_table_.get_variable(name)) {
      p->ref() = value;
    }
  }

  void set_string_variable_(const std::string& name,
                            const std::string& value) {
    if (auto p = symbol_table_.get_stringvar(name)) {
      p->ref() = value;
    }
  }

  std::string expression_string_;
  symbol_table_t symbol_table_;
  loop_runtime_check_t loop_rtc_;
  zeroing_unknown_symbol_resolver zeroing_unknown_symbol_resolver_;
  expression_t expression_;
  parser_t parser_;

  mutable std::mutex mutex_;
};

inline pqrs::not_null_shared_ptr_t<expression_wrapper> compile(const std::string& expression_string) {
  return std::make_shared<expression_wrapper>(expression_string);
}

} // namespace exprtk_utility
} // namespace krbn
