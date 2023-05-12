#include "connected_devices/connected_devices.hpp"
#include <boost/ut.hpp>

void run_device_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "connected_devices::details::device"_test = [] {
    {
      krbn::connected_devices::details::descriptions descriptions("manufacturer1",
                                                                  "product1",
                                                                  "USB");
      krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234),
                                           pqrs::hid::product_id::value_t(5678),
                                           "",
                                           true,
                                           false);
      krbn::connected_devices::details::device device(descriptions,
                                                      identifiers,
                                                      true,
                                                      true,
                                                      true);

      expect(device.get_descriptions() == descriptions);
      expect(device.get_identifiers() == identifiers);
      expect(device.get_is_built_in_keyboard() == true);
      expect(device.get_is_built_in_trackpad() == true);
      expect(device.get_is_built_in_touch_bar() == true);

      expect(device.to_json() == nlohmann::json(
                                     {{
                                          "descriptions",
                                          {{
                                               "manufacturer",
                                               "manufacturer1",
                                           },
                                           {
                                               "product",
                                               "product1",
                                           },
                                           {
                                               "transport",
                                               "USB",
                                           }},
                                      },
                                      {
                                          "identifiers",
                                          {
                                              {
                                                  "vendor_id",
                                                  1234,
                                              },
                                              {
                                                  "product_id",
                                                  5678,
                                              },
                                              {
                                                  "device_address",
                                                  "",
                                              },
                                              {
                                                  "is_keyboard",
                                                  true,
                                              },
                                              {
                                                  "is_pointing_device",
                                                  false,
                                              },
                                          },
                                      },
                                      {
                                          "is_built_in_keyboard",
                                          true,
                                      },
                                      {
                                          "is_built_in_trackpad",
                                          true,
                                      },
                                      {
                                          "is_built_in_touch_bar",
                                          true,
                                      }}));
    }
    {
      auto device1 = krbn::connected_devices::details::device::make_from_json(nlohmann::json(nullptr));
      auto device2 = krbn::connected_devices::details::device::make_from_json(nlohmann::json(
          {{
               "descriptions",
               {
                   {
                       "manufacturer",
                       "manufacturer2",
                   },
                   {
                       "product",
                       "product2",
                   },
               },
           },
           {
               "identifiers",
               {
                   {
                       "vendor_id",
                       1234,
                   },
                   {
                       "product_id",
                       5678,
                   },
                   {
                       "is_keyboard",
                       true,
                   },
                   {
                       "is_pointing_device",
                       false,
                   },
               },
           },
           {
               "is_built_in_keyboard",
               true,
           },
           {
               "is_built_in_trackpad",
               true,
           },
           {
               "is_built_in_touch_bar",
               true,
           }}));

      expect(device1.get_descriptions().get_manufacturer() == "");
      expect(device1.get_descriptions().get_product() == "");
      expect(device1.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
      expect(device1.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
      expect(device1.get_identifiers().get_device_address() == "");
      expect(device1.get_identifiers().get_is_keyboard() == false);
      expect(device1.get_identifiers().get_is_pointing_device() == false);
      expect(device1.get_is_built_in_keyboard() == false);
      expect(device1.get_is_built_in_trackpad() == false);
      expect(device1.get_is_built_in_touch_bar() == false);

      expect(device2.get_descriptions().get_manufacturer() == "manufacturer2");
      expect(device2.get_descriptions().get_product() == "product2");
      expect(device2.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(device2.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(device2.get_identifiers().get_is_keyboard() == true);
      expect(device2.get_identifiers().get_is_pointing_device() == false);
      expect(device2.get_is_built_in_keyboard() == true);
      expect(device2.get_is_built_in_trackpad() == true);
      expect(device2.get_is_built_in_touch_bar() == true);
    }

    // from device_properties

    {
      krbn::device_properties device_properties;
      krbn::connected_devices::details::device device(device_properties);

      expect(device.get_descriptions().get_manufacturer() == "");
      expect(device.get_descriptions().get_product() == "");
      expect(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
      expect(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
      expect(device.get_identifiers().get_device_address() == "");
      expect(device.get_identifiers().get_is_keyboard() == false);
      expect(device.get_identifiers().get_is_pointing_device() == false);
      expect(device.get_is_built_in_keyboard() == false);
      expect(device.get_is_built_in_trackpad() == false);
      expect(device.get_is_built_in_touch_bar() == false);
    }
    {
      krbn::device_properties device_properties;
      device_properties
          .set_manufacturer("manufacturer")
          .set_product("product")
          .set(pqrs::hid::vendor_id::value_t(1234))
          .set(pqrs::hid::product_id::value_t(5678))
          .set_device_address("ec-ba-73-21-e6-f4")
          .set_is_keyboard(true);

      {
        krbn::connected_devices::details::device device(device_properties);

        expect(device.get_descriptions().get_manufacturer() == "manufacturer");
        expect(device.get_descriptions().get_product() == "product");
        expect(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
        expect(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
        expect(device.get_identifiers().get_device_address() == "ec-ba-73-21-e6-f4");
        expect(device.get_identifiers().get_is_keyboard() == true);
        expect(device.get_identifiers().get_is_pointing_device() == false);
        expect(device.get_is_built_in_keyboard() == false);
        expect(device.get_is_built_in_trackpad() == false);
        expect(device.get_is_built_in_touch_bar() == false);
      }
    }
  };
}
