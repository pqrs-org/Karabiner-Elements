#include "json_writer.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "json_writer"_test = [] {
    krbn::json_writer::save_to_file(nlohmann::json("example1"), "tmp/example", 0600);
    krbn::json_writer::save_to_file(nlohmann::json("example2"), "tmp/example", 0600);
    krbn::json_writer::save_to_file(nlohmann::json("example3"), "tmp/example", 0600);
    krbn::json_writer::save_to_file(nlohmann::json("mode666"), "tmp/mode666", 0666);
    krbn::json_writer::save_to_file(nlohmann::json("mode644"), "tmp/mode644", 0644);
    krbn::json_writer::save_to_file(nlohmann::json("example"), "tmp/not_found/example", 0600);
  };

  return 0;
}
