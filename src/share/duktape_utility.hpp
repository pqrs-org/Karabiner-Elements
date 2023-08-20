#pragma once

#include <duk_console.h>
#include <duktape.h>
#include <filesystem>
#include <fstream>
#include <spdlog/fmt/fmt.h>

namespace krbn {
class duktape_eval_error : public std::runtime_error {
public:
  duktape_eval_error(const std::string& message) : std::runtime_error(message) {
  }
};

namespace duktape_utility {
inline void eval(const std::string_view& code) noexcept(false) {
  duk_context* ctx = duk_create_heap_default();

  duk_console_init(ctx, DUK_CONSOLE_FLUSH);

  if (duk_peval_string(ctx, code.data()) != 0) {
    throw duktape_eval_error(fmt::format("javascript error: {0}",
                                         duk_safe_to_string(ctx, -1)));
  }

  duk_destroy_heap(ctx);
}

inline void eval_file(const std::filesystem::path& path) noexcept(false) {
  std::ifstream input(path);
  if (input) {
    std::string code;

    input.seekg(0, std::ios::end);
    code.reserve(input.tellg());
    input.seekg(0, std::ios::beg);

    code.assign(std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>());

    eval(code);
  }
}
} // namespace duktape_utility
} // namespace krbn
