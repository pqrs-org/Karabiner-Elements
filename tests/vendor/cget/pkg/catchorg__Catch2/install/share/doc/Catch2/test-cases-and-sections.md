<a id="top"></a>
# Test cases and sections

While Catch fully supports the traditional, xUnit, style of class-based fixtures containing test case methods this is not the preferred style.

Instead Catch provides a powerful mechanism for nesting test case sections within a test case. For a more detailed discussion see the [tutorial](tutorial.md#test-cases-and-sections).

Test cases and sections are very easy to use in practice:

* **TEST_CASE(** _test name_ \[, _tags_ \] **)**
* **SECTION(** _section name_ **)**

_test name_ and _section name_ are free form, quoted, strings. The optional _tags_ argument is a quoted string containing one or more tags enclosed in square brackets. Tags are discussed below. Test names must be unique within the Catch executable.

For examples see the [Tutorial](tutorial.md#top)

## Tags

Tags allow an arbitrary number of additional strings to be associated with a test case. Test cases can be selected (for running, or just for listing) by tag - or even by an expression that combines several tags. At their most basic level they provide a simple way to group several related tests together.

As an example - given the following test cases:

    TEST_CASE( "A", "[widget]" ) { /* ... */ }
    TEST_CASE( "B", "[widget]" ) { /* ... */ }
    TEST_CASE( "C", "[gadget]" ) { /* ... */ }
    TEST_CASE( "D", "[widget][gadget]" ) { /* ... */ }

The tag expression, ```"[widget]"``` selects A, B & D. ```"[gadget]"``` selects C & D. ```"[widget][gadget]"``` selects just D and ```"[widget],[gadget]"``` selects all four test cases.

For more detail on command line selection see [the command line docs](command-line.md#specifying-which-tests-to-run)

Tag names are not case sensitive and can contain any ASCII characters. This means that tags `[tag with spaces]` and `[I said "good day"]` are both allowed tags and can be filtered on. Escapes are not supported however and `[\]]` is not a valid tag.

### Special Tags

All tag names beginning with non-alphanumeric characters are reserved by Catch. Catch defines a number of "special" tags, which have meaning to the test runner itself. These special tags all begin with a symbol character. Following is a list of currently defined special tags and their meanings.

* `[!hide]` or `[.]` - causes test cases to be skipped from the default list (i.e. when no test cases have been explicitly selected through tag expressions or name wildcards). The hide tag is often combined with another, user, tag (for example `[.][integration]` - so all integration tests are excluded from the default run but can be run by passing `[integration]` on the command line). As a short-cut you can combine these by simply prefixing your user tag with a `.` - e.g. `[.integration]`. Because the hide tag has evolved to have several forms, all forms are added as tags if you use one of them.

* `[!throws]` - lets Catch know that this test is likely to throw an exception even if successful. This causes the test to be excluded when running with `-e` or `--nothrow`.

* `[!mayfail]` - doesn't fail the test if any given assertion fails (but still reports it). This can be useful to flag a work-in-progress, or a known issue that you don't want to immediately fix but still want to track in your tests.

* `[!shouldfail]` - like `[!mayfail]` but *fails* the test if it *passes*. This can be useful if you want to be notified of accidental, or third-party, fixes.

* `[!nonportable]` - Indicates that behaviour may vary between platforms or compilers.

* `[#<filename>]` - running with `-#` or `--filenames-as-tags` causes Catch to add the filename, prefixed with `#` (and with any extension stripped), as a tag to all contained tests, e.g. tests in testfile.cpp would all be tagged `[#testfile]`.

* `[@<alias>]` - tag aliases all begin with `@` (see below).

* `[!benchmark]` - this test case is actually a benchmark. This is an experimental feature, and currently has no documentation. If you want to try it out, look at `projects/SelfTest/Benchmark.tests.cpp` for details.

## Tag aliases

Between tag expressions and wildcarded test names (as well as combinations of the two) quite complex patterns can be constructed to direct which test cases are run. If a complex pattern is used often it is convenient to be able to create an alias for the expression. This can be done, in code, using the following form:

    CATCH_REGISTER_TAG_ALIAS( <alias string>, <tag expression> )

Aliases must begin with the `@` character. An example of a tag alias is:

    CATCH_REGISTER_TAG_ALIAS( "[@nhf]", "[failing]~[.]" )

Now when `[@nhf]` is used on the command line this matches all tests that are tagged `[failing]`, but which are not also hidden.

## BDD-style test cases

In addition to Catch's take on the classic style of test cases, Catch supports an alternative syntax that allow tests to be written as "executable specifications" (one of the early goals of [Behaviour Driven Development](http://dannorth.net/introducing-bdd/)). This set of macros map on to ```TEST_CASE```s and ```SECTION```s, with a little internal support to make them smoother to work with.

* **SCENARIO(** _scenario name_ \[, _tags_ \] **)**

This macro maps onto ```TEST_CASE``` and works in the same way, except that the test case name will be prefixed by "Scenario: "

* **GIVEN(** _something_ **)**
* **WHEN(** _something_ **)**
* **THEN(** _something_ **)**

These macros map onto ```SECTION```s except that the section names are the _something_s prefixed by "given: ", "when: " or "then: " respectively.

* **AND_WHEN(** _something_ **)**
* **AND_THEN(** _something_ **)**

Similar to ```WHEN``` and ```THEN``` except that the prefixes start with "and ". These are used to chain ```WHEN```s and ```THEN```s together.

When any of these macros are used the console reporter recognises them and formats the test case header such that the Givens, Whens and Thens are aligned to aid readability.

Other than the additional prefixes and the formatting in the console reporter these macros behave exactly as ```TEST_CASE```s and ```SECTION```s. As such there is nothing enforcing the correct sequencing of these macros - that's up to the programmer!

---

[Home](Readme.md#top)
