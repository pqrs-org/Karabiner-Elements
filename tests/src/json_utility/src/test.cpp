#include "json_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "json_utility::unmarshal_string"_test = [] {
    //
    // string
    //

    {
      auto json = R"(

"hello"

)"_json;
      expect("hello"s == krbn::json_utility::unmarshal_string("string_key", json));
    }

    //
    // array
    //

    {
      auto json = R"(

[
  "hello",
  "world"
]

)"_json;
      expect("hello\nworld"s == krbn::json_utility::unmarshal_string("array_key", json));
    }

    //
    // unsupported type
    //

    {
      auto json = R"(

42

)"_json;
      try {
        krbn::json_utility::unmarshal_string("number_key", json);
        expect(false) << "json must throw unmarshal_error for unsupported type";
      } catch (const pqrs::json::unmarshal_error& e) {
        expect("`number_key` must be array of string, or string, but is `42`"s == e.what());
      }
    }

    //
    // unsupported type in array
    //

    {
      auto json = R"(

[
      "hello",
      42
]

)"_json;
      try {
        krbn::json_utility::unmarshal_string("number_array_key", json);
        expect(false) << "json must throw unmarshal_error for unsupported type";
      } catch (const pqrs::json::unmarshal_error& e) {
        expect(R"(`number_array_key` must be array of string, or string, but is `["hello",42]`)"s == e.what());
      }
    }
  };

  "json_utility::marshal_string"_test = [] {
    expect(R"(

"hello"

)"_json == krbn::json_utility::marshal_string("hello"));

    expect(R"(

[
  "hello",
  "world"
]

)"_json == krbn::json_utility::marshal_string("hello\nworld"));

    expect(R"(

[
  "hello",
  "",
  "world"
]

)"_json == krbn::json_utility::marshal_string("hello\n\nworld\n"));
  };

  return 0;
}
