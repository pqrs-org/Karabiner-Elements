#pragma once

class device final {
public:
  class identifiers final {
  public:
    identifiers(const nlohmann::json& json) : json_(json),
                                              vendor_id_(vendor_id(0)),
                                              product_id_(product_id(0)),
                                              is_keyboard_(false),
                                              is_pointing_device_(false) {
      {
        const std::string key = "vendor_id";
        if (json.find(key) != json.end() && json[key].is_number()) {
          vendor_id_ = vendor_id(static_cast<uint32_t>(json[key]));
        }
      }
      {
        const std::string key = "product_id";
        if (json.find(key) != json.end() && json[key].is_number()) {
          product_id_ = product_id(static_cast<uint32_t>(json[key]));
        }
      }
      {
        const std::string key = "is_keyboard";
        if (json.find(key) != json.end() && json[key].is_boolean()) {
          is_keyboard_ = json[key];
        }
      }
      {
        const std::string key = "is_pointing_device";
        if (json.find(key) != json.end() && json[key].is_boolean()) {
          is_pointing_device_ = json[key];
        }
      }
    }

    identifiers(vendor_id vendor_id,
                product_id product_id,
                bool is_keyboard,
                bool is_pointing_device) : identifiers(nlohmann::json({
                                               {"vendor_id", static_cast<uint32_t>(vendor_id)},
                                               {"product_id", static_cast<uint32_t>(product_id)},
                                               {"is_keyboard", is_keyboard},
                                               {"is_pointing_device", is_pointing_device},
                                           })) {
    }

    nlohmann::json to_json(void) const {
      auto j = json_;
      j["vendor_id"] = static_cast<uint32_t>(vendor_id_);
      j["product_id"] = static_cast<uint32_t>(product_id_);
      j["is_keyboard"] = is_keyboard_;
      j["is_pointing_device"] = is_pointing_device_;
      return j;
    }

    vendor_id get_vendor_id(void) const {
      return vendor_id_;
    }
    void set_vendor_id(vendor_id value) {
      vendor_id_ = value;
    }

    product_id get_product_id(void) const {
      return product_id_;
    }
    void set_product_id(product_id value) {
      product_id_ = value;
    }

    bool get_is_keyboard(void) const {
      return is_keyboard_;
    }
    void set_is_keyboard(bool value) {
      is_keyboard_ = value;
    }

    bool get_is_pointing_device(void) const {
      return is_pointing_device_;
    }
    void set_is_pointing_device(bool value) {
      is_pointing_device_ = value;
    }

    bool operator==(const identifiers& other) const {
      return vendor_id_ == other.vendor_id_ &&
             product_id_ == other.product_id_ &&
             is_keyboard_ == other.is_keyboard_ &&
             is_pointing_device_ == other.is_pointing_device_;
    }

  private:
    nlohmann::json json_;
    vendor_id vendor_id_;
    product_id product_id_;
    bool is_keyboard_;
    bool is_pointing_device_;
  };

  device(const nlohmann::json& json) : json_(json),
                                       identifiers_(json.find("identifiers") != json.end() ? json["identifiers"] : nlohmann::json()),
                                       ignore_(false),
                                       disable_built_in_keyboard_if_exists_(false),
                                       simple_modifications_(json.find("simple_modifications") != json.end() ? json["simple_modifications"] : nlohmann::json()),
                                       fn_function_keys_(nlohmann::json({
                                           {"f1", ""},
                                           {"f2", ""},
                                           {"f3", ""},
                                           {"f4", ""},
                                           {"f5", ""},
                                           {"f6", ""},
                                           {"f7", ""},
                                           {"f8", ""},
                                           {"f9", ""},
                                           {"f10", ""},
                                           {"f11", ""},
                                           {"f12", ""},
                                       })) {
    {
      const std::string key = "ignore";
      if (json.find(key) != json.end() && json[key].is_boolean()) {
        ignore_ = json[key];
      }
    }
    {
      const std::string key = "disable_built_in_keyboard_if_exists";
      if (json.find(key) != json.end() && json[key].is_boolean()) {
        disable_built_in_keyboard_if_exists_ = json[key];
      }
    }
    {
      const std::string key = "fn_function_keys";
      if (json.find(key) != json.end() && json[key].is_object()) {
        for (auto it = json[key].begin(); it != json[key].end(); ++it) {
          // it.key() is always std::string.
          if (it.value().is_string()) {
            fn_function_keys_.replace_second(it.key(), it.value());
          }
        }
      }
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["identifiers"] = identifiers_;
    j["ignore"] = ignore_;
    j["disable_built_in_keyboard_if_exists"] = disable_built_in_keyboard_if_exists_;
    j["simple_modifications"] = simple_modifications_;
    j["fn_function_keys"] = fn_function_keys_;
    return j;
  }

  const identifiers& get_identifiers(void) const {
    return identifiers_;
  }
  identifiers& get_identifiers(void) {
    return identifiers_;
  }

  bool get_ignore(void) const {
    return ignore_;
  }
  void set_ignore(bool value) {
    ignore_ = value;
  }

  bool get_disable_built_in_keyboard_if_exists(void) const {
    return disable_built_in_keyboard_if_exists_;
  }
  void set_disable_built_in_keyboard_if_exists(bool value) {
    disable_built_in_keyboard_if_exists_ = value;
  }

  const simple_modifications& get_simple_modifications(void) const {
    return simple_modifications_;
  }
  simple_modifications& get_simple_modifications(void) {
    return simple_modifications_;
  }

  const simple_modifications& get_fn_function_keys(void) const {
    return fn_function_keys_;
  }
  simple_modifications& get_fn_function_keys(void) {
    return fn_function_keys_;
  }

private:
  nlohmann::json json_;
  identifiers identifiers_;
  bool ignore_;
  bool disable_built_in_keyboard_if_exists_;
  simple_modifications simple_modifications_;
  simple_modifications fn_function_keys_;
};
