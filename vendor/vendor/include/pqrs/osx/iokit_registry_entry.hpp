#pragma once

// pqrs::osx::iokit_service v3.1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/osx/iokit_iterator.hpp>
#include <pqrs/osx/iokit_types.hpp>
#include <utility>

namespace pqrs::osx {
class iokit_registry_entry final {
public:
  //
  // Constructors
  //

  iokit_registry_entry() noexcept
      : iokit_registry_entry(IO_OBJECT_NULL) {
  }

  explicit iokit_registry_entry(io_registry_entry_t registry_entry) noexcept
      : registry_entry_(registry_entry) {
  }

  explicit iokit_registry_entry(const iokit_object_ptr& registry_entry) noexcept
      : registry_entry_(registry_entry) {
  }

  explicit iokit_registry_entry(iokit_object_ptr&& registry_entry) noexcept
      : registry_entry_(std::move(registry_entry)) {
  }

  [[nodiscard]] static iokit_registry_entry get_root_entry(iokit_mach_port::value_t port = iokit_mach_port::null) noexcept {
    return iokit_registry_entry(adopt_iokit_object_ptr(IORegistryGetRootEntry(type_safe::get(port))));
  }

  //
  // Methods
  //

  [[nodiscard]] const iokit_object_ptr& get() const noexcept {
    return registry_entry_;
  }

  [[nodiscard]] iokit_iterator get_child_iterator(const io_name_t plane) const noexcept {
    iokit_iterator result;

    if (registry_entry_) {
      io_iterator_t it = IO_OBJECT_NULL;
      kern_return r = IORegistryEntryGetChildIterator(*registry_entry_, plane, &it);
      if (r) {
        result = adopt_iokit_iterator(it);
      }
    }

    return result;
  }

  [[nodiscard]] iokit_iterator get_parent_iterator(const io_name_t plane) const noexcept {
    iokit_iterator result;

    if (registry_entry_) {
      io_iterator_t it = IO_OBJECT_NULL;
      kern_return r = IORegistryEntryGetParentIterator(*registry_entry_, plane, &it);
      if (r) {
        result = adopt_iokit_iterator(it);
      }
    }

    return result;
  }

  [[nodiscard]] std::optional<std::string> find_location_in_plane(const io_name_t plane) const {
    if (registry_entry_) {
      io_name_t location;
      kern_return r = IORegistryEntryGetLocationInPlane(*registry_entry_, plane, location);
      if (r) {
        return location;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::string> find_name() const {
    if (registry_entry_) {
      io_name_t name;
      kern_return r = IORegistryEntryGetName(*registry_entry_, name);
      if (r) {
        return name;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::string> find_name_in_plane(const io_name_t plane) const {
    if (registry_entry_) {
      io_name_t name;
      kern_return r = IORegistryEntryGetNameInPlane(*registry_entry_, plane, name);
      if (r) {
        return name;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] cf::cf_ptr<CFMutableDictionaryRef> find_properties() const noexcept {
    cf::cf_ptr<CFMutableDictionaryRef> result;

    if (registry_entry_) {
      CFMutableDictionaryRef properties = nullptr;
      kern_return r = IORegistryEntryCreateCFProperties(*registry_entry_, &properties, kCFAllocatorDefault, kNilOptions);
      if (r) {
        result = cf::adopt_cf_ptr(properties);
      }
    }

    return result;
  }

  [[nodiscard]] std::optional<int64_t> find_int64_property(CFStringRef key) const noexcept {
    std::optional<int64_t> result;

    if (registry_entry_) {
      if (auto property = cf::adopt_cf_ptr(IORegistryEntryCreateCFProperty(*registry_entry_, key, kCFAllocatorDefault, kNilOptions))) {
        if (CFGetTypeID(*property) == CFNumberGetTypeID()) {
          int64_t value = 0;
          if (CFNumberGetValue(static_cast<CFNumberRef>(*property), kCFNumberSInt64Type, &value)) {
            result = value;
          }
        }
      }
    }

    return result;
  }

  [[nodiscard]] std::optional<std::string> find_string_property(CFStringRef key) const {
    std::optional<std::string> result;

    if (registry_entry_) {
      if (auto property = cf::adopt_cf_ptr(IORegistryEntryCreateCFProperty(*registry_entry_, key, kCFAllocatorDefault, kNilOptions))) {
        result = cf::make_string(*property);
      }
    }

    return result;
  }

  [[nodiscard]] std::optional<std::string> find_path(const io_name_t plane) const {
    if (registry_entry_) {
      io_string_t path;
      kern_return r = IORegistryEntryGetPath(*registry_entry_, plane, path);
      if (r) {
        return path;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<iokit_registry_entry_id::value_t> find_registry_entry_id() const noexcept {
    if (registry_entry_) {
      uint64_t id;
      kern_return r = IORegistryEntryGetRegistryEntryID(*registry_entry_, &id);
      if (r) {
        return iokit_registry_entry_id::value_t(id);
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] bool in_plane(const io_name_t plane) const noexcept {
    if (registry_entry_) {
      return IORegistryEntryInPlane(*registry_entry_, plane);
    }

    return false;
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return registry_entry_;
  }

private:
  iokit_object_ptr registry_entry_;
};
} // namespace pqrs::osx
