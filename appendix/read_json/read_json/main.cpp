#include "boost_defs.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#include <boost/property_tree/json_parser.hpp>
#pragma clang diagnostic pop

#include <iostream>
#include <spdlog/spdlog.h>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("read_json", true);
    }
    return *logger;
  }
};

int main(int argc, const char* argv[]) {
  boost::property_tree::ptree pt;
  boost::property_tree::read_json("example.json", pt);

  for (const auto& it : pt.get_child("profiles")) {
    logger::get_logger().info("profile");

    auto& profile = it.second;

    if (auto name = profile.get_optional<std::string>("name")) {
      logger::get_logger().info("name {0}", *name);
    }
  }

  return 0;
}
