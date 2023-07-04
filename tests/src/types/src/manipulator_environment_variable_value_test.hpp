#include "test.hpp"
#include "types.hpp"
#include <boost/ut.hpp>

void run_manipulator_environment_variable_value_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::string_view_literals;

  "manipulator_environment_variable_value"_test = [] {
    using t = krbn::manipulator_environment_variable_value;

    t v1(10);
    t v2(20);
    t v3(10);
    t v4(true);
    t v5(false);
    t v6("hello");
    t v7("world");
    t v8("hello");
    t v9(0);

    expect(v1 != v2); // int, int
    expect(v1 != v4); // int, bool
    expect(v1 != v6); // int, std::string

    expect(v4 != v5); // bool, bool
    expect(v4 != v6); // bool, std::string

    expect(v6 != v7); // std::string, std::string

    expect(v1 == v3);  // int, int
    expect(v6 == v8);  // std::string, std::string
    expect(v9 == t()); // default value

    expect("10"sv == nlohmann::json(v1).dump());
    expect("20"sv == nlohmann::json(v2).dump());
    expect("true"sv == nlohmann::json(v4).dump());
    expect("false"sv == nlohmann::json(v5).dump());
    expect("\"hello\""sv == nlohmann::json(v6).dump());
    expect("\"world\""sv == nlohmann::json(v7).dump());

    expect(nlohmann::json(v1).get<t>() == v1);
    expect(nlohmann::json(v2).get<t>() == v2);
    expect(nlohmann::json(v3).get<t>() == v3);
    expect(nlohmann::json(v4).get<t>() == v4);
    expect(nlohmann::json(v5).get<t>() == v5);
    expect(nlohmann::json(v6).get<t>() == v6);
    expect(nlohmann::json(v7).get<t>() == v7);
    expect(nlohmann::json(v8).get<t>() == v8);

    // The floating point numbers will be treated as integer.
    expect(nlohmann::json(4.2).get<t>() == t(4));
    expect(nlohmann::json(-4.2).get<t>() == t(-4));

    // errors
    json_unmarshal_error_test<t>(nlohmann::json(),
                                 "json must be number, boolean or string, but is `null`");
    json_unmarshal_error_test<t>(nlohmann::json::array(),
                                 "json must be number, boolean or string, but is `[]`");
    json_unmarshal_error_test<t>(nlohmann::json::object(),
                                 "json must be number, boolean or string, but is `{}`");
  };
}
