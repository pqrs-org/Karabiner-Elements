#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("input_source_selector") {
  // language
  {
    krbn::input_source_selector selector(std::string("^en$"),
                                         std::nullopt,
                                         std::nullopt);

    {
      nlohmann::json expected;
      expected["language"] = "^en$";
      REQUIRE(selector.to_json() == expected);
    }

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::nullopt)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::nullopt,
                                                         std::nullopt)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    // regex
    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en2"),
                                                         std::nullopt,
                                                         std::nullopt)) == false);
  }

  // input_source_id
  {
    krbn::input_source_selector selector(std::nullopt,
                                         std::string("^com\\.apple\\.keylayout\\.US$"),
                                         std::nullopt);

    {
      nlohmann::json expected;
      expected["input_source_id"] = "^com\\.apple\\.keylayout\\.US$";
      REQUIRE(selector.to_json() == expected);
    }

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::nullopt)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::string("com.apple.keylayout.US"),
                                                         std::nullopt)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    // regex
    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::string("com/apple/keylayout/US"),
                                                         std::nullopt)) == false);
  }

  // input_mode_id
  {
    krbn::input_source_selector selector(std::nullopt,
                                         std::nullopt,
                                         std::string("^com\\.apple\\.inputmethod\\.Japanese\\.FullWidthRoman$"));

    {
      nlohmann::json expected;
      expected["input_mode_id"] = "^com\\.apple\\.inputmethod\\.Japanese\\.FullWidthRoman$";
      REQUIRE(selector.to_json() == expected);
    }

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::nullopt)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    // regex
    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::string("com/apple/inputmethod/Japanese/FullWidthRoman"))) == false);
  }

  // combination
  {
    krbn::input_source_selector selector(std::string("^en$"),
                                         std::string("^com\\.apple\\.keylayout\\.US$"),
                                         std::string("^com\\.apple\\.inputmethod\\.Japanese\\.FullWidthRoman$"));

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::nullopt)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::nullopt,
                                                         std::nullopt)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::string("com.apple.keylayout.US"),
                                                         std::nullopt)) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == false);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    // combination
    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com/apple/keylayout/US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == false);
  }

  // none selector
  {
    krbn::input_source_selector selector(std::nullopt,
                                         std::nullopt,
                                         std::nullopt);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::nullopt)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::nullopt,
                                                         std::nullopt)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::string("com.apple.keylayout.US"),
                                                         std::nullopt)) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::nullopt,
                                                         std::nullopt,
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);

    REQUIRE(selector.test(krbn::input_source_identifiers(std::string("en"),
                                                         std::string("com.apple.keylayout.US"),
                                                         std::string("com.apple.inputmethod.Japanese.FullWidthRoman"))) == true);
  }
}
