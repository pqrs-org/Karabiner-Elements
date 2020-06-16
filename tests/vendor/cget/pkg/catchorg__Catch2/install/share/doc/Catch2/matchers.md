<a id="top"></a>
# Matchers

Matchers are an alternative way to do assertions which are easily extensible and composable.
This makes them well suited to use with more complex types (such as collections) or your own custom types.
Matchers were first popularised by the [Hamcrest](https://en.wikipedia.org/wiki/Hamcrest) family of frameworks.

## In use

Matchers are introduced with the `REQUIRE_THAT` or `CHECK_THAT` macros, which take two arguments.
The first argument is the thing (object or value) under test. The second part is a match _expression_,
which consists of either a single matcher or one or more matchers combined using `&&`, `||` or `!` operators.

For example, to assert that a string ends with a certain substring:

 ```c++
using Catch::Matchers::EndsWith; // or Catch::EndsWith
std::string str = getStringFromSomewhere();
REQUIRE_THAT( str, EndsWith( "as a service" ) );
```

The matcher objects can take multiple arguments, allowing more fine tuning.
The built-in string matchers, for example, take a second argument specifying whether the comparison is
case sensitive or not:

```c++
REQUIRE_THAT( str, EndsWith( "as a service", Catch::CaseSensitive::No ) );
 ```

And matchers can be combined:

```c++
REQUIRE_THAT( str,
    EndsWith( "as a service" ) ||
    (StartsWith( "Big data" ) && !Contains( "web scale" ) ) );
```

_The combining operators do not take ownership of the matcher objects.
This means that if you store the combined object, you have to ensure that
the matcher objects outlive its last use. What this means is that code
like this leads to a use-after-free and (hopefully) a crash:_

```cpp
TEST_CASE("Bugs, bugs, bugs", "[Bug]"){
    std::string str = "Bugs as a service";

    auto match_expression = Catch::EndsWith( "as a service" ) ||
        (Catch::StartsWith( "Big data" ) && !Catch::Contains( "web scale" ) );
    REQUIRE_THAT(str, match_expression);
}
```


## Built in matchers
Catch2 provides some matchers by default. They can be found in the
`Catch::Matchers::foo` namespace and are imported into the `Catch`
namespace as well.

There are two parts to each of the built-in matchers, the matcher
type itself and a helper function that provides template argument
deduction when creating templated matchers. As an example, the matcher
for checking that two instances of `std::vector` are identical is
`EqualsMatcher<T>`, but the user is expected to use the `Equals`
helper function instead.


### String matchers
The string matchers are `StartsWith`, `EndsWith`, `Contains`, `Equals` and `Matches`. The first four match a literal (sub)string against a result, while `Matches` takes and matches an ECMAScript regex. Do note that `Matches` matches the string as a whole, meaning that "abc" will not match against "abcd", but "abc.*" will.

Each of the provided `std::string` matchers also takes an optional second argument, that decides case sensitivity (by-default, they are case sensitive).


### Vector matchers
Catch2 currently provides 5 built-in matchers that work on `std::vector`.
These are

 * `Contains` which checks whether a specified vector is present in the result
 * `VectorContains` which checks whether a specified element is present in the result
 * `Equals` which checks whether the result is exactly equal (order matters) to a specific vector
 * `UnorderedEquals` which checks whether the result is equal to a specific vector under a permutation
 * `Approx` which checks whether the result is "approx-equal" (order matters, but comparison is done via `Approx`) to a specific vector
> Approx matcher was [introduced](https://github.com/catchorg/Catch2/issues/1499) in Catch 2.7.2.


### Floating point matchers
Catch2 provides 3 matchers for working with floating point numbers. These
are `WithinAbsMatcher`, `WithinUlpsMatcher` and `WithinRelMatcher`.

The `WithinAbsMatcher` matcher accepts floating point numbers that are
within a certain distance of target. It should be constructed with the
`WithinAbs(double target, double margin)` helper.

The `WithinUlpsMatcher` matcher accepts floating point numbers that are
within a certain number of [ULPs](https://en.wikipedia.org/wiki/Unit_in_the_last_place)
of the target. Because ULP comparisons need to be done differently for
`float`s and for `double`s, there are two overloads of the helpers for
this matcher, `WithinULP(float target, int64_t ULPs)`, and
`WithinULP(double target, int64_t ULPs)`.

The `WithinRelMatcher` matcher accepts floating point numbers that are
_approximately equal_ with the target number with some specific tolerance.
In other words, it checks that `|lhs - rhs| <= epsilon * max(|lhs|, |rhs|)`,
with special casing for `INFINITY` and `NaN`. There are _4_ overloads of
the helpers for this matcher, `WithinRel(double target, double margin)`,
`WithinRel(float target, float margin)`, `WithinRel(double target)`, and
`WithinRel(float target)`. The latter two provide a default epsilon of
machine epsilon * 100.

> `WithinRel` matcher was introduced in Catch 2.10.0

### Generic matchers
Catch also aims to provide a set of generic matchers. Currently this set
contains only a matcher that takes arbitrary callable predicate and applies
it onto the provided object.

Because of type inference limitations, the argument type of the predicate
has to be provided explicitly. Example:
```cpp
REQUIRE_THAT("Hello olleH",
             Predicate<std::string>(
                 [] (std::string const& str) -> bool { return str.front() == str.back(); },
                 "First and last character should be equal")
);
```

The second argument is an optional description of the predicate, and is
used only during reporting of the result.


### Exception matchers
Catch2 also provides an exception matcher that can be used to verify
that an exception's message exactly matches desired string. The matcher
is `ExceptionMessageMatcher`, and we also provide a helper function
`Message`.

The matched exception must publicly derive from `std::exception` and
the message matching is done _exactly_, including case.

> `ExceptionMessageMatcher` was introduced in Catch 2.10.0

Example use:
```cpp
REQUIRE_THROWS_MATCHES(throwsDerivedException(),  DerivedException,  Message("DerivedException::what"));
```

## Custom matchers
It's easy to provide your own matchers to extend Catch or just to work with your own types.

You need to provide two things:
1. A matcher class, derived from `Catch::MatcherBase<T>` - where `T` is the type being tested.
The constructor takes and stores any arguments needed (e.g. something to compare against) and you must
override two methods: `match()` and `describe()`.
2. A simple builder function. This is what is actually called from the test code and allows overloading.

Here's an example for asserting that an integer falls within a given range
(note that it is all inline for the sake of keeping the example short):

```c++
// The matcher class
class IntRange : public Catch::MatcherBase<int> {
    int m_begin, m_end;
public:
    IntRange( int begin, int end ) : m_begin( begin ), m_end( end ) {}

    // Performs the test for this matcher
    bool match( int const& i ) const override {
        return i >= m_begin && i <= m_end;
    }

    // Produces a string describing what this matcher does. It should
    // include any provided data (the begin/ end in this case) and
    // be written as if it were stating a fact (in the output it will be
    // preceded by the value under test).
    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << "is between " << m_begin << " and " << m_end;
        return ss.str();
    }
};

// The builder function
inline IntRange IsBetween( int begin, int end ) {
    return IntRange( begin, end );
}

// ...

// Usage
TEST_CASE("Integers are within a range")
{
    CHECK_THAT( 3, IsBetween( 1, 10 ) );
    CHECK_THAT( 100, IsBetween( 1, 10 ) );
}
```

Running this test gives the following in the console:

```
/**/TestFile.cpp:123: FAILED:
  CHECK_THAT( 100, IsBetween( 1, 10 ) )
with expansion:
  100 is between 1 and 10
```

---

[Home](Readme.md#top)
