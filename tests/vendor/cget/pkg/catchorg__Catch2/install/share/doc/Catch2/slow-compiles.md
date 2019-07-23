<a id="top"></a>
# Why do my tests take so long to compile?

**Contents**<br>
[Short answer](#short-answer)<br>
[Long answer](#long-answer)<br>
[Practical example](#practical-example)<br>
[Other possible solutions](#other-possible-solutions)<br>

Several people have reported that test code written with Catch takes much longer to compile than they would expect. Why is that?

Catch is implemented entirely in headers. There is a little overhead due to this - but not as much as you might think - and you can minimise it simply by organising your test code as follows:

## Short answer
Exactly one source file must ```#define``` either ```CATCH_CONFIG_MAIN``` or ```CATCH_CONFIG_RUNNER``` before ```#include```-ing Catch. In this file *do not write any test cases*! In most cases that means this file will just contain two lines (the ```#define``` and the ```#include```).

## Long answer

Usually C++ code is split between a header file, containing declarations and prototypes, and an implementation file (.cpp) containing the definition, or implementation, code. Each implementation file, along with all the headers that it includes (and which those headers include, etc), is expanded into a single entity called a translation unit - which is then passed to the compiler and compiled down to an object file.

But functions and methods can also be written inline in header files. The downside to this is that these definitions will then be compiled in *every* translation unit that includes the header.

Because Catch is implemented *entirely* in headers you might think that the whole of Catch must be compiled into every translation unit that uses it! Actually it's not quite as bad as that. Catch mitigates this situation by effectively maintaining the traditional separation between the implementation code and declarations. Internally the implementation code is protected by ```#ifdef```s and is conditionally compiled into only one translation unit. This translation unit is that one that ```#define```s ```CATCH_CONFIG_MAIN``` or ```CATCH_CONFIG_RUNNER```. Let's call this the main source file.

As a result the main source file *does* compile the whole of Catch every time! So it makes sense to dedicate this file to *only* ```#define```-ing the identifier and ```#include```-ing Catch (and implementing the runner code, if you're doing that). Keep all your test cases in other files. This way you won't pay the recompilation cost for the whole of Catch 

## Practical example
Assume you have the `Factorial` function from the [tutorial](tutorial.md#top) in `factorial.cpp` (with forward declaration in `factorial.h`) and want to test it and keep the compile times down when adding new tests. Then you should have 2 files, `tests-main.cpp` and `tests-factorial.cpp`:

```cpp
// tests-main.cpp
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
```

```cpp
// tests-factorial.cpp
#include "catch.hpp"

#include "factorial.h"

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}
```

After compiling `tests-main.cpp` once, it is enough to link it with separately compiled `tests-factorial.cpp`. This means that adding more tests to `tests-factorial.cpp`, will not result in recompiling Catch's main and the resulting compilation times will decrease substantially.

```
$ g++ tests-main.cpp -c
$ g++ factorial.cpp -c
$ g++ tests-main.o factorial.o tests-factorial.cpp -o tests && ./tests -r compact
Passed 1 test case with 4 assertions.
```

Now, the next time we change the file `tests-factorial.cpp` (say we add `REQUIRE( Factorial(0) == 1)`), it is enough to recompile the tests instead of recompiling main as well:

```
$ g++ tests-main.o factorial.o tests-factorial.cpp -o tests && ./tests -r compact
tests-factorial.cpp:11: failed: Factorial(0) == 1 for: 0 == 1
Failed 1 test case, failed 1 assertion.
```

## Other possible solutions
You can also opt to sacrifice some features in order to speed-up Catch's compilation times. For details see the [documentation on Catch's compile-time configuration](configuration.md#other-toggles).

---

[Home](Readme.md#top)
