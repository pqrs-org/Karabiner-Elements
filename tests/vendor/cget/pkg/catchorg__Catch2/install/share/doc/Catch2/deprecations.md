<a id="top"></a>
# Deprecations and incoming changes

This page documents current deprecations and upcoming planned changes
inside Catch2. The difference between these is that a deprecated feature
will be removed, while a planned change to a feature means that the
feature will behave differently, but will still be present. Obviously,
either of these is a breaking change, and thus will not happen until
at least the next major release.


## Deprecations

### `--list-*` return values

The return codes of the `--list-*` family of command line arguments
will no longer be equal to the number of tests/tags/etc found, instead
it will be 0 for success and non-zero for failure.


### `--list-test-names-only`

`--list-test-names-only` command line argument will be removed.


### `ANON_TEST_CASE`

`ANON_TEST_CASE` is scheduled for removal, as it can be fully replaced
by a `TEST_CASE` with no arguments.


### Secondary description amongst tags

Currently, the tags part of `TEST_CASE` (and others) macro can also
contain text that is not part of tags. This text is then separated into
a "description" of the test case, but the description is then never used
apart from writing it out for `--list-tests -v high`.

Because it isn't actually used nor documented, and brings complications
to Catch2's internals, description support will be removed.


## Planned changes


### Reporter verbosities

The current implementation of verbosities, where the reporter is checked
up-front whether it supports the requested verbosity, is fundamentally
misguided and will be changed. The new implementation will no longer check
whether the specified reporter supports the requested verbosity, instead
it will be up to the reporters to deal with verbosities as they see fit
(with an expectation that unsupported verbosities will be, at most,
warnings, but not errors).


### Output format of `--list-*` command line parameters

The various list operations will be piped through reporters. This means
that e.g. XML reporter will write the output as machine-parseable XML,
while the Console reporter will keep the current, human-oriented output.


### `CHECKED_IF` and `CHECKED_ELSE`

To make the `CHECKED_IF` and `CHECKED_ELSE` macros more useful, they will
be marked as "OK to fail" (`Catch::ResultDisposition::SuppressFail` flag
will be added), which means that their failure will not fail the test,
making the `else` actually useful.


### Change semantics of `[.]` and tag exclusion

Currently, given these 2 tests
```cpp
TEST_CASE("A", "[.][foo]") {}
TEST_CASE("B", "[.][bar]") {}
```
specifying `[foo]` as the testspec will run test "A" and specifying
`~[foo]` will run test "B", even though it is hidden. Also, specifying
`~[baz]` will run both tests. This behaviour is often surprising and will
be changed so that hidden tests are included in a run only if they
positively match a testspec.


### Console Colour API

The API for Catch2's console colour will be changed to take an extra
argument, the stream to which the colour code should be applied.

---

[Home](Readme.md#top)
