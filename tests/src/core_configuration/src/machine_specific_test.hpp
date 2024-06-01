#include "core_configuration/core_configuration.hpp"
#include <boost/ut.hpp>

void run_machine_specific_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "machine_specific"_test = [] {
    krbn::karabiner_machine_identifier id1("krbn-identifier1");
    krbn::karabiner_machine_identifier id2("krbn-identifier2");
    krbn::karabiner_machine_identifier id3("krbn-identifier3");

    // empty json
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::machine_specific machine_specific(json);

      nlohmann::json j(machine_specific);
      expect(nlohmann::json::object() == j);
    }

    // load values from json
    {
      auto json = nlohmann::json::object({
          {"unknown_key1", "unknown_value1"},
          {type_safe::get(id1), nlohmann::json::object({
                                    {"enable_multitouch_extension", true},
                                })},
          {type_safe::get(id3), nlohmann::json::object({
                                    {"unknown_key2", "unknown_value2"},
                                    {"enable_multitouch_extension", true},
                                })},
      });
      krbn::core_configuration::details::machine_specific machine_specific(json);
      expect(true == machine_specific.get_entry(id1).get_enable_multitouch_extension());
      // new entry
      expect(false == machine_specific.get_entry(id2).get_enable_multitouch_extension());
      expect(true == machine_specific.get_entry(id3).get_enable_multitouch_extension());

      {
        nlohmann::json j(machine_specific);
        expect(json == j);
      }

      //
      // set values
      //

      machine_specific.get_entry(id1).set_enable_multitouch_extension(false);
      machine_specific.get_entry(id2).set_enable_multitouch_extension(true);

      // reflect changes to json
      json.erase(type_safe::get(id1));
      json[type_safe::get(id2)] = nlohmann::json::object({
          {"enable_multitouch_extension", true},
      });

      {
        nlohmann::json j(machine_specific);
        expect(json == j);
      }
    }

    // invalid values in json
    {
      auto json = nlohmann::json::object({
          {type_safe::get(id1), nlohmann::json::object({
                                    {"enable_multitouch_extension", 42},
                                })},
      });
      krbn::core_configuration::details::machine_specific machine_specific(json);
      expect(false == machine_specific.get_entry(id1).get_enable_multitouch_extension());
    }
  };
}
