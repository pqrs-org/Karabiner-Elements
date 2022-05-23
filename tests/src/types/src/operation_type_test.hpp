#include "types.hpp"
#include <boost/ut.hpp>

void run_operation_type_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "operation_type json"_test = [] {
    for (uint8_t i = static_cast<uint8_t>(krbn::operation_type::none);
         i < static_cast<uint8_t>(krbn::operation_type::end_);
         ++i) {
      auto t = krbn::operation_type(i);

      nlohmann::json json = t;
      expect(json.get<krbn::operation_type>() == t);
    }
  };
}
