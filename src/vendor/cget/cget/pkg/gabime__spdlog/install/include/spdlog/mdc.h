#pragma once

#include <map>
#include <string>

#include <spdlog/common.h>

namespace spdlog {
    class SPDLOG_API mdc {
    public:
        using mdc_map_t = std::map<std::string, std::string>;

        static void put(const std::string &key, const std::string &value) {
            get_context()[key] = value;
        }

        static std::string get(const std::string &key) {
            auto &context = get_context();
            auto it = context.find(key);
            if (it != context.end()) {
                return it->second;
            }
            return "";
        }

        static void remove(const std::string &key) { get_context().erase(key); }

        static void clear() { get_context().clear(); }

        static mdc_map_t &get_context() {
            static thread_local mdc_map_t context;
            return context;
        }
    };

}  // namespace spdlog
