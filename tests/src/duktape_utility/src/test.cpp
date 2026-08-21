#include "duktape_utility.hpp"
#include <boost/ut.hpp>
#include <optional>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "duktape_utility"_test = [] {
    // Context initialization must not consume the execution budget intended
    // for user JavaScript. Also verify that an unexpected timeout during
    // console setup becomes an exception instead of aborting the process.
    {
      auto context = krbn::duktape_utility::impl::create_context_with_limits();
      expect(context.heap_state->base.timeout.deadline_ns == 0);

      context.heap_state->base.timeout.deadline_ns = 1;
      try {
        krbn::duktape_utility::impl::setup_console(context.ctx.get(),
                                                   *context.heap_state,
                                                   context.log_messages);
        expect(false);
      } catch (krbn::duktape_eval_error&) {
        expect(context.heap_state->base.timeout.timed_out == 1);
      }
    }

    // Duktape keeps a pointer to heap_state as callback data. Verify that the
    // pointer remains valid after the original eval_context has been moved and
    // destroyed.
    {
      std::optional<krbn::duktape_utility::impl::eval_context> moved_context;
      {
        auto context = krbn::duktape_utility::impl::create_context_with_limits();
        moved_context.emplace(std::move(context));
      }

      auto& context = *moved_context;
      krbn::duktape_utility::impl::start_execution_timeout(*context.heap_state);
      expect(duk_peval_string(context.ctx.get(), "1 + 1") == 0);
      expect(duk_get_int(context.ctx.get(), -1) == 2);
    }

    krbn::duktape_utility::eval_file_with_fs_access("data/valid.js");

    try {
      krbn::duktape_utility::eval_file_with_fs_access("data/execution_timeout.js");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: execution timed out"sv == ex.what());
    }

    try {
      krbn::duktape_utility::eval_file_with_fs_access("data/syntax_error.js");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: SyntaxError: parse error (line 2, end of input)"sv == ex.what());
    }

    try {
      krbn::duktape_utility::eval_file_with_fs_access("data/reference_error.js");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: ReferenceError: identifier 'console2' undefined"sv == ex.what());
    }

    try {
      krbn::duktape_utility::eval_file_with_fs_access("data/module_not_found.js");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      auto data_directory = krbn::filesystem_utility::canonical("data");
      if (!data_directory) {
        expect(false);
        return;
      }

      auto expected_message = fmt::format("javascript error: TypeError: cannot find module: {0}/not_found.js",
                                          data_directory->string());
      expect(std::string_view(expected_message) == ex.what());
    }

    {
      auto result = krbn::duktape_utility::eval_string_to_json(R"(
function main() {
  var obj = {};
  for (var i = 1; i <= 3; ++i) {
    console.log("i == " + i);
    obj['key' + i] = i;
  }

  console.log("generated");
  return obj;
}

main();
)");

      auto expected = R"(
{
  "key1":1,
  "key2":2,
  "key3":3
}
      )"_json;
      expect(expected == result.json);

      auto log_messages = "i == 1\n"
                          "i == 2\n"
                          "i == 3\n"
                          "generated"sv;

      expect(log_messages == result.log_messages);
    }

    // Disable console.log.
    {
      auto result = krbn::duktape_utility::eval_string_to_json(R"(
console.log("ignored");
({ description: "example" });
)",
                                                               false);

      expect(nlohmann::json::object({
                 {"description", "example"},
             }) == result.json);
      expect(result.log_messages.empty());
    }

    // Unicode
    {
      auto result = krbn::duktape_utility::eval_string_to_json(R"(
console.log('✅🔥👍')
'✅🔥👍'
)");
      expect(nlohmann::json("✅🔥👍") == result.json);
      expect("✅🔥👍"sv == result.log_messages);
    }

    // Return value that cannot be converted to JSON.
    {
      auto result = krbn::duktape_utility::eval_string_to_json("undefined");
      expect(nlohmann::json() == result.json);
    }

    try {
      krbn::duktape_utility::eval_string_to_json("[");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: SyntaxError: parse error (line 1, end of input)"sv == ex.what());
    }

    try {
      krbn::duktape_utility::eval_string_to_json(R"(while (true) {})");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: execution timed out"sv == ex.what());
    }

    try {
      krbn::duktape_utility::eval_string_to_json(R"(
var a = [];
var i = 0;
while (true) {
  a.push('xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx');
  i += 1;
}
)");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: max memory exceeded"sv == ex.what());
    }

    try {
      krbn::duktape_utility::eval_string_to_json(R"(
function recurse(n) {
  if (n === 0) {
    return 0;
  }
  return 1 + recurse(n - 1);
}

recurse(2000);
)");
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: RangeError: callstack limit"sv == ex.what());
    }

    try {
      std::string large_input(1024 * 1024 + 1, 'x');
      krbn::duktape_utility::eval_string_to_json(large_input);
      expect(false);
    } catch (krbn::duktape_eval_error& ex) {
      expect("javascript error: input too large"sv == ex.what());
    }
  };

  return 0;
}
