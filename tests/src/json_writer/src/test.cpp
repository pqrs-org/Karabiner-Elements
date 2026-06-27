#include "filesystem_utility.hpp"
#include "json_writer.hpp"
#include <boost/ut.hpp>
#include <filesystem>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "json_writer"_test = [] {
    constexpr auto permissions_0666 =
        std::filesystem::perms::owner_read |
        std::filesystem::perms::owner_write |
        std::filesystem::perms::group_read |
        std::filesystem::perms::group_write |
        std::filesystem::perms::others_read |
        std::filesystem::perms::others_write;

    krbn::json_writer::save_to_file(nlohmann::json("example1"), "tmp/example", krbn::filesystem_utility::permissions_0600);
    krbn::json_writer::save_to_file(nlohmann::json("example2"), "tmp/example", krbn::filesystem_utility::permissions_0600);
    krbn::json_writer::save_to_file(nlohmann::json("example3"), "tmp/example", krbn::filesystem_utility::permissions_0600);
    krbn::json_writer::save_to_file(nlohmann::json("mode666"), "tmp/mode666", permissions_0666);
    krbn::json_writer::save_to_file(nlohmann::json("mode644"), "tmp/mode644", krbn::filesystem_utility::permissions_0644);
    krbn::json_writer::save_to_file(nlohmann::json("example"), "tmp/not_found/example", krbn::filesystem_utility::permissions_0600);
  };

  return 0;
}
