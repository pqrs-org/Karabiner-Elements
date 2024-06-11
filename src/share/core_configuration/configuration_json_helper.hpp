#pragma once

#include <pqrs/json.hpp>

namespace krbn {
namespace core_configuration {
namespace configuration_json_helper {

class base_t {
public:
  virtual ~base_t() = default;
  virtual const std::string& get_key(void) const = 0;
  virtual void update_value(const nlohmann::json& json) = 0;
  virtual void update_json(nlohmann::json& json) const = 0;
};

template <typename T>
class value_t final : public base_t {
public:
  value_t(const std::string& key,
          T& value,
          const T& default_value)
      : key_(key),
        value_(value),
        default_value_(default_value) {
  }

  const std::string& get_key(void) const override {
    return key_;
  }

  T& get_value(void) const {
    return value_;
  }

  void update_value(const nlohmann::json& json) override {
    if (auto v = pqrs::json::find<T>(json, key_)) {
      value_ = *v;
    }
  }

  void update_json(nlohmann::json& json) const override {
    if (value_ != default_value_) {
      json[key_] = value_;
    } else {
      json.erase(key_);
    }
  }

private:
  std::string key_;
  T& value_;
  const T& default_value_;
};

template <typename T>
class object_t final : public base_t {
public:
  object_t(const std::string& key,
           std::unique_ptr<T>& value)
      : key_(key),
        value_(value) {
  }

  const std::string& get_key(void) const override {
    return key_;
  }

  void update_value(const nlohmann::json& json) override {
    if (auto v = pqrs::json::find_object(json, key_)) {
      value_ = std::make_unique<T>(v->value());
    }
  }

  void update_json(nlohmann::json& json) const override {
    auto j = value_->to_json();
    if (!j.empty()) {
      json[key_] = j;
    } else {
      json.erase(key_);
    }
  }

private:
  std::string key_;
  std::unique_ptr<T>& value_;
};

template <typename T>
class array_t final : public base_t {
public:
  array_t(const std::string& key,
          std::vector<std::unique_ptr<T>>& value)
      : key_(key),
        value_(value) {
  }

  const std::string& get_key(void) const override {
    return key_;
  }

  void update_value(const nlohmann::json& json) override {
    if (!json.contains(key_)) {
      return;
    }

    pqrs::json::requires_array(json[key_], "`" + key_ + "`");

    for (const auto& j : json[key_]) {
      try {
        value_.push_back(std::make_unique<T>(j));
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key_, e.what()));
      }
    }
  }

  void update_json(nlohmann::json& json) const override {
    auto array = nlohmann::json::array();

    for (const auto& v : value_) {
      auto j = v->to_json();
      if (!j.empty()) {
        array.push_back(j);
      }
    }

    if (!array.empty()) {
      json[key_] = array;
    } else {
      json.erase(key_);
    }
  }

private:
  std::string key_;
  std::vector<std::unique_ptr<T>>& value_;
};

} // namespace configuration_json_helper
} // namespace core_configuration
} // namespace krbn
