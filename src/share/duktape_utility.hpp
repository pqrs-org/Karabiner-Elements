#pragma once

#include "logger.hpp"
#include <duk_console.h>
#include <duktape.h>

namespace krbn {
namespace duktape_utility {
inline void eval(const std::string_view& code) {
  duk_context* ctx = duk_create_heap_default();

  duk_console_init(ctx, DUK_CONSOLE_FLUSH);

  if (duk_peval_string(ctx, code.data()) != 0) {
    logger::get_logger()->error("javascript error: {0}",
                                duk_safe_to_string(ctx, -1));
  }

  duk_destroy_heap(ctx);
}

inline void eval_file(const std::filesystem::path& path) {
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
