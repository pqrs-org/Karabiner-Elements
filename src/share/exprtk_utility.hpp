#pragma once

// With `exprtk_disable_enhanced_features`, the binary size is reduced by about 4 MB.
// And for simple calculations, such as those used in game_pad_stick_converter, there is no speed reduction.
#define exprtk_disable_enhanced_features
#define exprtk_disable_string_capabilities
#define exprtk_disable_rtl_io
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops

#include "logger.hpp"
#include <exprtk/exprtk.hpp>

namespace krbn {
namespace exprtk_utility {

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;
typedef exprtk::loop_runtime_check loop_runtime_check_t;

class expression_wrapper {
public:
  expression_wrapper(std::shared_ptr<expression_t> expression = nullptr)
      : expression_(expression) {
  }

  double value(void) const noexcept {
    if (expression_) {
      try {
        return expression_->value();
      } catch (std::exception& e) {
        logger::get_logger()->error("exprtk error: {0}", e.what());
      }
    }

    return NAN;
  }

private:
  std::shared_ptr<expression_t> expression_;
};

inline expression_wrapper compile(const std::string& expression_string,
                                  std::vector<std::pair<std::string, double>> constants,
                                  std::vector<std::pair<std::string, double&>> variables) {
  symbol_table_t symbol_table;
  symbol_table.add_constants(); // pi, epsilon and inf
  for (auto&& [name, value] : constants) {
    symbol_table.add_constant(name, value);
  }
  for (auto&& [name, value] : variables) {
    symbol_table.add_variable(name, value);
  }

  auto expression = std::make_shared<expression_t>();
  expression->register_symbol_table(symbol_table);

  loop_runtime_check_t loop_rtc;
  loop_rtc.loop_set = loop_runtime_check_t::e_all_loops;
  loop_rtc.max_loop_iterations = 100;

  parser_t parser;
  parser.register_loop_runtime_check(loop_rtc);
  parser.compile(expression_string, *expression);

  return expression_wrapper(expression);
}

inline double eval(const std::string& expression_string,
                   std::vector<std::pair<std::string, double>> constants) {
  auto expression = compile(expression_string,
                            constants,
                            {});

  return expression.value();
}

} // namespace exprtk_utility
} // namespace krbn
