#pragma once

// With `exprtk_disable_enhanced_features`, the binary size is reduced by about 4 MB.
// And for simple calculations, such as those used in game_pad_stick_converter, there is no speed reduction.
#define exprtk_disable_enhanced_features
#define exprtk_disable_string_capabilities
#define exprtk_disable_rtl_io
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops

#include <exprtk/exprtk.hpp>

namespace krbn {
namespace exprtk_utility {
typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;

inline double eval(const std::string& expression_string,
                   std::vector<std::pair<std::string, double>> constants) {
  symbol_table_t symbol_table;
  symbol_table.add_constants(); // pi, epsilon and inf
  for (auto&& [name, value] : constants) {
    symbol_table.add_constant(name, value);
  }

  expression_t expression;
  expression.register_symbol_table(symbol_table);

  parser_t parser;
  parser.compile(expression_string, expression);

  return expression.value();
}

inline expression_t compile(const std::string& expression_string,
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

  expression_t expression;
  expression.register_symbol_table(symbol_table);

  parser_t parser;
  parser.compile(expression_string, expression);

  return expression;
}
} // namespace exprtk_utility
} // namespace krbn
