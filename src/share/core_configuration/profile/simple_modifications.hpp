#pragma once

class simple_modifications final {
  
public:
  class key_mapping final {
  public:
    key_mapping(std::string from, std::string to, uint32_t vendor_id, uint32_t product_id, bool disabled = false)
    : from(from), to(to), vid(vendor_id), pid(product_id), disabled(disabled) {
    }
    
    key_mapping(const key_mapping &km) : from(km.get_from()),
                                         to(km.get_to()),
                                         vid(static_cast<uint32_t>(km.get_vendor_id())),
                                         pid(static_cast<uint32_t>(km.get_product_id())),
                                         disabled(km.is_disabled()) {
    }
    
    key_mapping(const nlohmann::json& json) : json(json),
                                              from(""),
                                              to(""),
                                              vid(0),
                                              pid(0),
                                              disabled(false) {
      {
        const std::string key = "from";
        if (json.find(key) != json.end() && json[key].is_string()) {
          from = json[key];
        }
      }
      {
        const std::string key = "to";
        if (json.find(key) != json.end() && json[key].is_string()) {
          to = json[key];
        }
      }
      {
        const std::string key = "vendor_id";
        if (json.find(key) != json.end() && json[key].is_number()) {
          vid = static_cast<uint32_t>(json[key]);
        }
      }
      {
        const std::string key = "product_id";
        if (json.find(key) != json.end() && json[key].is_number()) {
          pid = static_cast<uint32_t>(json[key]);
        }
      }
      {
        const std::string key = "disabled";
        if (json.find(key) != json.end() && json[key].is_boolean()) {
          disabled = json[key];
        }
      }
     
      krbn::glogger::get_logger().info("init key_mapping: {}", this->str());
    }
    
    nlohmann::json to_json(void) const {
      auto j = json;
      j["from"] = from;
      j["to"] = to;
      j["vendor_id"] = static_cast<uint32_t>(vid);
      j["product_id"] = static_cast<uint32_t>(pid);
      j["disabled"] = disabled;
      return j;
    }
    
    vendor_id get_vendor_id(void) const {
      return vendor_id(vid);
    }
    
    void set_vendor_id(vendor_id value) {
      vid = static_cast<uint32_t>(value);
    }
    
    product_id get_product_id(void) const {
      return product_id(pid);
    }
    
    void set_product_id(product_id value) {
      pid = static_cast<uint32_t>(value);
    }
    
    bool is_disabled(void) const {
      return disabled;
    }
    void set_disabled(bool value) {
      disabled = value;
    }
    
    const std::string &get_from(void) const {
      return from;
    }
    void set_from(const std::string &value) {
      from = value;
    }
    
    boost::optional<krbn::key_code> get_from_key_code() const {
      return types::get_key_code(from);
    }
    
    const std::string &get_to(void) const {
      return to;
    }
    void set_to(const std::string &value) {
      to = value;
    }
    
    boost::optional<krbn::key_code> get_to_key_code() const {
      return types::get_key_code(to);
    }
    
    std::string str() {
      return std::string("from: ") + from + ", to: " + to + ", vid/pid: " + std::to_string(vid) + "/" + std::to_string(pid) + ", disabled: " + std::to_string(disabled);
    }

    bool operator==(const key_mapping& other) const {
      return vid == other.vid &&
      pid == other.pid &&
      from == other.from &&
      to == other.to &&
      disabled == other.disabled;
    }
    
    bool operator<(const key_mapping &other) const {
      return from < other.from || to < other.to || vid < other.vid || pid << other.pid || other.disabled;
    }
    
    virtual ~key_mapping() {
      krbn::glogger::get_logger().info("dealloc key_mapping: {}", this->str());
    }
    
    boost::optional<std::pair<key_code, key_code>> to_key_code() const {
      auto& from_string = get_from();
      auto& to_string = get_to();
      
      auto from_key_code = types::get_key_code(from_string);
      if (!from_key_code) {
         krbn::glogger::get_logger().warn("unknown key_code:{0}", from_string);
      }
      
      auto to_key_code = types::get_key_code(to_string);
      if (!to_key_code) {
        krbn::glogger::get_logger().warn("unknown key_code:{0}", to_string);
      }
      
      if (from_key_code && to_key_code) {
        return boost::make_optional(std::make_pair(*from_key_code, *to_key_code));
      } else {
        return boost::none;
      }
    }

  private:
    nlohmann::json json;
    std::string from;
    std::string to;
    uint32_t vid; // vendor_id
    uint32_t pid; // product_id
    bool disabled;
  };
  
  simple_modifications(const nlohmann::json& json) {
    if (json.is_array()) {
      for (auto it = json.begin(); it != json.end(); ++it) {
        auto &obj = *it;
        if (obj.is_object()) {
          pairs_.emplace_back(obj);
        }
      }
    }
  }
  
  nlohmann::json to_json(void) const {
    auto json = nlohmann::json::array();
    for (const auto& it : pairs_) {
      json.push_back(it.to_json());
    }
    return json;
  }
  
  const std::vector<simple_modifications::key_mapping>& get_pairs(void) const {
    return pairs_;
  }
  
  void push_back_pair(void) {
    pairs_.emplace_back("", "", 0, 0, false);
  }
  
  void erase_pair(size_t index) {
    auto it = pairs_.begin() + index;
    if (it != pairs_.end()) {
      pairs_.erase(it);
    }
  }
  
  void replace_pair(size_t index, const std::string& from, const std::string& to) {
    auto it = pairs_.begin() + index;
    if (it != pairs_.end()) {
      it->set_from(from);
      it->set_to(to);
    }
  }
  
  void replace_second(const std::string& from, const std::string& to) {
    for (auto&& it : pairs_) {
      if (it.get_from() == from) {
        it.set_to(to);
        return;
      }
    }
  }
  
  std::unordered_map<key_code, key_code> to_key_code_map(spdlog::logger& log) const {
    std::unordered_map<key_code, key_code> map;
    
    for (const auto& it : pairs_) {
      auto& from_string = it.get_from();
      auto& to_string = it.get_to();
      
      auto from_key_code = types::get_key_code(from_string);
      if (!from_key_code) {
        log.warn("unknown key_code:{0}", from_string);
        continue;
      }
      
      auto to_key_code = types::get_key_code(to_string);
      if (!to_key_code) {
        log.warn("unknown key_code:{0}", to_string);
        continue;
      }
      
      map[*from_key_code] = *to_key_code;
    }
    
    return map;
  }
  
private:
  std::vector<key_mapping> pairs_;
};
