#include "duktape_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "duktape_utility"_test = [] {
    krbn::duktape_utility::eval_file("data/valid.js");

    try {
      krbn::duktape_utility::eval_file("data/syntax_error.js");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: SyntaxError: parse error (line 2, end of input)"sv == ex.what());
    }

    try {
      krbn::duktape_utility::eval_file("data/reference-error.js");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: ReferenceError: identifier 'console2' undefined"sv == ex.what());
    }
  };

  return 0;
}
