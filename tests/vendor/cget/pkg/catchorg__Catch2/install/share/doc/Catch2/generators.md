<a id="top"></a>
# Data Generators

> Introduced in Catch 2.6.0.

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
  * `SingleValueGenerator<T>` -- contains only single element
  * `FixedValuesGenerator<T>` -- contains multiple elements
* 5 generic generators that modify other generators
  * `FilterGenerator<T, Predicate>` -- filters out elements from a generator
  for which the predicate returns "false"
  * `TakeGenerator<T>` -- takes first `n` elements from a generator
  * `RepeatGenerator<T>` -- repeats output from a generator `n` times
  * `MapGenerator<T, U, Func>` -- returns the result of applying `Func`
  on elements from a different generator
  * `ChunkGenerator<T>` -- returns chunks (inside `std::vector`) of n elements from a generator
* 4 specific purpose generators
  * `RandomIntegerGenerator<Integral>` -- generates random Integrals from range
  * `RandomFloatGenerator<Float>` -- generates random Floats from range
  * `RangeGenerator<T>` -- generates all values inside an arithmetic range
  * `IteratorGenerator<T>` -- copies and returns values from an iterator range

> `ChunkGenerator<T>`, `RandomIntegerGenerator<Integral>`, `RandomFloatGenerator<Float>` and `RangeGenerator<T>` were introduced in Catch 2.7.0.

> `IteratorGenerator<T>` was introduced in Catch 2.10.0.

The generators also have associated helper functions that infer their
type, making their usage much nicer. These are

* `value(T&&)` for `SingleValueGenerator<T>`
* `values(std::initializer_list<T>)` for `FixedValuesGenerator<T>`
* `table<Ts...>(std::initializer_list<std::tuple<Ts...>>)` for `FixedValuesGenerator<std::tuple<Ts...>>`
* `filter(predicate, GeneratorWrapper<T>&&)` for `FilterGenerator<T, Predicate>`
* `take(count, GeneratorWrapper<T>&&)` for `TakeGenerator<T>`
* `repeat(repeats, GeneratorWrapper<T>&&)` for `RepeatGenerator<T>`
* `map(func, GeneratorWrapper<T>&&)` for `MapGenerator<T, U, Func>` (map `U` to `T`, deduced from `Func`)
* `map<T>(func, GeneratorWrapper<U>&&)` for `MapGenerator<T, U, Func>` (map `U` to `T`)
* `chunk(chunk-size, GeneratorWrapper<T>&&)` for `ChunkGenerator<T>`
* `random(IntegerOrFloat a, IntegerOrFloat b)` for `RandomIntegerGenerator` or `RandomFloatGenerator`
* `range(Arithemtic start, Arithmetic end)` for `RangeGenerator<Arithmetic>` with a step size of `1`
* `range(Arithmetic start, Arithmetic end, Arithmetic step)` for `RangeGenerator<Arithmetic>` with a custom step size
* `from_range(InputIterator from, InputIterator to)` for `IteratorGenerator<T>`
* `from_range(Container const&)` for `IteratorGenerator<T>`

> `chunk()`, `random()` and both `range()` functions were introduced in Catch 2.7.0.

> `from_range` has been introduced in Catch 2.10.0

> `range()` for floating point numbers has been introduced in Catch 2.11.0

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


Apart from registering generators with Catch2, the `GENERATE` macro has
one more purpose, and that is to provide simple way of generating trivial
generators, as seen in the first example on this page, where we used it
as `auto i = GENERATE(1, 2, 3);`. This usage converted each of the three
literals into a single `SingleValueGenerator<int>` and then placed them all in
a special generator that concatenates other generators. It can also be
used with other generators as arguments, such as `auto i = GENERATE(0, 2,
take(100, random(300, 3000)));`. This is useful e.g. if you know that
specific inputs are problematic and want to test them separately/first.

**For safety reasons, you cannot use variables inside the `GENERATE` macro.
This is done because the generator expression _will_ outlive the outside
scope and thus capturing references is dangerous. If you need to use
variables inside the generator expression, make sure you thought through
the lifetime implications and use `GENERATE_COPY` or `GENERATE_REF`.**

> `GENERATE_COPY` and `GENERATE_REF` were introduced in Catch 2.7.1.

You can also override the inferred type by using `as<type>` as the first
argument to the macro. This can be useful when dealing with string literals,
if you want them to come out as `std::string`:

```cpp
TEST_CASE("type conversion", "[generators]") {
    auto str = GENERATE(as<std::string>{}, "a", "bb", "ccc");
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

