<a id="top"></a>
# Assertion Macros

**Contents**<br>
[Natural Expressions](#natural-expressions)<br>
[Exceptions](#exceptions)<br>
[Matcher expressions](#matcher-expressions)<br>
[Thread Safety](#thread-safety)<br>
[Expressions with commas](#expressions-with-commas)<br>

Most test frameworks have a large collection of assertion macros to capture all possible conditional forms (```_EQUALS```, ```_NOTEQUALS```, ```_GREATER_THAN``` etc).

Catch is different. Because it decomposes natural C-style conditional expressions most of these forms are reduced to one or two that you will use all the time. That said there is a rich set of auxiliary macros as well. We'll describe all of these here.

Most of these macros come in two forms:

## Natural Expressions

The ```REQUIRE``` family of macros tests an expression and aborts the test case if it fails.
The ```CHECK``` family are equivalent but execution continues in the same test case even if the assertion fails. This is useful if you have a series of essentially orthogonal assertions and it is useful to see all the results rather than stopping at the first failure.

* **REQUIRE(** _expression_ **)** and  
* **CHECK(** _expression_ **)**

Evaluates the expression and records the result. If an exception is thrown, it is caught, reported, and counted as a failure. These are the macros you will use most of the time.

Examples:
```
CHECK( str == "string value" );
CHECK( thisReturnsTrue() );
REQUIRE( i == 42 );
```

* **REQUIRE_FALSE(** _expression_ **)** and  
* **CHECK_FALSE(** _expression_ **)**

Evaluates the expression and records the _logical NOT_ of the result. If an exception is thrown it is caught, reported, and counted as a failure.
(these forms exist as a workaround for the fact that ! prefixed expressions cannot be decomposed).

Example:
```
REQUIRE_FALSE( thisReturnsFalse() );
```

Do note that "overly complex" expressions cannot be decomposed and thus will not compile. This is done partly for practical reasons (to keep the underlying expression template machinery to minimum) and partly for philosophical reasons (assertions should be simple and deterministic).

Examples:
* `CHECK(a == 1 && b == 2);`
This expression is too complex because of the `&&` operator. If you want to check that 2 or more properties hold, you can either put the expression into parenthesis, which stops decomposition from working, or you need to decompose the expression into two assertions: `CHECK( a == 1 ); CHECK( b == 2);`
* `CHECK( a == 2 || b == 1 );`
This expression is too complex because of the `||` operator. If you want to check that one of several properties hold, you can put the expression into parenthesis (unlike with `&&`, expression decomposition into several `CHECK`s is not possible).


### Floating point comparisons

When comparing floating point numbers - especially if at least one of them has been computed - great care must be taken to allow for rounding errors and inexact representations.

Catch provides a way to perform tolerant comparisons of floating point values through use of a wrapper class called `Approx`. `Approx` can be used on either side of a comparison expression. It overloads the comparisons operators to take a tolerance into account. Here's a simple example:

```cpp
REQUIRE( performComputation() == Approx( 2.1 ) );
```

Catch also provides a user-defined literal for `Approx`; `_a`. It resides in
the `Catch::literals` namespace and can be used like so:
```cpp
using namespace Catch::literals;
REQUIRE( performComputation() == 2.1_a );
```

`Approx` is constructed with defaults that should cover most simple cases.
For the more complex cases, `Approx` provides 3 customization points:

* __epsilon__ - epsilon serves to set the coefficient by which a result
can differ from `Approx`'s value before it is rejected.
_By default set to `std::numeric_limits<float>::epsilon()*100`._
* __margin__ - margin serves to set the the absolute value by which
a result can differ from `Approx`'s value before it is rejected.
_By default set to `0.0`._
* __scale__ - scale is used to change the magnitude of `Approx` for relative check.
_By default set to `0.0`._

#### epsilon example
```cpp
Approx target = Approx(100).epsilon(0.01);
100.0 == target; // Obviously true
200.0 == target; // Obviously still false
100.5 == target; // True, because we set target to allow up to 1% difference
```

#### margin example
```cpp
Approx target = Approx(100).margin(5);
100.0 == target; // Obviously true
200.0 == target; // Obviously still false
104.0 == target; // True, because we set target to allow absolute difference of at most 5
```

#### scale
Scale can be useful if the computation leading to the result worked
on different scale than is used by the results. Since allowed difference
between Approx's value and compared value is based primarily on Approx's value
(the allowed difference is computed as
`(Approx::scale + Approx::value) * epsilon`), the resulting comparison could
need rescaling to be correct.


## Exceptions

* **REQUIRE_NOTHROW(** _expression_ **)** and  
* **CHECK_NOTHROW(** _expression_ **)**

Expects that no exception is thrown during evaluation of the expression.

* **REQUIRE_THROWS(** _expression_ **)** and  
* **CHECK_THROWS(** _expression_ **)**

Expects that an exception (of any type) is be thrown during evaluation of the expression.

* **REQUIRE_THROWS_AS(** _expression_, _exception type_ **)** and  
* **CHECK_THROWS_AS(** _expression_, _exception type_ **)**

Expects that an exception of the _specified type_ is thrown during evaluation of the expression. Note that the _exception type_ is extended with `const&` and you should not include it yourself.

* **REQUIRE_THROWS_WITH(** _expression_, _string or string matcher_ **)** and  
* **CHECK_THROWS_WITH(** _expression_, _string or string matcher_ **)**

Expects that an exception is thrown that, when converted to a string, matches the _string_ or _string matcher_ provided (see next section for Matchers).

e.g.
```cpp
REQUIRE_THROWS_WITH( openThePodBayDoors(), Contains( "afraid" ) && Contains( "can't do that" ) );
REQUIRE_THROWS_WITH( dismantleHal(), "My mind is going" );
```

* **REQUIRE_THROWS_MATCHES(** _expression_, _exception type_, _matcher for given exception type_ **)** and
* **CHECK_THROWS_MATCHES(** _expression_, _exception type_, _matcher for given exception type_ **)**

Expects that exception of _exception type_ is thrown and it matches provided matcher (see next section for Matchers).


_Please note that the `THROW` family of assertions expects to be passed a single expression, not a statement or series of statements. If you want to check a more complicated sequence of operations, you can use a C++11 lambda function._

```cpp
REQUIRE_NOTHROW([&](){
    int i = 1;
    int j = 2;
    auto k = i + j;
    if (k == 3) {
        throw 1;
    }
}());
```



## Matcher expressions

To support Matchers a slightly different form is used. Matchers have [their own documentation](matchers.md#top).

* **REQUIRE_THAT(** _lhs_, _matcher expression_ **)** and  
* **CHECK_THAT(** _lhs_, _matcher expression_ **)**  

Matchers can be composed using `&&`, `||` and `!` operators.

## Thread Safety

Currently assertions in Catch are not thread safe.
For more details, along with workarounds, see the section on [the limitations page](limitations.md#thread-safe-assertions).

## Expressions with commas

Because the preprocessor parses code using different rules than the
compiler, multiple-argument assertions (e.g. `REQUIRE_THROWS_AS`) have
problems with commas inside the provided expressions. As an example
`REQUIRE_THROWS_AS(std::pair<int, int>(1, 2), std::invalid_argument);`
will fail to compile, because the preprocessor sees 3 arguments provided,
but the macro accepts only 2. There are two possible workarounds.

1) Use typedef:
```cpp
using int_pair = std::pair<int, int>;
REQUIRE_THROWS_AS(int_pair(1, 2), std::invalid_argument);
```

This solution is always applicable, but makes the meaning of the code
less clear.

2) Parenthesize the expression:
```cpp
TEST_CASE_METHOD((Fixture<int, int>), "foo", "[bar]") {
    SUCCEED();
}
```

This solution is not always applicable, because it might require extra
changes on the Catch's side to work.

---

[Home](Readme.md#top)
