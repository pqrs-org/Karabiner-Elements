#include "exprtk_utility.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  if (argc <= 1) {
    std::cout << "Usage:" << std::endl
              << "    " << argv[0] << " expression" << std::endl
              << "Example:" << std::endl
              << "    " << argv[0] << " '42 * 42'" << std::endl;
    return 1;
  }

  std::string expression_string(argv[1]);
  auto expression = krbn::exprtk_utility::compile(expression_string);
  expression->set_variable("example_variable", 42.0);
  expression->set_variable("example_string", "exmaple");

  std::cout << expression_string
            << " = "
            << expression->value()
            << std::endl;

  return 0;
}
