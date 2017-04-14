#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "thread_utility.hpp"
#include "types.hpp"

TEST_CASE("get_key_code") {
  {
    auto actual = krbn::types::get_key_code(krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                            krbn::hid_usage(kHIDUsage_KeyboardTab));
    REQUIRE(*actual == krbn::key_code(kHIDUsage_KeyboardTab));
  }
  {
    auto actual = krbn::types::get_key_code(krbn::hid_usage_page(krbn::kHIDPage_AppleVendorTopCase),
                                            krbn::hid_usage(krbn::kHIDUsage_AV_TopCase_KeyboardFn));
    REQUIRE(*actual == krbn::key_code::fn);
  }
  {
    auto actual = krbn::types::get_key_code(krbn::hid_usage_page(krbn::kHIDPage_AppleVendorKeyboard),
                                            krbn::hid_usage(krbn::kHIDUsage_AppleVendorKeyboard_Function));
    REQUIRE(*actual == krbn::key_code::fn);
  }
  {
    auto actual = krbn::types::get_key_code(krbn::hid_usage_page(kHIDPage_Button),
                                            krbn::hid_usage(1));
    if (actual != boost::none) {
      REQUIRE(false);
    }
  }
}

TEST_CASE("get_pointing_button") {
  {
    auto actual = krbn::types::get_pointing_button(krbn::hid_usage_page(kHIDPage_Button),
                                                   krbn::hid_usage(1));
    REQUIRE(*actual == krbn::pointing_button::button1);
  }
  {
    auto actual = krbn::types::get_pointing_button(krbn::hid_usage_page(kHIDPage_KeyboardOrKeypad),
                                                   krbn::hid_usage(kHIDUsage_KeyboardTab));
    if (actual != boost::none) {
      REQUIRE(false);
    }
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
