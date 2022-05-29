#include "../../share/json_helper.hpp"
#include "state_json_writer.hpp"
#include <boost/ut.hpp>
#include <pqrs/filesystem.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  "state_json_writer"_test = [] {
    system("rm -rf tmp");
    system("mkdir -p tmp");

    {
      krbn::state_json_writer writer("tmp/state.json");

      writer.set("key1", 123);

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_1.json"));

      writer.set("key1", 345);

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_2.json"));

      writer.set("key2", "value123");

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key2_1.json"));
    }

    {
      krbn::state_json_writer writer("tmp/state.json");

      writer.set("key2", "value345");

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key2_2.json"));
    }

    {
      system("echo '[' > tmp/state.json");

      krbn::state_json_writer writer("tmp/state.json");

      writer.set("key1", 123);

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_1.json"));
    }

    //
    // Optional
    //

    {
      std::error_code error_code;
      std::filesystem::remove("tmp/state.json", error_code);

      krbn::state_json_writer writer("tmp/state.json");

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/empty_object.json"));

      writer.set("key1", std::nullopt);

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/empty_object.json"));

      writer.set("key1", 123);

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_1.json"));

      writer.set("key1", std::nullopt);

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/empty_object.json"));

      writer.set("key1", std::optional<int>(123));

      expect(krbn::unit_testing::json_helper::compare_files("tmp/state.json", "data/key1_1.json"));
    }
  };

  return 0;
}
