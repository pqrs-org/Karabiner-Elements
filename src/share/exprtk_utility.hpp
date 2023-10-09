#pragma once

#define exprtk_disable_string_capabilities
#define exprtk_disable_rtl_io
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops

#include <exprtk/exprtk.hpp>

namespace krbn {
namespace exprtk_utility {
inline double eval(const std::string& expression_string,
                   std::vector<std::pair<std::string, double>> constants) {
  exprtk::symbol_table<double> symbol_table;
  symbol_table.add_constants(); // pi, epsilon and inf
  for (auto&& [name, value] : constants) {
    symbol_table.add_constant(name, value);
  }

  exprtk::expression<double> expression;
  expression.register_symbol_table(symbol_table);

  exprtk::parser<double> parser;
  parser.compile(expression_string, expression);

  return expression.value();
}

inline exprtk::expression<double> compile(const std::string& expression_string,
                                          std::vector<std::pair<std::string, double>> constants,
                                          std::vector<std::pair<std::string, double&>> variables) {
  exprtk::symbol_table<double> symbol_table;
  symbol_table.add_constants(); // pi, epsilon and inf
  for (auto&& [name, value] : constants) {
    symbol_table.add_constant(name, value);
  }
  for (auto&& [name, value] : variables) {
    symbol_table.add_variable(name, value);
  }

  exprtk::expression<double> expression;
  expression.register_symbol_table(symbol_table);

  exprtk::parser<double> parser;
  parser.compile(expression_string, expression);

  return expression;
}
} // namespace exprtk_utility
} // namespace krbn
