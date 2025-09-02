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

class expression_wrapper final {
public:
  expression_wrapper(const expression_wrapper&) = delete;

  expression_wrapper(const std::string& expression_string,
                     const std::vector<std::pair<std::string, double>>& variables)
      : expression_string_(expression_string) {
    symbol_table_.add_constants(); // pi, epsilon and inf

    loop_rtc_.loop_set = loop_runtime_check_t::e_all_loops;
    loop_rtc_.max_loop_iterations = 100;
    parser_.register_loop_runtime_check(loop_rtc_);

    reset_expression();

    if (!set_variables(variables)) {
      // If compile isn't called automatically inside set_variables, invoke it.
      compile();
    }
  }

  // Return true if the expression was recompiled.
  bool set_variable(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (set_variable_(name, value)) {
      compile();
      return true;
    }

    return false;
  }

  // Return true if the expression was recompiled.
  bool set_variables(const std::vector<std::pair<std::string, double>>& new_variables) {
    std::lock_guard<std::mutex> lock(mutex_);

    bool needs_to_compile = false;

    for (auto&& [name, value] : new_variables) {
      if (set_variable_(name, value)) {
        needs_to_compile = true;
      }
    }

    if (needs_to_compile) {
      compile();
      return true;
    }

    return false;
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
  void reset_expression(void) {
    expression_ = expression_t();
    expression_.register_symbol_table(symbol_table_);
  }

  void compile(void) {
    if (!parser_.compile(expression_string_, expression_)) {
      // If compilation fails, the previous expression_ may remain intact, so explicitly reinitialize it.
      reset_expression();
    }
  }

  // Return true if we need to recompile.
  bool set_variable_(const std::string& name, double value) {
    auto [it, inserted] = variables_.insert_or_assign(name, value);

    if (inserted) {
      symbol_table_.add_variable(name, it->second);
    } else {
      if (auto p = symbol_table_.get_variable(name)) {
        p->ref() = value;
      }
    }

    // We need to recompile whenever the symbol table changes.
    return inserted;
  }

  std::string expression_string_;
  symbol_table_t symbol_table_;
  loop_runtime_check_t loop_rtc_;
  expression_t expression_;
  parser_t parser_;

  // Note:
  // References to data in an unordered_map remain valid as long as the element is not erased.
  //
  // > References and pointers to either key or data stored in the container are only invalidated by erasing that element,
  // > even when the corresponding iterator is invalidated.
  // > https://en.cppreference.com/w/cpp/container/unordered_map.html
  std::unordered_map<std::string, double> variables_;

  mutable std::mutex mutex_;
};

inline gsl::not_null<std::shared_ptr<expression_wrapper>> compile(const std::string& expression_string,
                                                                  const std::vector<std::pair<std::string, double>>& variables) {
  return std::make_shared<expression_wrapper>(expression_string,
                                              variables);
}

inline double eval(const std::string& expression_string,
                   const std::vector<std::pair<std::string, double>>& variables) {
  auto expression = compile(expression_string,
                            variables);
  return expression->value();
}

inline gsl::not_null<std::shared_ptr<expression_wrapper>> make_empty_expression(void) {
  return compile("",
                 {});
}

} // namespace exprtk_utility
} // namespace krbn
