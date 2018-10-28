#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "cf_ptr.hpp"
#include "cf_utility.hpp"

TEST_CASE("cf_ptr") {
  auto cfstring1 = krbn::cf_utility::create_cfstring("cfstring1");
  auto cfstring2 = krbn::cf_utility::create_cfstring("cfstring2");
  REQUIRE(CFGetRetainCount(cfstring1) == 1);
  REQUIRE(CFGetRetainCount(cfstring2) == 1);

  {
    krbn::cf_ptr<CFStringRef> ptr1(cfstring1);
    REQUIRE(CFGetRetainCount(cfstring1) == 2);
    REQUIRE(ptr1 == true);

    REQUIRE(CFGetRetainCount(ptr1.get()) == 2);
    REQUIRE(CFGetRetainCount(*ptr1) == 2);

    ptr1.reset();
    REQUIRE(CFGetRetainCount(cfstring1) == 1);
    REQUIRE(ptr1 == false);
  }

  REQUIRE(CFGetRetainCount(cfstring1) == 1);

  {
    krbn::cf_ptr<CFStringRef> ptr1(cfstring1);
    REQUIRE(CFGetRetainCount(cfstring1) == 2);

    ptr1 = nullptr;
    REQUIRE(CFGetRetainCount(cfstring1) == 1);
  }

  {
    krbn::cf_ptr<CFStringRef> ptr1(cfstring1);
    REQUIRE(CFGetRetainCount(cfstring1) == 2);
    krbn::cf_ptr<CFStringRef> ptr2(cfstring2);
    REQUIRE(CFGetRetainCount(cfstring2) == 2);

    ptr1 = ptr2;
    REQUIRE(CFGetRetainCount(cfstring1) == 1);
    REQUIRE(CFGetRetainCount(cfstring2) == 3);

    ptr1 = nullptr;
    REQUIRE(CFGetRetainCount(cfstring1) == 1);
    REQUIRE(CFGetRetainCount(cfstring2) == 2);

    ptr2 = nullptr;
    REQUIRE(CFGetRetainCount(cfstring1) == 1);
    REQUIRE(CFGetRetainCount(cfstring2) == 1);
  }

  {
    krbn::cf_ptr<CFStringRef> ptr1(cfstring1);
    REQUIRE(CFGetRetainCount(cfstring1) == 2);

    ptr1 = ptr1;
    REQUIRE(CFGetRetainCount(cfstring1) == 2);
  }

  {
    krbn::cf_ptr<CFStringRef> ptr1(cfstring1);
    REQUIRE(CFGetRetainCount(cfstring1) == 2);

    krbn::cf_ptr<CFStringRef> ptr1_1(ptr1);
    REQUIRE(CFGetRetainCount(cfstring1) == 3);

    ptr1 = nullptr;
    REQUIRE(CFGetRetainCount(cfstring1) == 2);

    ptr1_1 = nullptr;
    REQUIRE(CFGetRetainCount(cfstring1) == 1);
  }
}
