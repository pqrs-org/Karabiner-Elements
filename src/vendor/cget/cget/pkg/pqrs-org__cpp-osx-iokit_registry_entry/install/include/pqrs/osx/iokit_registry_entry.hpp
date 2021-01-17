#pragma once

// pqrs::osx::iokit_service v2.1

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/osx/iokit_iterator.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs {
namespace osx {
class iokit_registry_entry final {
public:
  //
  // Constructors
  //

  iokit_registry_entry(void) : iokit_registry_entry(IO_OBJECT_NULL) {
  }

  explicit iokit_registry_entry(io_registry_entry_t registry_entry) : registry_entry_(registry_entry) {
  }

  explicit iokit_registry_entry(const iokit_object_ptr& registry_entry) : registry_entry_(registry_entry) {
  }

  static iokit_registry_entry get_root_entry(mach_port_t master_port = kIOMasterPortDefault) {
    return iokit_registry_entry(IORegistryGetRootEntry(master_port));
  }

  //
  // Methods
  //

  const iokit_object_ptr& get(void) const {
    return registry_entry_;
  }

  iokit_iterator get_child_iterator(const io_name_t plane) const {
    iokit_iterator result;

    if (registry_entry_) {
      io_iterator_t it = IO_OBJECT_NULL;
      kern_return r = IORegistryEntryGetChildIterator(*registry_entry_, plane, &it);
      if (r) {
        result = iokit_iterator(it);
        IOObjectRelease(it);
      }
    }

    return result;
  }

  iokit_iterator get_parent_iterator(const io_name_t plane) const {
    iokit_iterator result;

    if (registry_entry_) {
      io_iterator_t it = IO_OBJECT_NULL;
      kern_return r = IORegistryEntryGetParentIterator(*registry_entry_, plane, &it);
      if (r) {
        result = iokit_iterator(it);
        IOObjectRelease(it);
      }
    }

    return result;
  }

  std::optional<std::string> find_location_in_plane(const io_name_t plane) const {
    if (registry_entry_) {
      io_name_t location;
      kern_return r = IORegistryEntryGetLocationInPlane(*registry_entry_, plane, location);
      if (r) {
        return location;
      }
    }

    return std::nullopt;
  }

  std::optional<std::string> find_name(void) const {
    if (registry_entry_) {
      io_name_t name;
      kern_return r = IORegistryEntryGetName(*registry_entry_, name);
      if (r) {
        return name;
      }
    }

    return std::nullopt;
  }

  std::optional<std::string> find_name_in_plane(const io_name_t plane) const {
    if (registry_entry_) {
      io_name_t name;
      kern_return r = IORegistryEntryGetNameInPlane(*registry_entry_, plane, name);
      if (r) {
        return name;
      }
    }

    return std::nullopt;
  }

  cf::cf_ptr<CFMutableDictionaryRef> find_properties(void) const {
    cf::cf_ptr<CFMutableDictionaryRef> result;

    if (registry_entry_) {
      CFMutableDictionaryRef properties;
      kern_return r = IORegistryEntryCreateCFProperties(*registry_entry_, &properties, kCFAllocatorDefault, kNilOptions);
      if (r) {
        result = properties;
        CFRelease(properties);
      }
    }

    return result;
  }

  std::optional<int64_t> find_int64_property(CFStringRef key) const {
    std::optional<int64_t> result;

    if (registry_entry_) {
      if (auto property = IORegistryEntryCreateCFProperty(*registry_entry_, key, kCFAllocatorDefault, kNilOptions)) {
        if (CFGetTypeID(property) == CFNumberGetTypeID()) {
          int64_t value = 0;
          if (CFNumberGetValue(static_cast<CFNumberRef>(property), kCFNumberSInt64Type, &value)) {
            result = value;
          }
        }
        CFRelease(property);
      }
    }

    return result;
  }

  std::optional<std::string> find_string_property(CFStringRef key) const {
    std::optional<std::string> result;

    if (registry_entry_) {
      if (auto property = IORegistryEntryCreateCFProperty(*registry_entry_, key, kCFAllocatorDefault, kNilOptions)) {
        result = cf::make_string(property);
        CFRelease(property);
      }
    }

    return result;
  }

  std::optional<std::string> find_path(const io_name_t plane) const {
    if (registry_entry_) {
      io_string_t path;
      kern_return r = IORegistryEntryGetPath(*registry_entry_, plane, path);
      if (r) {
        return path;
      }
    }

    return std::nullopt;
  }

  std::optional<iokit_registry_entry_id::value_t> find_registry_entry_id(void) const {
    if (registry_entry_) {
      uint64_t id;
      kern_return r = IORegistryEntryGetRegistryEntryID(*registry_entry_, &id);
      if (r) {
        return iokit_registry_entry_id::value_t(id);
      }
    }

    return std::nullopt;
  }

  bool in_plane(const io_name_t plane) const {
    if (registry_entry_) {
      return IORegistryEntryInPlane(*registry_entry_, plane);
    }

    return false;
  }

  operator bool(void) const {
    return registry_entry_;
  }

private:
  iokit_object_ptr registry_entry_;
};
} // namespace osx
} // namespace pqrs
