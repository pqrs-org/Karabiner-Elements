#pragma once

#include "filesystem_utility.hpp"
#include "json_utility.hpp"
#include <duk_config.h>
#include <duk_console.h>
#include <duk_module_node.h>
#include <duktape.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <spdlog/fmt/fmt.h>

namespace krbn {
class duktape_eval_error : public std::runtime_error {
public:
  duktape_eval_error(const std::string& message) : std::runtime_error(message) {
  }
};

namespace duktape_utility {
struct eval_string_to_json_result {
  nlohmann::json json;
  std::string log_messages;
};

inline void eval_file(const std::filesystem::path& path) noexcept(false) {
  if (auto code = filesystem_utility::read_file(path)) {
    duk_context* ctx = duk_create_heap_default();

    //
    // console
    //

    duk_console_init(ctx, DUK_CONSOLE_FLUSH);

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
      throw duktape_eval_error(fmt::format("javascript error: {0}",
                                           duk_safe_to_string(ctx, -1)));
    }

    duk_destroy_heap(ctx);
  }
}

inline eval_string_to_json_result eval_string_to_json(const std::string& code) noexcept(false) {
  krbn_duktape_timeout_state timeout_state{
      // 1-second timeout for JavaScript evaluation.
      .deadline_ns = krbn_duktape_get_monotonic_ns() + 1000000000ULL,
      .timed_out = 0,
  };
  duk_context* ctx = duk_create_heap(nullptr, nullptr, nullptr, &timeout_state, nullptr);
  auto log_messages = std::make_shared<std::string>();

  //
  // console
  //

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
        std::string message;
        for (duk_idx_t i = 0; i < n; ++i) {
          if (!message.empty()) {
            message += " ";
          }
          message += duk_safe_to_string(ctx, i);
        }
        if (!log_messages->empty()) {
          log_messages->append("\n");
        }
        log_messages->append(message);
        return 0;
      },
      DUK_VARARGS);
  duk_put_prop_string(ctx, -2, "log");
  duk_put_global_string(ctx, "console");

  //
  // eval
  //

  if (duk_peval_string(ctx, code.c_str()) != 0) {
    auto message = timeout_state.timed_out ? "javascript error: execution timed out"
                                           : fmt::format("javascript error: {0}",
                                                         duk_safe_to_string(ctx, -1));
    duk_destroy_heap(ctx);
    throw duktape_eval_error(message);
  }

  auto json_encode = [](duk_context* ctx, void*) -> duk_ret_t {
    duk_json_encode(ctx, -1);
    return 1;
  };

  if (duk_safe_call(ctx, json_encode, nullptr, 1, 1) != DUK_EXEC_SUCCESS) {
    auto message = fmt::format("javascript error: {0}",
                               duk_safe_to_string(ctx, -1));
    duk_destroy_heap(ctx);
    throw duktape_eval_error(message);
  }

  if (!duk_is_string(ctx, -1)) {
    duk_destroy_heap(ctx);
    throw duktape_eval_error("javascript error: result is not a JSON string");
  }

  std::string json(duk_get_string(ctx, -1));
  duk_destroy_heap(ctx);
  return {
      .json = json_utility::parse_jsonc(json),
      .log_messages = std::move(*log_messages),
  };
}
} // namespace duktape_utility
} // namespace krbn
