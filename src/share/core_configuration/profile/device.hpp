#pragma once

class device final {
public:
  device(const nlohmann::json& json) : json_(json),
                                       identifiers_(json.find("identifiers") != json.end() ? json["identifiers"] : nlohmann::json()),
                                       ignore_(false),
                                       disable_built_in_keyboard_if_exists_(false),
                                       simple_modifications_(json.find("simple_modifications") != json.end() ? json["simple_modifications"] : nlohmann::json::array()),
                                       fn_function_keys_(make_default_fn_function_keys_json()) {
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
      if (json.find(key) != json.end()) {
        fn_function_keys_.update(json[key]);
      }
    }
  }

  static nlohmann::json make_default_fn_function_keys_json(void) {
    auto json = nlohmann::json::array();

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f1";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f2";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f3";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f4";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f5";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f6";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f7";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f8";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f9";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f10";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f11";
    json.back()["to"][""] = "";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f12";
    json.back()["to"][""] = "";

    return json;
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

  const device_identifiers& get_identifiers(void) const {
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
  device_identifiers identifiers_;
  bool ignore_;
  bool disable_built_in_keyboard_if_exists_;
  simple_modifications simple_modifications_;
  simple_modifications fn_function_keys_;
};
