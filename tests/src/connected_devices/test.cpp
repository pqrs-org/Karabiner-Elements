#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "connected_devices.hpp"
#include "thread_utility.hpp"
#include <iostream>
#include <spdlog/spdlog.h>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_color_mt("connected_devices");
      logger->set_level(spdlog::level::off);
    }

    return *logger;
  }
};

TEST_CASE("connected_devices::device::descriptions") {
  {
    krbn::connected_devices::device::descriptions descriptions1("manufacturer1",
                                                                "product1");
    krbn::connected_devices::device::descriptions descriptions2("manufacturer2",
                                                                "product2");
    krbn::connected_devices::device::descriptions descriptions3("manufacturer1",
                                                                "product1");

    REQUIRE(descriptions1.get_manufacturer() == "manufacturer1");
    REQUIRE(descriptions1.get_product() == "product1");

    REQUIRE(descriptions1.to_json() == nlohmann::json({
                                           {"manufacturer", "manufacturer1"},
                                           {"product", "product1"},
                                       }));

    REQUIRE(descriptions1 == descriptions3);
    REQUIRE(descriptions1 != descriptions2);
  }
  {
    krbn::connected_devices::device::descriptions descriptions1(nlohmann::json(nullptr));
    krbn::connected_devices::device::descriptions descriptions2(nlohmann::json({
        {"manufacturer", "manufacturer2"},
        {"product", "product2"},
    }));

    REQUIRE(descriptions1.get_manufacturer() == "");
    REQUIRE(descriptions1.get_product() == "");

    REQUIRE(descriptions2.get_manufacturer() == "manufacturer2");
    REQUIRE(descriptions2.get_product() == "product2");
  }
}

TEST_CASE("connected_devices::device") {
  {
    krbn::connected_devices::device::descriptions descriptions("manufacturer1",
                                                               "product1");
    krbn::core_configuration::profile::device::identifiers identifiers(krbn::vendor_id(1234),
                                                                       krbn::product_id(5678),
                                                                       true,
                                                                       false);
    krbn::connected_devices::device device(descriptions,
                                           identifiers,
                                           true);

    REQUIRE(device.get_descriptions() == descriptions);
    REQUIRE(device.get_identifiers() == identifiers);
    REQUIRE(device.get_is_built_in_keyboard() == true);

    REQUIRE(device.to_json() == nlohmann::json(
                                    {{
                                         "descriptions", {
                                                             {
                                                                 "manufacturer", "manufacturer1",
                                                             },
                                                             {
                                                                 "product", "product1",
                                                             },
                                                         },
                                     },
                                     {
                                         "identifiers", {
                                                            {
                                                                "vendor_id", 1234,
                                                            },
                                                            {
                                                                "product_id", 5678,
                                                            },
                                                            {
                                                                "is_keyboard", true,
                                                            },
                                                            {
                                                                "is_pointing_device", false,
                                                            },
                                                        },
                                     },
                                     {
                                         "is_built_in_keyboard", true,
                                     }}));
  }
  {
    krbn::connected_devices::device device1(nlohmann::json(nullptr));
    krbn::connected_devices::device device2(nlohmann::json(
        {{
             "descriptions", {
                                 {
                                     "manufacturer", "manufacturer2",
                                 },
                                 {
                                     "product", "product2",
                                 },
                             },
         },
         {
             "identifiers", {
                                {
                                    "vendor_id", 1234,
                                },
                                {
                                    "product_id", 5678,
                                },
                                {
                                    "is_keyboard", true,
                                },
                                {
                                    "is_pointing_device", false,
                                },
                            },
         },
         {
             "is_built_in_keyboard", true,
         }}));

    REQUIRE(device1.get_descriptions().get_manufacturer() == "");
    REQUIRE(device1.get_descriptions().get_product() == "");
    REQUIRE(device1.get_identifiers().get_vendor_id() == krbn::vendor_id(0));
    REQUIRE(device1.get_identifiers().get_product_id() == krbn::product_id(0));
    REQUIRE(device1.get_identifiers().get_is_keyboard() == false);
    REQUIRE(device1.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device1.get_is_built_in_keyboard() == false);

    REQUIRE(device2.get_descriptions().get_manufacturer() == "manufacturer2");
    REQUIRE(device2.get_descriptions().get_product() == "product2");
    REQUIRE(device2.get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
    REQUIRE(device2.get_identifiers().get_product_id() == krbn::product_id(5678));
    REQUIRE(device2.get_identifiers().get_is_keyboard() == true);
    REQUIRE(device2.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device2.get_is_built_in_keyboard() == true);
  }
}

TEST_CASE("connected_devices") {
  {
    krbn::connected_devices connected_devices;
    REQUIRE(connected_devices.to_json() == nlohmann::json::array());
  }
  {
    krbn::connected_devices connected_devices;

    {
      krbn::connected_devices::device::descriptions descriptions("manufacturer1",
                                                                 "product1");
      krbn::core_configuration::profile::device::identifiers identifiers(krbn::vendor_id(1234),
                                                                         krbn::product_id(5678),
                                                                         true,
                                                                         false);
      krbn::connected_devices::device device(descriptions,
                                             identifiers,
                                             true);
      connected_devices.push_back_device(device);
    }
    {
      krbn::connected_devices::device::descriptions descriptions("manufacturer1 (ignored)",
                                                                 "product1 (ignored)");
      krbn::core_configuration::profile::device::identifiers identifiers(krbn::vendor_id(1234),
                                                                         krbn::product_id(5678),
                                                                         true,
                                                                         false);
      krbn::connected_devices::device device(descriptions,
                                             identifiers,
                                             true);
      connected_devices.push_back_device(device);
    }
    {
      krbn::connected_devices::device::descriptions descriptions("manufacturer3",
                                                                 "product3");
      krbn::core_configuration::profile::device::identifiers identifiers(krbn::vendor_id(2345),
                                                                         krbn::product_id(6789),
                                                                         false,
                                                                         true);
      krbn::connected_devices::device device(descriptions,
                                             identifiers,
                                             false);
      connected_devices.push_back_device(device);
    }
    {
      krbn::connected_devices::device::descriptions descriptions("manufacturer2",
                                                                 "product2");
      krbn::core_configuration::profile::device::identifiers identifiers(krbn::vendor_id(1234),
                                                                         krbn::product_id(5679),
                                                                         false,
                                                                         true);
      krbn::connected_devices::device device(descriptions,
                                             identifiers,
                                             false);
      connected_devices.push_back_device(device);
    }

    REQUIRE(connected_devices.is_loaded() == true);
    REQUIRE(connected_devices.get_devices().size() == 3);
    REQUIRE(connected_devices.get_devices()[0].get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
    REQUIRE(connected_devices.get_devices()[0].get_identifiers().get_product_id() == krbn::product_id(5678));
    REQUIRE(connected_devices.get_devices()[1].get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
    REQUIRE(connected_devices.get_devices()[1].get_identifiers().get_product_id() == krbn::product_id(5679));
    REQUIRE(connected_devices.get_devices()[2].get_identifiers().get_vendor_id() == krbn::vendor_id(2345));
    REQUIRE(connected_devices.get_devices()[2].get_identifiers().get_product_id() == krbn::product_id(6789));

    std::ifstream ifs("json/connected_devices.json");

    REQUIRE(connected_devices.to_json() == nlohmann::json::parse(ifs));
  }

  {
    krbn::connected_devices connected_devices(logger::get_logger(), "json/connected_devices.json");

    REQUIRE(connected_devices.is_loaded() == true);
    REQUIRE(connected_devices.get_devices().size() == 3);
    REQUIRE(connected_devices.get_devices()[0].get_is_built_in_keyboard() == true);
    REQUIRE(connected_devices.get_devices()[1].get_is_built_in_keyboard() == false);
  }

  {
    krbn::connected_devices connected_devices(logger::get_logger(), "json/broken.json");

    REQUIRE(connected_devices.is_loaded() == false);
    REQUIRE(connected_devices.get_devices().size() == 0);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
