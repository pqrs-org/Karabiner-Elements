#include "connected_devices/connected_devices.hpp"
#include <boost/ut.hpp>

void run_descriptions_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "connected_devices::details::descriptions"_test = [] {
    {
      krbn::connected_devices::details::descriptions descriptions1("manufacturer1",
                                                                   "product1",
                                                                   "USB");
      krbn::connected_devices::details::descriptions descriptions2("manufacturer2",
                                                                   "product2",
                                                                   "USB");
      krbn::connected_devices::details::descriptions descriptions3("manufacturer1",
                                                                   "product1",
                                                                   "USB");

      expect(descriptions1.get_manufacturer() == "manufacturer1"sv);
      expect(descriptions1.get_product() == "product1"sv);
      expect(descriptions1.get_transport() == "USB"sv);

      expect(descriptions1.to_json() == nlohmann::json({
                                            {"manufacturer", "manufacturer1"},
                                            {"product", "product1"},
                                            {"transport", "USB"},
                                        }));

      expect(descriptions1 == descriptions3);
      expect(descriptions1 != descriptions2);
    }
    {
      auto descriptions1 = krbn::connected_devices::details::descriptions::make_from_json(
          nlohmann::json(nullptr));
      auto descriptions2 = krbn::connected_devices::details::descriptions::make_from_json(
          nlohmann::json({
              {"manufacturer", "manufacturer2"},
              {"product", "product2"},
              {"transport", "Bluetooth"},
          }));

      expect(descriptions1.get_manufacturer() == ""sv);
      expect(descriptions1.get_product() == ""sv);
      expect(descriptions1.get_transport() == ""sv);

      expect(descriptions2.get_manufacturer() == "manufacturer2"sv);
      expect(descriptions2.get_product() == "product2"sv);
      expect(descriptions2.get_transport() == "Bluetooth"sv);
    }

    // from device_properties

    {
      krbn::device_properties device_properties;
      krbn::connected_devices::details::descriptions descriptions(device_properties);

      expect(descriptions.get_manufacturer() == ""sv);
      expect(descriptions.get_product() == ""sv);
      expect(descriptions.get_transport() == ""sv);
    }
    {
      krbn::device_properties device_properties;
      device_properties
          .set_manufacturer("manufacturer")
          .set_product("product")
          .set_transport("FIFO");
      krbn::connected_devices::details::descriptions descriptions(device_properties);

      expect(descriptions.get_manufacturer() == "manufacturer"sv);
      expect(descriptions.get_product() == "product"sv);
      expect(descriptions.get_transport() == "FIFO"sv);
    }
  };
}
