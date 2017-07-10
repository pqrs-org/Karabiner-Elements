#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/condition_manager.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("manipulator.manipulator_factory") {
  {
    nlohmann::json json;
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::frontmost_application*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "frontmost_application_if"},
        {
            "bundle_identifiers", {
                                      "^com\\.apple\\.Terminal$", "^com\\.googlecode\\.iterm2$",
                                  },
        },
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::frontmost_application*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "frontmost_application_unless"},
        {
            "bundle_identifiers", {
                                      "^com\\.apple\\.Terminal$", "^com\\.googlecode\\.iterm2$", "broken([regex",
                                  },
        },
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::frontmost_application*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
}

namespace {
class actual_examples_helper final {
public:
  actual_examples_helper(const std::string& file_name) {
    std::ifstream input(std::string("json/") + file_name);
    auto json = nlohmann::json::parse(input);
    for (const auto& j : json) {
      condition_manager_.push_back_condition(krbn::manipulator::manipulator_factory::make_condition(j));
    }
  }

  const krbn::manipulator::condition_manager& get_condition_manager(void) const {
    return condition_manager_;
  }

private:
  krbn::manipulator::condition_manager condition_manager_;
};
} // namespace

TEST_CASE("manipulator.frontmost_application") {
  actual_examples_helper helper("frontmost_application.json");
  krbn::manipulator_environment manipulator_environment;

  // bundle_identifiers matching
  manipulator_environment.get_frontmost_application().set_bundle_identifier("com.apple.Terminal");
  manipulator_environment.get_frontmost_application().set_file_path("/not_found");
  REQUIRE(helper.get_condition_manager().is_fulfilled(manipulator_environment) == true);

  // Test regex escape works properly
  manipulator_environment.get_frontmost_application().set_bundle_identifier("com/apple/Terminal");
  REQUIRE(helper.get_condition_manager().is_fulfilled(manipulator_environment) == false);

  // file_path matching
  manipulator_environment.get_frontmost_application().set_file_path("/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal");
  REQUIRE(helper.get_condition_manager().is_fulfilled(manipulator_environment) == true);

  // frontmost_application_not
  manipulator_environment.get_frontmost_application().set_bundle_identifier("com.googlecode.iterm2");
  manipulator_environment.get_frontmost_application().set_file_path("/Applications/iTerm.app");
  REQUIRE(helper.get_condition_manager().is_fulfilled(manipulator_environment) == true);
  manipulator_environment.get_frontmost_application().set_file_path("/Users/tekezo/Applications/iTerm.app");
  REQUIRE(helper.get_condition_manager().is_fulfilled(manipulator_environment) == false);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
