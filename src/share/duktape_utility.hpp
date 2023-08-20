#pragma once

#include "filesystem_utility.hpp"
#include <duk_console.h>
#include <duk_module_node.h>
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

    duk_push_string(ctx, path.c_str());
    duk_put_global_string(ctx, "__filename");

    duk_push_object(ctx);

    // resolve
    duk_push_c_function(
        ctx,
        [](duk_context* ctx) {
          auto module_id = duk_require_string(ctx, 0);
          auto parent_id = duk_require_string(ctx, 1);

          if (std::string_view("") == parent_id) {
            duk_get_global_string(ctx, "__filename");
            auto filename = duk_to_string(ctx, -1);

            duk_push_sprintf(ctx,
                             "%s/%s.js",
                             std::filesystem::path(filename).parent_path().c_str(),
                             module_id);
          } else {
            duk_push_sprintf(ctx,
                             "%s/%s.js",
                             std::filesystem::path(parent_id).parent_path().c_str(),
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
} // namespace duktape_utility
} // namespace krbn
