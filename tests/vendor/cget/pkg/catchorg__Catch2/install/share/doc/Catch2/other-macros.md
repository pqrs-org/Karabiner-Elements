<a id="top"></a>
# Other macros

This page serves as a reference for macros that are not documented
elsewhere. For now, these macros are separated into 2 rough categories,
"assertion related macros" and "test case related macros".

## Assertion related macros

* `CHECKED_IF` and `CHECKED_ELSE`

`CHECKED_IF( expr )` is an `if` replacement, that also applies Catch2's
stringification machinery to the _expr_ and records the result. As with
`if`, the block after a `CHECKED_IF` is entered only if the expression
evaluates to `true`. `CHECKED_ELSE( expr )` work similarly, but the block
is entered only if the _expr_ evaluated to `false`.

Example:
```cpp
int a = ...;
int b = ...;
CHECKED_IF( a == b ) {
    // This block is entered when a == b
} CHECKED_ELSE ( a == b ) {
    // This block is entered when a != b
}
```

* `CHECK_NOFAIL`

`CHECK_NOFAIL( expr )` is a variant of `CHECK` that does not fail the test
case if _expr_ evaluates to `false`. This can be useful for checking some
assumption, that might be violated without the test necessarily failing.

Example output:
```
main.cpp:6:
FAILED - but was ok:
  CHECK_NOFAIL( 1 == 2 )

main.cpp:7:
PASSED:
  CHECK( 2 == 2 )
```

* `SUCCEED`

`SUCCEED( msg )` is mostly equivalent with `INFO( msg ); REQUIRE( true );`.
In other words, `SUCCEED` is for cases where just reaching a certain line
means that the test has been a success.

Example usage:
```cpp
TEST_CASE( "SUCCEED showcase" ) {
    int I = 1;
    SUCCEED( "I is " << I );
}
```

* `STATIC_REQUIRE`

`STATIC_REQUIRE( expr )` is a macro that can be used the same way as a
`static_assert`, but also registers the success with Catch2, so it is
reported as a success at runtime. The whole check can also be deferred
to the runtime, by defining `CATCH_CONFIG_RUNTIME_STATIC_REQUIRE` before
including the Catch2 header.

Example:
```cpp
TEST_CASE("STATIC_REQUIRE showcase", "[traits]") {
    STATIC_REQUIRE( std::is_void<void>::value );
    STATIC_REQUIRE_FALSE( std::is_void<int>::value );
}
```

## Test case related macros

* `METHOD_AS_TEST_CASE`

`METHOD_AS_TEST_CASE( member-function-pointer, description )` lets you
register a member function of a class as a Catch2 test case. The class
will be separately instantiated for each method registered in this way.

```cpp
class TestClass {
    std::string s;

public:
    TestClass()
        :s( "hello" )
    {}

    void testCase() {
        REQUIRE( s == "hello" );
    }
};


METHOD_AS_TEST_CASE( TestClass::testCase, "Use class's method as a test case", "[class]" )
```

* `REGISTER_TEST_CASE`

`REGISTER_TEST_CASE( function, description )` let's you register
a `function` as a test case. The function has to have `void()` signature,
the description can contain both name and tags.

Example:
```cpp
REGISTER_TEST_CASE( someFunction, "ManuallyRegistered", "[tags]" );
```

_Note that the registration still has to happen before Catch2's session
is initiated. This means that it either needs to be done in a global
constructor, or before Catch2's session is created in user's own main._


* `ANON_TEST_CASE`

`ANON_TEST_CASE` is a `TEST_CASE` replacement that will autogenerate
unique name. The advantage of this is that you do not have to think
of a name for the test case,`the disadvantage is that the name doesn't
necessarily remain stable across different links, and thus it might be
hard to run directly.

Example:
```cpp
ANON_TEST_CASE() {
    SUCCEED("Hello from anonymous test case");
}
```

* `DYNAMIC_SECTION`

`DYNAMIC_SECTION` is a `SECTION` where the user can use `operator<<` to
create the final name for that section. This can be useful with e.g.
generators, or when creating a `SECTION` dynamically, within a loop.

Example:
```cpp
TEST_CASE( "looped SECTION tests" ) {
    int a = 1;

    for( int b = 0; b < 10; ++b ) {
        DYNAMIC_SECTION( "b is currently: " << b ) {
            CHECK( b > a );
        }
    }
}
```
