#pragma once

// `krbn::exprtk_utility::expression_wrapper` can be used safely in a multi-threaded environment.

// With `exprtk_disable_enhanced_features`, the binary size is reduced by about 4 MB.
// And for simple calculations, such as those used in game_pad_stick_converter, there is no speed reduction.
#define exprtk_disable_enhanced_features
#define exprtk_disable_string_capabilities
#define exprtk_disable_rtl_io
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops

#include "logger.hpp"
#include <exprtk/exprtk.hpp>
#include <gsl/gsl>
#include <iostream>

namespace krbn {
namespace exprtk_utility {

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;
typedef exprtk::loop_runtime_check loop_runtime_check_t;
typedef parser_t::unknown_symbol_resolver unknown_symbol_resolver_t;

// Treat undefined variables as 0.
struct zeroing_unknown_symbol_resolver : public unknown_symbol_resolver_t {
  zeroing_unknown_symbol_resolver(void)
      : unknown_symbol_resolver_t(unknown_symbol_resolver_t::e_usrmode_extended) {
  }

  bool process(const std::string& name,
               symbol_table_t& primary,
               std::string& error_message) override {
    // With `create_variable`, exprtk manages the variable's storage internally,
    // so unlike `add_variable` we don't need to manage the backing storage ourselves.
    if (!primary.create_variable(name, 0)) {
      error_message = "failed to create variable: " + name;
      return false;
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

  void set_variable(const std::string& name,
                    double value) {
    std::lock_guard<std::mutex> lock(mutex_);

    set_variable_(name, value);
  }

  void set_variables(const std::vector<std::pair<std::string, double>>& variables) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto&& [name, value] : variables) {
      set_variable_(name, value);
    }
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

  std::string expression_string_;
  symbol_table_t symbol_table_;
  loop_runtime_check_t loop_rtc_;
  zeroing_unknown_symbol_resolver zeroing_unknown_symbol_resolver_;
  expression_t expression_;
  parser_t parser_;

  mutable std::mutex mutex_;
};

inline gsl::not_null<std::shared_ptr<expression_wrapper>> compile(const std::string& expression_string) {
  return std::make_shared<expression_wrapper>(expression_string);
}

inline double eval(const std::string& expression_string,
                   const std::vector<std::pair<std::string, double>>& variables) {
  auto expression = compile(expression_string);
  expression->set_variables(variables);
  return expression->value();
}

} // namespace exprtk_utility
} // namespace krbn
