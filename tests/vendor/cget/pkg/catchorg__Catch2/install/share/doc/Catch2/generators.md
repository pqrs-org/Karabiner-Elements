<a id="top"></a>
# Data Generators

_Generators are currently considered an experimental feature and their
API can change between versions freely._

Data generators (also known as _data driven/parametrized test cases_)
let you reuse the same set of assertions across different input values.
In Catch2, this means that they respect the ordering and nesting
of the `TEST_CASE` and `SECTION` macros.

How does combining generators and test cases work might be better
explained by an example:

```cpp
TEST_CASE("Generators") {
    auto i = GENERATE( range(1, 11) );

    SECTION( "Some section" ) {
        auto j = GENERATE( range( 11, 21 ) );
        REQUIRE(i < j);
    }
}
```

the assertion will be checked 100 times, because there are 10 possible
values for `i` (1, 2, ..., 10) and for each of them, there are 10 possible
values for `j` (11, 12, ..., 20).

You can also combine multiple generators by concatenation:
```cpp
static int square(int x) { return x * x; }
TEST_CASE("Generators 2") {
    auto i = GENERATE(0, 1, -1, range(-20, -10), range(10, 20));
    CAPTURE(i);
    REQUIRE(square(i) >= 0);
}
```

This will call `square` with arguments `0`, `1`, `-1`, `-20`, ..., `-11`,
`10`, ..., `19`.

----------

Because of the experimental nature of the current Generator implementation,
we won't list all of the first-party generators in Catch2. Instead you
should look at our current usage tests in
[projects/SelfTest/UsageTests/Generators.tests.cpp](/projects/SelfTest/UsageTests/Generators.tests.cpp).
For implementing your own generators, you can look at their implementation in
[include/internal/catch_generators.hpp](/include/internal/catch_generators.hpp).
