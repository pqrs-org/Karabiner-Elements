#include "../../share/ut_helper.hpp"
#include "core_configuration/core_configuration.hpp"
#include <boost/ut.hpp>
#include <iostream>

namespace {
class test_object final {
public:
  test_object(const nlohmann::json& json,
              krbn::core_configuration::error_handling error_handling) {
    helper_values.push_back_value<int>("int", i, 42);

    helper_values.update_value(json, error_handling);
  }

  nlohmann::json to_json(void) const {
    auto j = nlohmann::json::object();

    helper_values.update_json(j);

    return j;
  }

  int i;
  krbn::core_configuration::configuration_json_helper::helper_values helper_values;
};

class test_class final {
public:
  test_class(const nlohmann::json& json,
             krbn::core_configuration::error_handling error_handling)
      : o(std::make_shared<test_object>(nlohmann::json::object(), error_handling)) {
    helper_values.push_back_value<bool>("bool", b, true);
    helper_values.push_back_value<double>("double", d, 1042.0);
    helper_values.push_back_value<int>("int", i, 2042);
    helper_values.push_back_value<std::string>("string", s, "42");
    helper_values.push_back_object<test_object>("object", o);
    helper_values.push_back_array<test_object>("array", a);

    helper_values.update_value(json, error_handling);
  }

  nlohmann::json to_json(void) const {
    auto j = nlohmann::json::object();

    helper_values.update_json(j);

    return j;
  }

  bool b;
  double d;
  int i;
  std::string s;
  gsl::not_null<std::shared_ptr<test_object>> o;
  std::vector<gsl::not_null<std::shared_ptr<test_object>>> a;
  krbn::core_configuration::configuration_json_helper::helper_values helper_values;
};
} // namespace

void run_configuration_json_helper_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "empty"_test = [] {
    auto actual = test_class(nlohmann::json::object(), krbn::core_configuration::error_handling::strict);
    expect(true == actual.b);
    expect(1042.0_d == actual.d);
    expect(2042_i == actual.i);
    expect(std::string("42") == actual.s);
    expect(42_i == actual.o->i);
    expect(actual.a.empty());

    {
      auto j = actual.to_json();
      expect(nlohmann::json::object() == j) << UT_SHOW_LINE;
    }
  };

  "update_json"_test = [] {
    auto json = R"(

{
  "bool": false,
  "double": 1.0,
  "int": 2,
  "string": "4242",
  "object": {
    "int": 3
  },
  "array": [
    {
      "int": 4
    }
  ]
}

)"_json;

    auto actual = test_class(json, krbn::core_configuration::error_handling::strict);
    expect(false == actual.b);
    expect(1.0_d == actual.d);
    expect(2_i == actual.i);
    expect(std::string("4242") == actual.s);
    expect(3_i == actual.o->i);
    expect(1_l == actual.a.size());
    expect(4_i == actual.a[0]->i);

    {
      auto j = actual.to_json();
      expect(json == j) << UT_SHOW_LINE;
    }

    // Set default values
    actual.b = true;
    actual.d = 1042.0;
    actual.i = 2042;
    actual.s = "42";
    actual.o->i = 42;
    actual.a.clear();

    {
      auto j = actual.to_json();
      expect(nlohmann::json::object() == j) << UT_SHOW_LINE;
    }
  };

  "find_default_value"_test = [] {
    auto actual = test_class(nlohmann::json::object(), krbn::core_configuration::error_handling::strict);
    expect(true == actual.helper_values.find_default_value(actual.b));
    expect(1042.0_d == actual.helper_values.find_default_value(actual.d));
    expect(2042_i == actual.helper_values.find_default_value(actual.i));
    expect(std::string("42") == actual.helper_values.find_default_value(actual.s));
  };

  "set_default_value"_test = [] {
    //
    // The value of the default is also updated when the default value is changed.
    //

    {
      auto actual = test_class(nlohmann::json::object(), krbn::core_configuration::error_handling::strict);
      expect(2042_i == actual.helper_values.find_default_value(actual.i));
      expect(2042_i == actual.i);

      actual.helper_values.set_default_value(actual.i, 42);
      expect(42_i == actual.helper_values.find_default_value(actual.i));
      expect(42_i == actual.i);

      expect(nlohmann::json::object() == actual.to_json()) << UT_SHOW_LINE;
    }

    //
    // The value set by JSON retains the same value even after the default value is changed.
    //

    {
      auto json = R"(

{
  "int": 2
}

)"_json;

      auto actual = test_class(json, krbn::core_configuration::error_handling::strict);
      expect(2042_i == actual.helper_values.find_default_value(actual.i));
      expect(2_i == actual.i);

      actual.helper_values.set_default_value(actual.i, 42);
      expect(42_i == actual.helper_values.find_default_value(actual.i));
      expect(2_i == actual.i);

      expect(json == actual.to_json()) << UT_SHOW_LINE;
    }

    // JSON value is the old default value
    {
      auto json = R"(

{
  "int": 2042
}

)"_json;

      auto actual = test_class(json, krbn::core_configuration::error_handling::strict);
      expect(2042_i == actual.helper_values.find_default_value(actual.i));
      expect(2042_i == actual.i);

      actual.helper_values.set_default_value(actual.i, 42);
      expect(42_i == actual.helper_values.find_default_value(actual.i));
      expect(2042_i == actual.i);

      expect(json == actual.to_json()) << UT_SHOW_LINE;
    }

    // JSON value is the new default value
    {
      auto json = R"(

{
  "int": 42
}

)"_json;

      auto actual = test_class(json, krbn::core_configuration::error_handling::strict);
      expect(2042_i == actual.helper_values.find_default_value(actual.i));
      expect(42_i == actual.i);

      actual.helper_values.set_default_value(actual.i, 42);
      expect(42_i == actual.helper_values.find_default_value(actual.i));
      expect(42_i == actual.i);

      expect(nlohmann::json::object() == actual.to_json()) << UT_SHOW_LINE;
    }
  };

  //
  // Multi-line string
  //

  {
    auto json = R"(

{
  "string": [
    "1",
    "22",
    "333",
    "4444"
  ]
}

)"_json;

    auto actual = test_class(json, krbn::core_configuration::error_handling::strict);
    expect(std::string("1\n22\n333\n4444") == actual.s);

    expect(json == actual.to_json()) << UT_SHOW_LINE;
  }

  //
  // Invalid values
  //

  // strict
  {
    expect(throws([] {
      auto json = nlohmann::json::parse(R"( { "bool": null } )");
      test_class(json, krbn::core_configuration::error_handling::strict);
    }));
  }
  {
    expect(throws([] {
      auto json = nlohmann::json::parse(R"( { "double": null } )");
      test_class(json, krbn::core_configuration::error_handling::strict);
    }));
  }
  {
    expect(throws([] {
      auto json = nlohmann::json::parse(R"( { "int": null } )");
      test_class(json, krbn::core_configuration::error_handling::strict);
    }));
  }
  {
    expect(throws([] {
      auto json = nlohmann::json::parse(R"( { "string": null } )");
      test_class(json, krbn::core_configuration::error_handling::strict);
    }));
  }

  // loose
  {
    auto json = nlohmann::json::parse(R"( { "bool": null } )");
    auto actual = test_class(json, krbn::core_configuration::error_handling::loose);
    expect(true == actual.b);
  }
  {
    auto json = nlohmann::json::parse(R"( { "double": null } )");
    auto actual = test_class(json, krbn::core_configuration::error_handling::loose);
    expect(1042.0_d == actual.d);
  }
  {
    auto json = nlohmann::json::parse(R"( { "int": null } )");
    auto actual = test_class(json, krbn::core_configuration::error_handling::loose);
    expect(2042_i == actual.i);
  }
  {
    auto json = nlohmann::json::parse(R"( { "string": null } )");
    auto actual = test_class(json, krbn::core_configuration::error_handling::loose);
    expect(std::string("42") == actual.s);
  }
}
