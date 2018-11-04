<a id="top"></a>
# Logging macros

Additional messages can be logged during a test case. Note that the messages are scoped and thus will not be reported if failure occurs in scope preceding the message declaration. An example:

```cpp
TEST_CASE("Foo") {
    INFO("Test case start");
    for (int i = 0; i < 2; ++i) {
        INFO("The number is " << i);
        CHECK(i == 0);
    }
}

TEST_CASE("Bar") {
    INFO("Test case start");
    for (int i = 0; i < 2; ++i) {
        INFO("The number is " << i);
        CHECK(i == i);
    }
    CHECK(false);
}
```
When the `CHECK` fails in the "Foo" test case, then two messages will be printed.
```
Test case start
The number is 1
```
When the last `CHECK` fails in the "Bar" test case, then only one message will be printed: `Test case start`.


## Streaming macros

All these macros allow heterogenous sequences of values to be streaming using the insertion operator (```<<```) in the same way that std::ostream, std::cout, etc support it.

E.g.:
```c++
INFO( "The number is " << i );
```

(Note that there is no initial ```<<``` - instead the insertion sequence is placed in parentheses.)
These macros come in three forms:

**INFO(** _message expression_ **)**

The message is logged to a buffer, but only reported with the next assertion that is logged. This allows you to log contextual information in case of failures which is not shown during a successful test run (for the console reporter, without -s). Messages are removed from the buffer at the end of their scope, so may be used, for example, in loops.

**WARN(** _message expression_ **)**

The message is always reported but does not fail the test.

**FAIL(** _message expression_ **)**

The message is reported and the test case fails.

**FAIL_CHECK(** _message expression_ **)**

AS `FAIL`, but does not abort the test

## Quickly capture a variable value

**CAPTURE(** _expression_ **)**

Sometimes you just want to log the name and value of a variable. While you can easily do this with the INFO macro, above, as a convenience the CAPTURE macro handles the stringising of the variable name for you (actually it works with any expression, not just variables).

E.g.
```c++
CAPTURE( theAnswer );
```

This would log something like:

<pre>"theAnswer := 42"</pre>

---

[Home](Readme.md#top)
