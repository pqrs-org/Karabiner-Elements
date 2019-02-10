<a id="top"></a>
# Data Generators

Data generators (also known as _data driven/parametrized test cases_)
let you reuse the same set of assertions across different input values.
In Catch2, this means that they respect the ordering and nesting
of the `TEST_CASE` and `SECTION` macros, and their nested sections
are run once per each value in a generator.

This is best explained with an example:
```cpp
TEST_CASE("Generators") {
    auto i = GENERATE(1, 2, 3);
    SECTION("one") {
        auto j = GENERATE( -3, -2, -1 );
        REQUIRE(j < i);
    }
}
```

The assertion in this test case will be run 9 times, because there
are 3 possible values for `i` (1, 2, and 3) and there are 3 possible
values for `j` (-3, -2, and -1).


There are 2 parts to generators in Catch2, the `GENERATE` macro together
with the already provided generators, and the `IGenerator<T>` interface
that allows users to implement their own generators.

## Provided generators

Catch2's provided generator functionality consists of three parts,

* `GENERATE` macro,  that serves to integrate generator expression with
a test case,
* 2 fundamental generators
  * `ValueGenerator<T>` -- contains only single element
  * `ValuesGenerator<T>` -- contains multiple elements
* 4 generic generators that modify other generators
  * `FilterGenerator<T, Predicate>` -- filters out elements from a generator
  for which the predicate returns "false"
  * `TakeGenerator<T>` -- takes first `n` elements from a generator
  * `RepeatGenerator<T>` -- repeats output from a generator `n` times
  * `MapGenerator<T, U, Func>` -- returns the result of applying `Func`
  on elements from a different generator

The generators also have associated helper functions that infer their
type, making their usage much nicer. These are

* `value(T&&)` for `ValueGenerator<T>`
* `values(std::initializer_list<T>)` for `ValuesGenerator<T>`
* `filter(predicate, GeneratorWrapper<T>&&)` for `FilterGenerator<T, Predicate>`
* `take(count, GeneratorWrapper<T>&&)` for `TakeGenerator<T>`
* `repeat(repeats, GeneratorWrapper<T>&&)` for `RepeatGenerator<T>`
* `map(func, GeneratorWrapper<T>&&)` for `MapGenerator<T, T, Func>` (map `T` to `T`)
* `map<T>(func, GeneratorWrapper<U>&&)` for `MapGenerator<T, U, Func>` (map `U` to `T`)

And can be used as shown in the example below to create a generator
that returns 100 odd random number:

```cpp
TEST_CASE("Generating random ints", "[example][generator]") {
    SECTION("Deducing functions") {
        auto i = GENERATE(take(100, filter([](int i) { return i % 2 == 1; }, random(-100, 100))));
        REQUIRE(i > -100);
        REQUIRE(i < 100);
        REQUIRE(i % 2 == 1);
    }
}
```

_Note that `random` is currently not a part of the first-party generators_.


Apart from registering generators with Catch2, the `GENERATE` macro has
one more purpose, and that is to provide simple way of generating trivial
generators, as seen in the first example on this page, where we used it
as `auto i = GENERATE(1, 2, 3);`. This usage converted each of the three
literals into a single `ValueGenerator<int>` and then placed them all in
a special generator that concatenates other generators. It can also be
used with other generators as arguments, such as `auto i = GENERATE(0, 2,
take(100, random(300, 3000)));`. This is useful e.g. if you know that
specific inputs are problematic and want to test them separately/first.

**For safety reasons, you cannot use variables inside the `GENERATE` macro.**

You can also override the inferred type by using `as<type>` as the first
argument to the macro. This can be useful when dealing with string literals,
if you want them to come out as `std::string`:

```cpp
TEST_CASE("type conversion", "[generators]") {
    auto str = GENERATE(as<std::string>{}, "a", "bb", "ccc");`
    REQUIRE(str.size() > 0);
}
```

## Generator interface

You can also implement your own generators, by deriving from the
`IGenerator<T>` interface:

```cpp
template<typename T>
struct IGenerator : GeneratorUntypedBase {
    // via GeneratorUntypedBase:
    // Attempts to move the generator to the next element.
    // Returns true if successful (and thus has another element that can be read)
    virtual bool next() = 0;

    // Precondition:
    // The generator is either freshly constructed or the last call to next() returned true
    virtual T const& get() const = 0;
};
```

However, to be able to use your custom generator inside `GENERATE`, it
will need to be wrapped inside a `GeneratorWrapper<T>`.
`GeneratorWrapper<T>` is a value wrapper around a
`std::unique_ptr<IGenerator<T>>`.

For full example of implementing your own generator, look into Catch2's
examples, specifically
[Generators: Create your own generator](../examples/300-Gen-OwnGenerator.cpp).

