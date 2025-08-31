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

  auto expression = argv[1];

  double example_variable = 242.0;

  std::cout << expression
            << " = "
            << krbn::exprtk_utility::eval(expression,
                                          {
                                              {"example_constant", 142.0},
                                          },
                                          {
                                              {"example_variable", example_variable},
                                          })
            << std::endl;

  return 0;
}
