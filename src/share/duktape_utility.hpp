#pragma once

#include "filesystem_utility.hpp"
#include "json_utility.hpp"
#include <cstdlib>
#include <duk_config.h>
#include <duk_console.h>
#include <duk_module_node.h>
#include <duktape.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <spdlog/fmt/fmt.h>
#include <sstream>
#include <utf8cpp/utf8.h>

namespace krbn {
class duktape_eval_error : public std::runtime_error {
public:
  duktape_eval_error(const std::string& message) : std::runtime_error(message) {
  }
};

namespace duktape_utility {
struct eval_heap_state {
  krbn_duktape_heap_state base;
};

namespace impl {
constexpr size_t max_input_bytes = 1ULL * 1024ULL * 1024ULL; // 1MB

inline std::string cesu8_to_utf8(std::string_view s) {
  std::u16string u16;
  u16.reserve(s.size());

  auto it = s.begin();
  auto end = s.end();
  while (it != end) {
    auto cp = utf8::unchecked::next(it);
    if (cp <= 0xFFFF) {
      u16.push_back(static_cast<char16_t>(cp));
    } else {
      cp -= 0x10000;
      u16.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
      u16.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
    }
  }

  std::string out;
  out.reserve(s.size());
  utf8::unchecked::utf16to8(u16.begin(), u16.end(), std::back_inserter(out));
  return out;
}

inline void* alloc(void* udata, duk_size_t size) {
  if (size == 0) {
    return nullptr;
  }
  auto state = static_cast<eval_heap_state*>(udata);
  if (!state) {
    return nullptr;
  }
  if (state->base.memory_used_bytes + size > state->base.memory_limit_bytes) {
    state->base.memory_exceeded = 1;
    return nullptr;
  }

  auto total = size + sizeof(size_t);
  void* raw = std::malloc(total);
  if (!raw) {
    return nullptr;
  }
  *static_cast<size_t*>(raw) = size;
  state->base.memory_used_bytes += size;
  return static_cast<unsigned char*>(raw) + sizeof(size_t);
}

inline void* realloc(void* udata, void* ptr, duk_size_t size) {
  auto state = static_cast<eval_heap_state*>(udata);
  if (!state) {
    return nullptr;
  }

  if (!ptr) {
    return alloc(udata, size);
  }

  if (size == 0) {
    auto raw = static_cast<unsigned char*>(ptr) - sizeof(size_t);
    auto old_size = *reinterpret_cast<size_t*>(raw);
    state->base.memory_used_bytes -= old_size;
    std::free(raw);
    return nullptr;
  }

  auto raw = static_cast<unsigned char*>(ptr) - sizeof(size_t);
  auto old_size = *reinterpret_cast<size_t*>(raw);

  if (state->base.memory_used_bytes - old_size + size > state->base.memory_limit_bytes) {
    state->base.memory_exceeded = 1;
    return nullptr;
  }

  auto total = size + sizeof(size_t);
  void* new_raw = std::realloc(raw, total);
  if (!new_raw) {
    return nullptr;
  }
  *static_cast<size_t*>(new_raw) = size;
  state->base.memory_used_bytes = state->base.memory_used_bytes - old_size + size;
  return static_cast<unsigned char*>(new_raw) + sizeof(size_t);
}

inline void free(void* udata, void* ptr) {
  if (!ptr) {
    return;
  }
  auto state = static_cast<eval_heap_state*>(udata);
  auto raw = static_cast<unsigned char*>(ptr) - sizeof(size_t);
  auto old_size = *reinterpret_cast<size_t*>(raw);
  if (state) {
    state->base.memory_used_bytes -= old_size;
  }
  std::free(raw);
}

inline void setup_console(duk_context* ctx,
                          const std::shared_ptr<std::string>& log_messages) {
  duk_console_init(ctx, DUK_CONSOLE_FLUSH);

  {
    duk_push_heap_stash(ctx);
    duk_push_pointer(ctx, log_messages.get());
    duk_put_prop_string(ctx, -2, "krbn_console_log_messages");
    duk_pop(ctx);
  }

  duk_push_object(ctx);
  duk_push_c_function(
      ctx,
      [](duk_context* ctx) -> duk_ret_t {
        std::string* log_messages = nullptr;
        duk_push_heap_stash(ctx);
        duk_get_prop_string(ctx, -1, "krbn_console_log_messages");
        log_messages = static_cast<std::string*>(duk_get_pointer(ctx, -1));
        duk_pop_2(ctx);

        if (!log_messages) {
          return 0;
        }

        auto n = duk_get_top(ctx);
        std::ostringstream ss;
        for (duk_idx_t i = 0; i < n; ++i) {
          if (i > 0) {
            ss << ' ';
          }
          ss << duk_safe_to_string(ctx, i);
        }
        auto message = cesu8_to_utf8(ss.str());
        if (!log_messages->empty()) {
          log_messages->append("\n");
        }
        log_messages->append(message);
        return 0;
      },
      DUK_VARARGS);
  duk_put_prop_string(ctx, -2, "log");
  duk_put_global_string(ctx, "console");
}

inline std::string build_eval_error_message(const eval_heap_state& heap_state,
                                            duk_context* ctx) {
  if (heap_state.base.memory_exceeded) {
    return "javascript error: max memory exceeded";
  }
  if (heap_state.base.timeout.timed_out) {
    return "javascript error: execution timed out";
  }
  return fmt::format("javascript error: {0}",
                     duk_safe_to_string(ctx, -1));
}

struct eval_context {
  eval_heap_state heap_state;
  duk_context* ctx;
  std::shared_ptr<std::string> log_messages;
};

inline eval_context create_context_with_limits(void) {
  eval_context result{
      .heap_state = {
          .base = {
              // 1-second timeout for JavaScript evaluation.
              .timeout = {
                  .deadline_ns = krbn_duktape_get_monotonic_ns() + 1000000000ULL,
                  .timed_out = 0,
              },
              // 32MB soft cap; actual allocations add sizeof(size_t) per block,
              // so worst-case overhead can be ~8MB with many small allocations.
              .memory_limit_bytes = 32ULL * 1024ULL * 1024ULL,
              .memory_used_bytes = 0,
              .memory_exceeded = 0,
          },
      },
      .ctx = nullptr,
      .log_messages = std::make_shared<std::string>(),
  };

  result.ctx = duk_create_heap(alloc,
                               realloc,
                               free,
                               &result.heap_state,
                               nullptr);
  setup_console(result.ctx, result.log_messages);
  return result;
}

inline void check_input_size(const std::string& code) {
  if (code.size() > max_input_bytes) {
    throw duktape_eval_error("javascript error: input too large");
  }
}
} // namespace impl

struct eval_string_to_json_result {
  nlohmann::json json;
  std::string log_messages;
};

inline std::string eval_file_with_fs_access(const std::filesystem::path& path) noexcept(false) {
  if (auto code = filesystem_utility::read_file(path)) {
    impl::check_input_size(*code);

    auto context = impl::create_context_with_limits();
    duk_context* ctx = context.ctx;

    //
    // module-node
    //

    // Note:
    // `__filename` will be updated in duk_module_node.c when `require()` is called.
    // Since `__filename` is not set in the entry file, set it here.
    duk_push_string(ctx, std::filesystem::absolute(path).c_str());
    duk_put_global_string(ctx, "__filename");

    // A custom extension in order to determine whether a script was executed directly from the command line or loaded with `require()`.
    // Although setting `require.main` is a more nodejs-compatible method, we take this method, which does not require any changes to `duk_module_node.c`.
    duk_push_string(ctx, std::filesystem::absolute(path).c_str());
    duk_put_global_string(ctx, "__main");

    duk_push_object(ctx);

    // resolve
    duk_push_c_function(
        ctx,
        [](duk_context* ctx) {
          auto module_id = duk_require_string(ctx, 0);
          auto parent_id = duk_require_string(ctx, 1);

          if (std::string_view("") == parent_id) {
            duk_get_global_string(ctx, "__main");
            auto filename = std::filesystem::path(duk_to_string(ctx, -1));
            duk_pop(ctx);

            duk_push_sprintf(ctx,
                             "%s/%s.js",
                             filename.parent_path().c_str(),
                             module_id);
          } else {
            auto filename = std::filesystem::path(parent_id);

            duk_push_sprintf(ctx,
                             "%s/%s.js",
                             filename.parent_path().c_str(),
                             module_id);
          }

          return 1;
        },
        DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "resolve");

    // load
    duk_push_c_function(
        ctx,
        [](duk_context* ctx) {
          auto module_id = duk_require_string(ctx, 0);
          duk_get_prop_string(ctx, 2, "filename");
          auto filename = duk_require_string(ctx, -1);

          if (auto code = filesystem_utility::read_file(filename)) {
            duk_push_string(ctx, code->c_str());
          } else {
            duk_type_error(ctx, "cannot find module: %s", module_id);
          }

          return 1;
        },
        DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "load");

    duk_module_node_init(ctx);

    //
    // eval
    //

    if (duk_peval_string(ctx, code->c_str()) != 0) {
      auto message = impl::build_eval_error_message(context.heap_state, ctx);
      duk_destroy_heap(ctx);
      throw duktape_eval_error(message);
    }

    duk_destroy_heap(ctx);
    return std::move(*context.log_messages);
  }
  return "";
}

inline eval_string_to_json_result eval_string_to_json(const std::string& code) noexcept(false) {
  impl::check_input_size(code);

  auto context = impl::create_context_with_limits();
  duk_context* ctx = context.ctx;

  //
  // eval
  //

  if (duk_peval_string(ctx, code.c_str()) != 0) {
    auto message = impl::build_eval_error_message(context.heap_state, ctx);
    duk_destroy_heap(ctx);
    throw duktape_eval_error(message);
  }

  auto json_encode = [](duk_context* ctx, void*) -> duk_ret_t {
    duk_json_encode(ctx, -1);
    return 1;
  };

  nlohmann::json json;
  if (duk_safe_call(ctx, json_encode, nullptr, 1, 1) == DUK_EXEC_SUCCESS) {
    try {
      duk_size_t len = 0;
      auto s = duk_get_lstring(ctx, -1, &len);
      if (s && len > 0) {
        json = json_utility::parse_jsonc(impl::cesu8_to_utf8(std::string_view(s, len)));
      } else {
        json = json_utility::parse_jsonc("");
      }
    } catch (std::exception& e) {
    }
  }

  duk_destroy_heap(ctx);

  return {
      .json = json,
      .log_messages = std::move(*context.log_messages),
  };
}
} // namespace duktape_utility
} // namespace krbn
