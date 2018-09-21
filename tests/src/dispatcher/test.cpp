#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "dispatcher.hpp"

TEST_CASE("object_id") {
  {
    krbn::dispatcher::object_id object_id1(krbn::dispatcher::object_id::make_object_id());
    krbn::dispatcher::object_id object_id2(krbn::dispatcher::object_id::make_object_id());

    REQUIRE(object_id1.get() == 1);
    REQUIRE(object_id2.get() == 2);
    REQUIRE(krbn::dispatcher::object_id::active_object_id_count() == 2);
  }

  REQUIRE(krbn::dispatcher::object_id::active_object_id_count() == 0);

  {
    krbn::dispatcher::object_id object_id3(krbn::dispatcher::object_id::make_object_id());
    krbn::dispatcher::object_id object_id4(krbn::dispatcher::object_id::make_object_id());

    REQUIRE(object_id3.get() == 3);
    REQUIRE(object_id4.get() == 4);
    REQUIRE(krbn::dispatcher::object_id::active_object_id_count() == 2);
  }
}
