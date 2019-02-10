<a id="top"></a>
# Compile-time configuration

**Contents**<br>
[main()/ implementation](#main-implementation)<br>
[Reporter / Listener interfaces](#reporter--listener-interfaces)<br>
[Prefixing Catch macros](#prefixing-catch-macros)<br>
[Terminal colour](#terminal-colour)<br>
[Console width](#console-width)<br>
[stdout](#stdout)<br>
[Fallback stringifier](#fallback-stringifier)<br>
[Default reporter](#default-reporter)<br>
[C++11 toggles](#c11-toggles)<br>
[C++17 toggles](#c17-toggles)<br>
[Other toggles](#other-toggles)<br>
[Windows header clutter](#windows-header-clutter)<br>
[Enabling stringification](#enabling-stringification)<br>
[Disabling exceptions](#disabling-exceptions)<br>

Catch is designed to "just work" as much as possible. For most people the only configuration needed is telling Catch which source file should host all the implementation code (```CATCH_CONFIG_MAIN```).

Nonetheless there are still some occasions where finer control is needed. For these occasions Catch exposes a set of macros for configuring how it is built.

## main()/ implementation

    CATCH_CONFIG_MAIN      // Designates this as implementation file and defines main()
    CATCH_CONFIG_RUNNER    // Designates this as implementation file

Although Catch is header only it still, internally, maintains a distinction between interface headers and headers that contain implementation. Only one source file in your test project should compile the implementation headers and this is controlled through the use of one of these macros - one of these identifiers should be defined before including Catch in *exactly one implementation file in your project*.

## Reporter / Listener interfaces

    CATCH_CONFIG_EXTERNAL_INTERFACES  // Brings in necessary headers for Reporter/Listener implementation

Brings in various parts of Catch that are required for user defined Reporters and Listeners. This means that new Reporters and Listeners can be defined in this file as well as in the main file.

Implied by both `CATCH_CONFIG_MAIN` and `CATCH_CONFIG_RUNNER`.

## Prefixing Catch macros

    CATCH_CONFIG_PREFIX_ALL

To keep test code clean and uncluttered Catch uses short macro names (e.g. ```TEST_CASE``` and ```REQUIRE```). Occasionally these may conflict with identifiers from platform headers or the system under test. In this case the above identifier can be defined. This will cause all the Catch user macros to be prefixed with ```CATCH_``` (e.g. ```CATCH_TEST_CASE``` and ```CATCH_REQUIRE```).


## Terminal colour

    CATCH_CONFIG_COLOUR_NONE      // completely disables all text colouring
    CATCH_CONFIG_COLOUR_WINDOWS   // forces the Win32 console API to be used
    CATCH_CONFIG_COLOUR_ANSI      // forces ANSI colour codes to be used

Yes, I am English, so I will continue to spell "colour" with a 'u'.

When sending output to the terminal, if it detects that it can, Catch will use colourised text. On Windows the Win32 API, ```SetConsoleTextAttribute```, is used. On POSIX systems ANSI colour escape codes are inserted into the stream.

For finer control you can define one of the above identifiers (these are mutually exclusive - but that is not checked so may behave unexpectedly if you mix them):

Note that when ANSI colour codes are used "unistd.h" must be includable - along with a definition of ```isatty()```

Typically you should place the ```#define``` before #including "catch.hpp" in your main source file - but if you prefer you can define it for your whole project by whatever your IDE or build system provides for you to do so.

## Console width

    CATCH_CONFIG_CONSOLE_WIDTH = x // where x is a number

Catch formats output intended for the console to fit within a fixed number of characters. This is especially important as indentation is used extensively and uncontrolled line wraps break this.
By default a console width of 80 is assumed but this can be controlled by defining the above identifier to be a different value.

## stdout

    CATCH_CONFIG_NOSTDOUT

To support platforms that do not provide `std::cout`, `std::cerr` and
`std::clog`, Catch does not usem the directly, but rather calls
`Catch::cout`, `Catch::cerr` and `Catch::clog`. You can replace their
implementation by defining `CATCH_CONFIG_NOSTDOUT` and implementing
them yourself, their signatures are:

    std::ostream& cout();
    std::ostream& cerr();
    std::ostream& clog();

[You can see an example of replacing these functions here.](
../examples/231-Cfg-OutputStreams.cpp)


## Fallback stringifier

By default, when Catch's stringification machinery has to stringify
a type that does not specialize `StringMaker`, does not overload `operator<<`,
is not an enumeration and is not a range, it uses `"{?}"`. This can be
overriden by defining `CATCH_CONFIG_FALLBACK_STRINGIFIER` to name of a
function that should perform the stringification instead.

All types that do not provide `StringMaker` specialization or `operator<<`
overload will be sent to this function (this includes enums and ranges).
The provided function must return `std::string` and must accept any type,
e.g. via overloading.

_Note that if the provided function does not handle a type and this type
requires to be stringified, the compilation will fail._


## Default reporter

Catch's default reporter can be changed by defining macro
`CATCH_CONFIG_DEFAULT_REPORTER` to string literal naming the desired
default reporter.

This means that defining `CATCH_CONFIG_DEFAULT_REPORTER` to `"console"`
is equivalent with the out-of-the-box experience.


## C++11 toggles

    CATCH_CONFIG_CPP11_TO_STRING // Use `std::to_string`

Because we support platforms whose standard library does not contain
`std::to_string`, it is possible to force Catch to use a workaround
based on `std::stringstream`. On platforms other than Android,
the default is to use `std::to_string`. On Android, the default is to
use the `stringstream` workaround. As always, it is possible to override
Catch's selection, by defining either `CATCH_CONFIG_CPP11_TO_STRING` or
`CATCH_CONFIG_NO_CPP11_TO_STRING`.


## C++17 toggles

    CATCH_CONFIG_CPP17_UNCAUGHT_EXCEPTIONS  // Use std::uncaught_exceptions instead of std::uncaught_exception
    CATCH_CONFIG_CPP17_STRING_VIEW          // Provide StringMaker specialization for std::string_view
    CATCH_CONFIG_CPP17_VARIANT              // Override C++17 detection for CATCH_CONFIG_ENABLE_VARIANT_STRINGMAKER

Catch contains basic compiler/standard detection and attempts to use
some C++17 features whenever appropriate. This automatic detection
can be manually overridden in both directions, that is, a feature
can be enabled by defining the macro in the table above, and disabled
by using `_NO_` in the macro, e.g. `CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS`.


## Other toggles

    CATCH_CONFIG_COUNTER                    // Use __COUNTER__ to generate unique names for test cases
    CATCH_CONFIG_WINDOWS_SEH                // Enable SEH handling on Windows
    CATCH_CONFIG_FAST_COMPILE               // Sacrifices some (rather minor) features for compilation speed
    CATCH_CONFIG_DISABLE_MATCHERS           // Do not compile Matchers in this compilation unit
    CATCH_CONFIG_POSIX_SIGNALS              // Enable handling POSIX signals
    CATCH_CONFIG_WINDOWS_CRTDBG             // Enable leak checking using Windows's CRT Debug Heap
    CATCH_CONFIG_DISABLE_STRINGIFICATION    // Disable stringifying the original expression
    CATCH_CONFIG_DISABLE                    // Disables assertions and test case registration
    CATCH_CONFIG_WCHAR                      // Enables use of wchart_t
    CATCH_CONFIG_EXPERIMENTAL_REDIRECT      // Enables the new (experimental) way of capturing stdout/stderr

Currently Catch enables `CATCH_CONFIG_WINDOWS_SEH` only when compiled with MSVC, because some versions of MinGW do not have the necessary Win32 API support.

`CATCH_CONFIG_POSIX_SIGNALS` is on by default, except when Catch is compiled under `Cygwin`, where it is disabled by default (but can be force-enabled by defining `CATCH_CONFIG_POSIX_SIGNALS`).

`CATCH_CONFIG_WINDOWS_CRTDBG` is off by default. If enabled, Windows's CRT is used to check for memory leaks, and displays them after the tests finish running.

`CATCH_CONFIG_WCHAR` is on by default, but can be disabled. Currently
it is only used in support for DJGPP cross-compiler.

With the exception of `CATCH_CONFIG_EXPERIMENTAL_REDIRECT`,
these toggles can be disabled by using `_NO_` form of the toggle,
e.g. `CATCH_CONFIG_NO_WINDOWS_SEH`.

### `CATCH_CONFIG_FAST_COMPILE`
This compile-time flag speeds up compilation of assertion macros by ~20%,
by disabling the generation of assertion-local try-catch blocks for
non-exception family of assertion macros ({`REQUIRE`,`CHECK`}{``,`_FALSE`, `_THAT`}).
This disables translation of exceptions thrown under these assertions, but
should not lead to false negatives.

`CATCH_CONFIG_FAST_COMPILE` has to be either defined, or not defined,
in all translation units that are linked into single test binary.

### `CATCH_CONFIG_DISABLE_MATCHERS`
When `CATCH_CONFIG_DISABLE_MATCHERS` is defined, all mentions of Catch's Matchers are ifdef-ed away from the translation unit. Doing so will speed up compilation of that TU.

_Note: If you define `CATCH_CONFIG_DISABLE_MATCHERS` in the same file as Catch's main is implemented, your test executable will fail to link if you use Matchers anywhere._

### `CATCH_CONFIG_DISABLE_STRINGIFICATION`
This toggle enables a workaround for VS 2017 bug. For details see [known limitations](limitations.md#visual-studio-2017----raw-string-literal-in-assert-fails-to-compile).

### `CATCH_CONFIG_DISABLE`
This toggle removes most of Catch from given file. This means that `TEST_CASE`s are not registered and assertions are turned into no-ops. Useful for keeping tests within implementation files (ie for functions with internal linkage), instead of in external files.

This feature is considered experimental and might change at any point.

_Inspired by Doctest's `DOCTEST_CONFIG_DISABLE`_

## Windows header clutter

On Windows Catch includes `windows.h`. To minimize global namespace clutter in the implementation file, it defines `NOMINMAX` and `WIN32_LEAN_AND_MEAN` before including it. You can control this behaviour via two macros:

    CATCH_CONFIG_NO_NOMINMAX            // Stops Catch from using NOMINMAX macro 
    CATCH_CONFIG_NO_WIN32_LEAN_AND_MEAN // Stops Catch from using WIN32_LEAN_AND_MEAN macro


## Enabling stringification

By default, Catch does not stringify some types from the standard library. This is done to avoid dragging in various standard library headers by default. However, Catch does contain these and can be configured to provide them, using these macros:

    CATCH_CONFIG_ENABLE_PAIR_STRINGMAKER     // Provide StringMaker specialization for std::pair
    CATCH_CONFIG_ENABLE_TUPLE_STRINGMAKER    // Provide StringMaker specialization for std::tuple
    CATCH_CONFIG_ENABLE_CHRONO_STRINGMAKER   // Provide StringMaker specialization for std::chrono::duration, std::chrono::timepoint
    CATCH_CONFIG_ENABLE_VARIANT_STRINGMAKER  // Provide StringMaker specialization for std::variant, std::monostate (on C++17)
    CATCH_CONFIG_ENABLE_OPTIONAL_STRINGMAKER // Provide StringMaker specialization for std::optional (on C++17)
    CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS     // Defines all of the above


## Disabling exceptions

By default, Catch2 uses exceptions to signal errors and to abort tests
when an assertion from the `REQUIRE` family of assertions fails. We also
provide an experimental support for disabling exceptions. Catch2 should
automatically detect when it is compiled with exceptions disabled, but
it can be forced to compile without exceptions by defining

    CATCH_CONFIG_DISABLE_EXCEPTIONS

Note that when using Catch2 without exceptions, there are 2 major
limitations:

1) If there is an error that would normally be signalled by an exception,
the exception's message will instead be written to `Catch::cerr` and
`std::terminate` will be called.
2) If an assertion from the `REQUIRE` family of macros fails,
`std::terminate` will be called after the active reporter returns.


There is also a customization point for the exact behaviour of what
happens instead of exception being thrown. To use it, define

    CATCH_CONFIG_DISABLE_EXCEPTIONS_CUSTOM_HANDLER

and provide a definition for this function:

```cpp
namespace Catch {
    [[noreturn]]
    void throw_exception(std::exception const&);
}
```

---

[Home](Readme.md#top)
