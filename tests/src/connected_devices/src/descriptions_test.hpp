#include "connected_devices/connected_devices.hpp"
#include <boost/ut.hpp>

void run_descriptions_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "connected_devices::details::descriptions"_test = [] {
    {
      krbn::connected_devices::details::descriptions descriptions1(pqrs::hid::manufacturer_string::value_t("manufacturer1"),
                                                                   pqrs::hid::product_string::value_t("product1"),
                                                                   "USB");
      krbn::connected_devices::details::descriptions descriptions2(pqrs::hid::manufacturer_string::value_t("manufacturer2"),
                                                                   pqrs::hid::product_string::value_t("product2"),
                                                                   "USB");
      krbn::connected_devices::details::descriptions descriptions3(pqrs::hid::manufacturer_string::value_t("manufacturer1"),
                                                                   pqrs::hid::product_string::value_t("product1"),
                                                                   "USB");

      expect(descriptions1.get_manufacturer() == pqrs::hid::manufacturer_string::value_t("manufacturer1"));
      expect(descriptions1.get_product() == pqrs::hid::product_string::value_t("product1"));
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

      expect(descriptions1.get_manufacturer() == pqrs::hid::manufacturer_string::value_t(""));
      expect(descriptions1.get_product() == pqrs::hid::product_string::value_t(""));
      expect(descriptions1.get_transport() == ""sv);

      expect(descriptions2.get_manufacturer() == pqrs::hid::manufacturer_string::value_t("manufacturer2"));
      expect(descriptions2.get_product() == pqrs::hid::product_string::value_t("product2"));
      expect(descriptions2.get_transport() == "Bluetooth"sv);
    }

    // from device_properties

    {
      krbn::device_properties device_properties;
      krbn::connected_devices::details::descriptions descriptions(device_properties);

      expect(descriptions.get_manufacturer() == pqrs::hid::manufacturer_string::value_t(""));
      expect(descriptions.get_product() == pqrs::hid::product_string::value_t(""));
      expect(descriptions.get_transport() == ""sv);
    }
    {
      krbn::device_properties device_properties;
      device_properties
          .set_manufacturer(pqrs::hid::manufacturer_string::value_t("manufacturer"))
          .set_product(pqrs::hid::product_string::value_t("product"))
          .set_transport("FIFO");
      krbn::connected_devices::details::descriptions descriptions(device_properties);

      expect(descriptions.get_manufacturer() == pqrs::hid::manufacturer_string::value_t("manufacturer"));
      expect(descriptions.get_product() == pqrs::hid::product_string::value_t("product"));
      expect(descriptions.get_transport() == "FIFO"sv);
    }
  };
}
