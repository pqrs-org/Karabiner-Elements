#pragma once

#include "logger.hpp"
#include "memory_utility.hpp"
#include "types.hpp"
#include <chrono>
#include <gsl/gsl>
#include <pqrs/json.hpp>

namespace krbn {
namespace core_configuration {
namespace configuration_json_helper {

enum class value_origin {
  default_value,
  json,
};

class base_t {
public:
  virtual ~base_t() = default;
  virtual const std::string& get_key(void) const = 0;
  virtual void update_value(const nlohmann::json& json,
                            error_handling error_handling) = 0;
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
        default_value_(default_value),
        value_origin_(value_origin::default_value) {
    value_ = default_value_;
  }

  const std::string& get_key(void) const override {
    return key_;
  }

  T& get_value(void) const {
    return value_;
  }

  const T& get_default_value(void) const {
    return default_value_;
  }

  void set_default_value(const T& value) {
    default_value_ = value;

    if (value_origin_ == value_origin::default_value) {
      value_ = value;
    }
  }

  void update_value(const nlohmann::json& json,
                    error_handling error_handling) override {
    pqrs::json::requires_object(json, "json");

    auto it = json.find(key_);
    if (it == std::end(json)) {
      return;
    }

    try {
      if constexpr (std::is_same<T, std::string>::value) {
        // The string is saved as an array of strings if it is multi-line; otherwise, it is saved as a single string.
        try {
          if (it->is_string()) {
            value_ = it->template get<std::string>();

          } else if (it->is_array()) {
            std::stringstream ss;

            for (const auto& j : *it) {
              ss << j.template get<std::string>() << '\n';
            }

            auto s = ss.str();
            // Remove the last newline.
            if (!s.empty()) {
              s.pop_back();
            }

            value_ = s;

          } else {
            throw std::runtime_error("unknown type");
          }

        } catch (std::exception& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array of string, or string, but is `{1}`",
                                                        key_,
                                                        pqrs::json::dump_for_error_message(*it)));
        }

      } else if constexpr (std::is_same<T, std::chrono::milliseconds>::value) {
        pqrs::json::requires_number(*it, "`" + key_ + "`");

        value_ = std::chrono::milliseconds(it->template get<int>());

      } else {
        if constexpr (std::is_same<T, bool>::value) {
          pqrs::json::requires_boolean(*it, "`" + key_ + "`");
        } else if constexpr (std::is_same<T, int>::value ||
                             std::is_same<T, double>::value) {
          pqrs::json::requires_number(*it, "`" + key_ + "`");
        }

        value_ = it->template get<T>();
      }

      value_origin_ = value_origin::json;

    } catch (std::exception& e) {
      if (error_handling == error_handling::strict) {
        throw;
      } else {
        logger::get_logger()->warn(e.what());
      }
    }
  }

  void update_json(nlohmann::json& json) const override {
    if (value_ != default_value_) {
      if constexpr (std::is_same<T, std::string>::value) {
        // The string is saved as an array of strings if it is multi-line; otherwise, it is saved as a single string.
        std::stringstream ss(value_);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(ss, line, '\n')) {
          lines.push_back(line);
        }

        if (lines.size() == 1) {
          json[key_] = value_;
        } else {
          json[key_] = lines;
        }

      } else if constexpr (std::is_same<T, std::chrono::milliseconds>::value) {
        json[key_] = value_.count();

      } else {
        // For generic values, save them as they are.
        json[key_] = value_;
      }

    } else {
      // Remove it from the JSON if it is the same as the default value.
      json.erase(key_);
    }
  }

private:
  std::string key_;
  T& value_;
  T default_value_;
  value_origin value_origin_;
};

template <typename T>
class object_t final : public base_t {
public:
  object_t(const std::string& key,
           gsl::not_null<std::shared_ptr<T>>& value)
      : key_(key),
        value_(value) {
  }

  const std::string& get_key(void) const override {
    return key_;
  }

  void update_value(const nlohmann::json& json,
                    error_handling error_handling) override {
    auto it = json.find(key_);
    if (it == std::end(json)) {
      return;
    }

    try {
      value_ = std::make_shared<T>(*it,
                                   error_handling);
    } catch (pqrs::json::unmarshal_error& e) {
      throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key_, e.what()));
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
  gsl::not_null<std::shared_ptr<T>>& value_;
};

template <typename T>
class array_t final : public base_t {
public:
  array_t(const std::string& key,
          std::vector<gsl::not_null<std::shared_ptr<T>>>& value)
      : key_(key),
        value_(value) {
  }

  const std::string& get_key(void) const override {
    return key_;
  }

  void update_value(const nlohmann::json& json,
                    error_handling error_handling) override {
    pqrs::json::requires_object(json, "json");

    auto it = json.find(key_);
    if (it == std::end(json)) {
      return;
    }

    pqrs::json::requires_array(*it, "`" + key_ + "`");

    for (const auto& j : *it) {
      try {
        value_.push_back(std::make_shared<T>(j,
                                             error_handling));
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
  std::vector<gsl::not_null<std::shared_ptr<T>>>& value_;
};

class helper_values final {
public:
  template <typename T>
  void push_back_value(const std::string& key,
                       T& value,
                       const T& default_value) {
    values_.push_back(std::make_shared<value_t<T>>(key,
                                                   value,
                                                   default_value));
  }

  template <typename T>
  void push_back_object(const std::string& key,
                        gsl::not_null<std::shared_ptr<T>>& value) {
    values_.push_back(std::make_shared<object_t<T>>(key,
                                                    value));
  }

  template <typename T>
  void push_back_array(const std::string& key,
                       std::vector<gsl::not_null<std::shared_ptr<T>>>& value) {
    values_.push_back(std::make_shared<array_t<T>>(key,
                                                   value));
  }

  void update_value(const nlohmann::json& json,
                    error_handling error_handling) {
    for (const auto& v : values_) {
      v->update_value(json,
                      error_handling);
    }
  }

  void update_json(nlohmann::json& json) const {
    for (const auto& v : values_) {
      v->update_json(json);
    }
  }

  template <typename T>
  std::shared_ptr<value_t<T>> find_value(const T& value) const {
    for (const auto& v : values_) {
      if (auto p = std::dynamic_pointer_cast<value_t<T>>(memory_utility::unwrap_not_null(v))) {
        if (&(p->get_value()) == &value) {
          return p;
        }
      }
    }

    return nullptr;
  }

  template <typename T>
  T find_default_value(const T& value, T fallback_value) const {
    if (auto v = find_value(value)) {
      return v->get_default_value();
    }

    // Unreachable in a normal case
    return fallback_value;
  }

  bool find_default_value(const bool& value) const {
    return find_default_value(value, false);
  }

  double find_default_value(const double& value) const {
    return find_default_value(value, 0.0);
  }

  int find_default_value(const int& value) const {
    return find_default_value(value, 0);
  }

  std::chrono::milliseconds find_default_value(const std::chrono::milliseconds& value) const {
    return find_default_value(value, std::chrono::milliseconds(0));
  }

  std::string find_default_value(const std::string& value) const {
    return find_default_value(value, std::string(""));
  }

  template <typename T>
  void set_default_value(const T& value, const T& default_value) const {
    if (auto v = find_value(value)) {
      v->set_default_value(default_value);
    }
  }

private:
  std::vector<gsl::not_null<std::shared_ptr<base_t>>> values_;
};

} // namespace configuration_json_helper
} // namespace core_configuration
} // namespace krbn
