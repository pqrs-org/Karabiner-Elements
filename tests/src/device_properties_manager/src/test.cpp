#include "device_properties_manager.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "device_properties_manager"_test = [] {
    krbn::device_properties_manager manager;

    manager.insert(krbn::device_id(1),
                   krbn::device_properties(krbn::device_id(1), nullptr));
    manager.insert(krbn::device_id(2),
                   krbn::device_properties(krbn::device_id(2), nullptr));
    manager.insert(krbn::device_id(3),
                   std::make_shared<krbn::device_properties>(krbn::device_id(3), nullptr));

    // iokit_device_id(1)

    {
      auto dp = manager.find(krbn::device_id(1));
      expect(dp.get() != nullptr);
      expect(dp->get_device_id() == krbn::device_id(1));
    }

    // iokit_device_id(2)

    {
      auto dp = manager.find(krbn::device_id(2));
      expect(dp.get() != nullptr);
      expect(dp->get_device_id() == krbn::device_id(2));
    }

    // iokit_device_id(3)

    {
      auto dp = manager.find(krbn::device_id(3));
      expect(dp.get() != nullptr);
      expect(dp->get_device_id() == krbn::device_id(3));
    }

    // iokit_device_id(4)

    {
      auto dp = manager.find(krbn::device_id(4));
      expect(dp.get() == nullptr);
    }

    // erase iokit_device_id(2)

    {
      manager.erase(krbn::device_id(2));
      auto dp = manager.find(krbn::device_id(2));
      expect(dp.get() == nullptr);
    }

    // clear

    {
      manager.clear();
      auto dp = manager.find(krbn::device_id(1));
      expect(dp.get() == nullptr);
    }
  };

  return 0;
}
