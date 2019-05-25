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

## Built in matchers
Catch currently provides some matchers, they are in the `Catch::Matchers` and `Catch` namespaces.

### String matchers
The string matchers are `StartsWith`, `EndsWith`, `Contains`, `Equals` and `Matches`. The first four match a literal (sub)string against a result, while `Matches` takes and matches an ECMAScript regex. Do note that `Matches` matches the string as a whole, meaning that "abc" will not match against "abcd", but "abc.*" will.

Each of the provided `std::string` matchers also takes an optional second argument, that decides case sensitivity (by-default, they are case sensitive).


### Vector matchers
The vector matchers are `Contains`, `VectorContains` and `Equals`. `VectorContains` looks for a single element in the matched vector, `Contains` looks for a set (vector) of elements inside the matched vector.

### Floating point matchers
The floating point matchers are `WithinULP` and `WithinAbs`. `WithinAbs` accepts floating point numbers that are within a certain margin of target. `WithinULP` performs an [ULP](https://en.wikipedia.org/wiki/Unit_in_the_last_place)-based comparison of two floating point numbers and accepts them if they are less than certain number of ULPs apart.

Do note that ULP-based checks only make sense when both compared numbers are of the same type and `WithinULP` will use type of its argument as the target type. This means that `WithinULP(1.f, 1)` will expect to compare `float`s, but `WithinULP(1., 1)` will expect to compare `double`s.


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
