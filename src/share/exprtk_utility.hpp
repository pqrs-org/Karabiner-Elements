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
                     const std::unordered_map<std::string, double>& constants,
                     const std::unordered_map<std::string, double&>& variables)
      : expression_string_(expression_string) {
    symbol_table_.add_constants(); // pi, epsilon and inf

    loop_rtc_.loop_set = loop_runtime_check_t::e_all_loops;
    loop_rtc_.max_loop_iterations = 100;
    parser_.register_loop_runtime_check(loop_rtc_);

    reset_expression();
    replace_variables(constants,
                      variables,
                      true);
  }

  bool replace_variables(const std::unordered_map<std::string, double>& new_constants,
                         const std::unordered_map<std::string, double&>& new_variables,
                         bool force_replace = false) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (force_replace ||
        needs_to_replace_symbol_table(new_constants,
                                      new_variables)) {
      //
      // Remove variables that are already registered.
      // (We must remove them first, even when overwriting)
      //

      for (auto&& [name, value] : constants_) {
        symbol_table_.remove_variable(name);
      }
      constants_.clear();

      for (auto&& [name, value] : variables_) {
        symbol_table_.remove_variable(name);
      }
      variables_.clear();

      //
      // Register variables.
      //

      for (auto&& [name, value] : new_constants) {
        symbol_table_.add_constant(name, value);
        constants_.emplace(name, value);
      }

      for (auto&& [name, value] : new_variables) {
        symbol_table_.add_variable(name, value);
        variables_.emplace(name, value);
      }

      // We need to recompile whenever the symbol table changes.
      if (!parser_.compile(expression_string_, expression_)) {
        // If compilation fails, the previous expression_ may remain intact, so explicitly reinitialize it.
        reset_expression();
      }

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

  bool needs_to_replace_symbol_table(const std::unordered_map<std::string, double>& new_constants,
                                     const std::unordered_map<std::string, double&>& new_variables) const {
    //
    // Check for newly added variables or changes to existing values.
    //

    for (auto&& [name, value] : new_constants) {
      auto it = constants_.find(name);
      if (it != std::end(constants_) &&
          value == it->second) {
        continue;
      }

      return true;
    }

    for (auto&& [name, value] : new_variables) {
      auto it = variables_.find(name);
      if (it != std::end(variables_) &&
          &value == &(it->second)) {
        continue;
      }

      return true;
    }

    //
    // Check whether any variables have been deleted.
    //

    if (std::ranges::any_of(constants_,
                            [&](auto&& kv) {
                              return !new_constants.contains(kv.first);
                            })) {
      return true;
    }

    if (std::ranges::any_of(variables_,
                            [&](auto&& kv) {
                              return !new_variables.contains(kv.first);
                            })) {
      return true;
    }

    return false;
  }

  std::string expression_string_;
  symbol_table_t symbol_table_;
  std::unordered_map<std::string, double> constants_;
  std::unordered_map<std::string, double&> variables_;
  loop_runtime_check_t loop_rtc_;
  expression_t expression_;
  parser_t parser_;
  mutable std::mutex mutex_;
};

inline gsl::not_null<std::shared_ptr<expression_wrapper>> compile(const std::string& expression_string,
                                                                  const std::unordered_map<std::string, double>& constants,
                                                                  const std::unordered_map<std::string, double&>& variables) {
  return std::make_shared<expression_wrapper>(expression_string,
                                              constants,
                                              variables);
}

inline double eval(const std::string& expression_string,
                   const std::unordered_map<std::string, double>& constants,
                   const std::unordered_map<std::string, double&>& variables) {
  auto expression = compile(expression_string,
                            constants,
                            variables);
  return expression->value();
}

inline gsl::not_null<std::shared_ptr<expression_wrapper>> make_empty_expression(void) {
  return compile("",
                 {},
                 {});
}

} // namespace exprtk_utility
} // namespace krbn
