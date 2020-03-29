//======================================================================//
// Copyright (C) 2016-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
//
// This software is provided 'as-is', without any express or
// implied warranty. In no event will the authors be held
// liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute
// it freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but
//    is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any
//    source distribution.
//======================================================================//

#ifndef DEBUG_ASSERT_HPP_INCLUDED
#define DEBUG_ASSERT_HPP_INCLUDED

#include <cstdlib>

#ifndef DEBUG_ASSERT_NO_STDIO
#include <cstdio>
#endif

#ifndef DEBUG_ASSERT_MARK_UNREACHABLE
#ifdef __GNUC__
#define DEBUG_ASSERT_MARK_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#define DEBUG_ASSERT_MARK_UNREACHABLE __assume(0)
#else
/// Hint to the compiler that a code branch is unreachable.
/// Define it yourself prior to including the header to override it.
/// \notes This must be usable in an expression.
#define DEBUG_ASSERT_MARK_UNREACHABLE
#endif
#endif

#ifndef DEBUG_ASSERT_PURE_FUNCTION
#ifdef __GNUC__
#define DEBUG_ASSERT_PURE_FUNCTION __attribute__((pure))
#else
/// Hint to the compiler that a function is pure.
#define DEBUG_ASSERT_PURE_FUNCTION
#endif
#endif

#ifndef DEBUG_ASSERT_FORCE_INLINE
#ifdef __GNUC__
#define DEBUG_ASSERT_FORCE_INLINE [[gnu::always_inline]] inline
#elif defined(_MSC_VER)
#define DEBUG_ASSERT_FORCE_INLINE __forceinline
#else
/// Strong hint to the compiler to inline a function.
/// Define it yourself prior to including the header to override it.
#define DEBUG_ASSERT_FORCE_INLINE inline
#endif
#endif

namespace debug_assert
{
    //=== source location ===//
    /// Defines a location in the source code.
    struct source_location
    {
        const char* file_name;   ///< The file name.
        unsigned    line_number; ///< The line number.
    };

/// Expands to the current [debug_assert::source_location]().
#define DEBUG_ASSERT_CUR_SOURCE_LOCATION                                                           \
    debug_assert::source_location                                                                  \
    {                                                                                              \
        __FILE__, static_cast<unsigned>(__LINE__)                                                  \
    }

    //=== level ===//
    /// Tag type to indicate the level of an assertion.
    template <unsigned Level>
    struct level
    {
    };

    /// Helper class that sets a certain level.
    /// Inherit from it in your module handler.
    template <unsigned Level>
    struct set_level
    {
        static const unsigned level = Level;
    };

    template <unsigned Level>
    const unsigned set_level<Level>::level;

    /// Helper class that controls whether the handler can throw or not.
    /// Inherit from it in your module handler.
    /// If the module does not inherit from this class, it is assumed that
    /// the handle does not throw.
    struct allow_exception
    {
        static const bool throwing_exception_is_allowed = true;
    };

    //=== handler ===//
    /// Does not do anything to handle a failed assertion (except calling
    /// [std::abort()]()).
    /// Inherit from it in your module handler.
    struct no_handler
    {
        /// \effects Does nothing.
        /// \notes Can take any additional arguments.
        template <typename... Args>
        static void handle(const source_location&, const char*, Args&&...) noexcept
        {
        }
    };

    /// The default handler that writes a message to `stderr`.
    /// Inherit from it in your module handler.
    struct default_handler
    {
        /// \effects Prints a message to `stderr`.
        /// \notes It can optionally accept an additional message string.
        /// \notes If `DEBUG_ASSERT_NO_STDIO` is defined, it will do nothing.
        static void handle(const source_location& loc, const char* expression,
                           const char* message = nullptr) noexcept
        {
#ifndef DEBUG_ASSERT_NO_STDIO
            if (*expression == '\0')
            {
                if (message)
                    std::fprintf(stderr, "[debug assert] %s:%u: Unreachable code reached - %s.\n",
                                 loc.file_name, loc.line_number, message);
                else
                    std::fprintf(stderr, "[debug assert] %s:%u: Unreachable code reached.\n",
                                 loc.file_name, loc.line_number);
            }
            else if (message)
                std::fprintf(stderr, "[debug assert] %s:%u: Assertion '%s' failed - %s.\n",
                             loc.file_name, loc.line_number, expression, message);
            else
                std::fprintf(stderr, "[debug assert] %s:%u: Assertion '%s' failed.\n",
                             loc.file_name, loc.line_number, expression);
#else
            (void)loc;
            (void)expression;
            (void)message;
#endif
        }
    };

    /// \exclude
    namespace detail
    {
        //=== boilerplate ===//
        // from http://en.cppreference.com/w/cpp/types/remove_reference
        template <typename T>
        struct remove_reference
        {
            using type = T;
        };

        template <typename T>
        struct remove_reference<T&>
        {
            using type = T;
        };

        template <typename T>
        struct remove_reference<T&&>
        {
            using type = T;
        };

        // from http://stackoverflow.com/a/27501759
        template <class T>
        T&& forward(typename remove_reference<T>::type& t)
        {
            return static_cast<T&&>(t);
        }

        template <class T>
        T&& forward(typename remove_reference<T>::type&& t)
        {
            return static_cast<T&&>(t);
        }

        template <bool Value, typename T = void>
        struct enable_if;

        template <typename T>
        struct enable_if<true, T>
        {
            using type = T;
        };

        template <typename T>
        struct enable_if<false, T>
        {
        };

        //=== helper class to check if throw is allowed ===//
        template <class Handler, typename = void>
        struct allows_exception
        {
            static const bool value = false;
        };

        template <class Handler>
        struct allows_exception<Handler,
                                typename enable_if<Handler::throwing_exception_is_allowed>::type>
        {
            static const bool value = Handler::throwing_exception_is_allowed;
        };

        //=== regular void fake ===//
        struct regular_void
        {
            constexpr regular_void() = default;

            // enable conversion to anything
            // conversion must not actually be used
            template <typename T>
            constexpr operator T&() const noexcept
            {
                // doesn't matter how to get the T
                return DEBUG_ASSERT_MARK_UNREACHABLE, *static_cast<T*>(nullptr);
            }
        };

        //=== assert implementation ===//
        // function name will be shown on constexpr assertion failure
        template <class Handler, typename... Args>
        regular_void debug_assertion_failed(const source_location& loc, const char* expression,
                                            Args&&... args)
        {
            return Handler::handle(loc, expression, detail::forward<Args>(args)...), std::abort(),
                   regular_void();
        }

        // use enable if instead of tag dispatching
        // this removes on additional function and encourage optimization
        template <class Expr, class Handler, unsigned Level, typename... Args>
        constexpr auto do_assert(
            const Expr& expr, const source_location& loc, const char* expression, Handler,
            level<Level>,
            Args&&... args) noexcept(!allows_exception<Handler>::value
                                     || noexcept(Handler::handle(loc, expression,
                                                                 detail::forward<Args>(args)...)))
            -> typename enable_if<Level <= Handler::level, regular_void>::type
        {
            static_assert(Level > 0, "level of an assertion must not be 0");
            return expr() ? regular_void() :
                            debug_assertion_failed<Handler>(loc, expression,
                                                            detail::forward<Args>(args)...);
        }

        template <class Expr, class Handler, unsigned Level, typename... Args>
        DEBUG_ASSERT_FORCE_INLINE constexpr auto do_assert(const Expr&, const source_location&,
                                                           const char*, Handler, level<Level>,
                                                           Args&&...) noexcept ->
            typename enable_if<(Level > Handler::level), regular_void>::type
        {
            return regular_void();
        }

        template <class Expr, class Handler, typename... Args>
        constexpr auto do_assert(
            const Expr& expr, const source_location& loc, const char* expression, Handler,
            Args&&... args) noexcept(!allows_exception<Handler>::value
                                     || noexcept(Handler::handle(loc, expression,
                                                                 detail::forward<Args>(args)...)))
            -> typename enable_if<Handler::level != 0, regular_void>::type
        {
            return expr() ? regular_void() :
                            debug_assertion_failed<Handler>(loc, expression,
                                                            detail::forward<Args>(args)...);
        }

        template <class Expr, class Handler, typename... Args>
        DEBUG_ASSERT_FORCE_INLINE constexpr auto do_assert(const Expr&, const source_location&,
                                                           const char*, Handler, Args&&...) noexcept
            -> typename enable_if<Handler::level == 0, regular_void>::type
        {
            return regular_void();
        }

        DEBUG_ASSERT_PURE_FUNCTION constexpr bool always_false() noexcept
        {
            return false;
        }
    } // namespace detail
} // namespace debug_assert

//=== assertion macros ===//
#ifndef DEBUG_ASSERT_DISABLE
/// The assertion macro.
//
/// Usage: `DEBUG_ASSERT(<expr>, <handler>, [<level>],
/// [<handler-specific-args>].
/// Where:
/// * `<expr>` - the expression to check for, the expression `!<expr>` must be
/// well-formed and contextually convertible to `bool`.
/// * `<handler>` - an object of the module specific handler
/// * `<level>` (optional, defaults to `1`) - the level of the assertion, must
/// be an object of type [debug_assert::level<Level>]().
/// * `<handler-specific-args>` (optional) - any additional arguments that are
/// just forwarded to the handler function.
///
/// It will only check the assertion if `<level>` is less than or equal to
/// `Handler::level`.
/// A failed assertion will call: `Handler::handle(location, expression, args)`.
/// `location` is the [debug_assert::source_location]() at the macro expansion,
/// `expression` is the stringified expression and `args` are the
/// `<handler-specific-args>` as-is.
/// If the handler function returns, it will call [std::abort()].
///
/// The macro will expand to a `void` expression.
///
/// \notes Define `DEBUG_ASSERT_DISABLE` to completely disable this macro, it
/// will expand to nothing.
/// This should not be necessary, the regular version is optimized away
/// completely.
#define DEBUG_ASSERT(Expr, ...)                                                                    \
    static_cast<void>(                                                                             \
        debug_assert::detail::do_assert([&]()                                                      \
                                            DEBUG_ASSERT_PURE_FUNCTION noexcept { return Expr; },  \
                                        DEBUG_ASSERT_CUR_SOURCE_LOCATION, #Expr, __VA_ARGS__))

/// Marks a branch as unreachable.
///
/// Usage: `DEBUG_UNREACHABLE(<handler>, [<level>], [<handler-specific-args>])`
/// Where:
/// * `<handler>` - an object of the module specific handler
/// * `<level>` (optional, defaults to `1`) - the level of the assertion, must
/// be an object of type [debug_assert::level<Level>]().
/// * `<handler-specific-args>` (optional) - any additional arguments that are
/// just forwarded to the handler function.
///
/// It will only check the assertion if `<level>` is less than or equal to
/// `Handler::level`.
/// A failed assertion will call: `Handler::handle(location, "", args)`.
/// and `args` are the `<handler-specific-args>` as-is.
/// If the handler function returns, it will call [std::abort()].
///
/// This macro is also usable in a constant expression,
/// i.e. you can use it in a `constexpr` function to verify a condition like so:
/// `cond(val) ? do_sth(val) : DEBUG_UNREACHABLE(…)`.
/// You can't use `DEBUG_ASSERT` there.
///
/// The macro will expand to an expression convertible to any type,
/// although the resulting object is invalid,
/// which doesn't matter, as the statement is unreachable anyway.
///
/// \notes Define `DEBUG_ASSERT_DISABLE` to completely disable this macro, it
/// will expand to `DEBUG_ASSERT_MARK_UNREACHABLE`.
/// This should not be necessary, the regular version is optimized away
/// completely.
#define DEBUG_UNREACHABLE(...)                                                                     \
    debug_assert::detail::do_assert(debug_assert::detail::always_false,                            \
                                    DEBUG_ASSERT_CUR_SOURCE_LOCATION, "", __VA_ARGS__)
#else
#define DEBUG_ASSERT(Expr, ...) static_cast<void>(0)

#define DEBUG_UNREACHABLE(...) (DEBUG_ASSERT_MARK_UNREACHABLE, debug_assert::detail::regular_void())
#endif

#endif // DEBUG_ASSERT_HPP_INCLUDED
