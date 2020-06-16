<a id="top"></a>
# CMake integration

**Contents**<br>
[CMake target](#cmake-target)<br>
[Automatic test registration](#automatic-test-registration)<br>
[CMake project options](#cmake-project-options)<br>
[Installing Catch2 from git repository](#installing-catch2-from-git-repository)<br>
[Installing Catch2 from vcpkg](#installing-catch2-from-vcpkg)<br>

Because we use CMake to build Catch2, we also provide a couple of
integration points for our users.

1) Catch2 exports a (namespaced) CMake target
2) Catch2's repository contains CMake scripts for automatic registration
of `TEST_CASE`s in CTest

## CMake target

Catch2's CMake build exports an interface target `Catch2::Catch2`. Linking
against it will add the proper include path and all necessary capabilities
to the resulting binary.

This means that if Catch2 has been installed on the system, it should be
enough to do:
```cmake
find_package(Catch2 REQUIRED)
target_link_libraries(tests Catch2::Catch2)
```


This target is also provided when Catch2 is used as a subdirectory.
Assuming that Catch2 has been cloned to `lib/Catch2`:
```cmake
add_subdirectory(lib/Catch2)
target_link_libraries(tests Catch2::Catch2)
```

## Automatic test registration

Catch2's repository also contains two CMake scripts that help users
with automatically registering their `TEST_CASE`s with CTest. They
can be found in the `contrib` folder, and are

1) `Catch.cmake` (and its dependency `CatchAddTests.cmake`)
2) `ParseAndAddCatchTests.cmake`

If Catch2 has been installed in system, both of these can be used after
doing `find_package(Catch2 REQUIRED)`. Otherwise you need to add them
to your CMake module path.

### `Catch.cmake` and `CatchAddTests.cmake`

`Catch.cmake` provides function `catch_discover_tests` to get tests from
a target. This function works by running the resulting executable with
`--list-test-names-only` flag, and then parsing the output to find all
existing tests.

#### Usage
```cmake
cmake_minimum_required(VERSION 3.5)

project(baz LANGUAGES CXX VERSION 0.0.1)

find_package(Catch2 REQUIRED)
add_executable(foo test.cpp)
target_link_libraries(foo Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(foo)
```


#### Customization
`catch_discover_tests` can be given several extra argumets:
```cmake
catch_discover_tests(target
                     [TEST_SPEC arg1...]
                     [EXTRA_ARGS arg1...]
                     [WORKING_DIRECTORY dir]
                     [TEST_PREFIX prefix]
                     [TEST_SUFFIX suffix]
                     [PROPERTIES name1 value1...]
                     [TEST_LIST var]
)
```

* `TEST_SPEC arg1...`

Specifies test cases, wildcarded test cases, tags and tag expressions to
pass to the Catch executable alongside the `--list-test-names-only` flag.


* `EXTRA_ARGS arg1...`

Any extra arguments to pass on the command line to each test case.


* `WORKING_DIRECTORY dir`

Specifies the directory in which to run the discovered test cases.  If this
option is not provided, the current binary directory is used.


* `TEST_PREFIX prefix`

Specifies a _prefix_ to be added to the name of each discovered test case.
This can be useful when the same test executable is being used in multiple
calls to `catch_discover_tests()`, with different `TEST_SPEC` or `EXTRA_ARGS`.


* `TEST_SUFFIX suffix`

Same as `TEST_PREFIX`, except it specific the _suffix_ for the test names.
Both `TEST_PREFIX` and `TEST_SUFFIX` can be specified at the same time.


* `PROPERTIES name1 value1...`

Specifies additional properties to be set on all tests discovered by this
invocation of `catch_discover_tests`.


* `TEST_LIST var`

Make the list of tests available in the variable `var`, rather than the
default `<target>_TESTS`.  This can be useful when the same test
executable is being used in multiple calls to `catch_discover_tests()`.
Note that this variable is only available in CTest.


### `ParseAndAddCatchTests.cmake`

`ParseAndAddCatchTests` works by parsing all implementation files
associated with the provided target, and registering them via CTest's
`add_test`. This approach has some limitations, such as the fact that
commented-out tests will be registered anyway.


#### Usage

```cmake
cmake_minimum_required(VERSION 3.5)

project(baz LANGUAGES CXX VERSION 0.0.1)

find_package(Catch2 REQUIRED)
add_executable(foo test.cpp)
target_link_libraries(foo Catch2::Catch2)

include(CTest)
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(foo)
```


#### Customization

`ParseAndAddCatchTests` provides some customization points:
* `PARSE_CATCH_TESTS_VERBOSE` -- When `ON`, the script prints debug
messages. Defaults to `OFF`.
* `PARSE_CATCH_TESTS_NO_HIDDEN_TESTS` -- When `ON`, hidden tests (tests
tagged with any of `[!hide]`, `[.]` or `[.foo]`) will not be registered.
Defaults to `OFF`.
* `PARSE_CATCH_TESTS_ADD_FIXTURE_IN_TEST_NAME` -- When `ON`, adds fixture
class name to the test name in CTest. Defaults to `ON`.
* `PARSE_CATCH_TESTS_ADD_TARGET_IN_TEST_NAME` -- When `ON`, adds target
name to the test name in CTest. Defaults to `ON`.
* `PARSE_CATCH_TESTS_ADD_TO_CONFIGURE_DEPENDS` -- When `ON`, adds test
file to `CMAKE_CONFIGURE_DEPENDS`. This means that the CMake configuration
step will be re-ran when the test files change, letting new tests be
automatically discovered. Defaults to `OFF`.


Optionally, one can specify a launching command to run tests by setting the
variable `OptionalCatchTestLauncher` before calling `ParseAndAddCatchTests`. For
instance to run some tests using `MPI` and other sequentially, one can write
```cmake
set(OptionalCatchTestLauncher ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${NUMPROC})
ParseAndAddCatchTests(mpi_foo)
unset(OptionalCatchTestLauncher)
ParseAndAddCatchTests(bar)
```

## CMake project options

Catch2's CMake project also provides some options for other projects
that consume it. These are

* `CATCH_BUILD_TESTING` -- When `ON`, Catch2's SelfTest project will be
built. Defaults to `ON`. Note that Catch2 also obeys `BUILD_TESTING` CMake
variable, so _both_ of them need to be `ON` for the SelfTest to be built,
and either of them can be set to `OFF` to disable building SelfTest.
* `CATCH_BUILD_EXAMPLES` -- When `ON`, Catch2's usage examples will be
built. Defaults to `OFF`.
* `CATCH_INSTALL_DOCS` -- When `ON`, Catch2's documentation will be
included in the installation. Defaults to `ON`.
* `CATCH_INSTALL_HELPERS` -- When `ON`, Catch2's contrib folder will be
included in the installation. Defaults to `ON`.
* `BUILD_TESTING` -- When `ON` and the project is not used as a subproject,
Catch2's test binary will be built. Defaults to `ON`.


## Installing Catch2 from git repository

If you cannot install Catch2 from a package manager (e.g. Ubuntu 16.04
provides catch only in version 1.2.0) you might want to install it from
the repository instead. Assuming you have enough rights, you can just
install it to the default location, like so:
```
$ git clone https://github.com/catchorg/Catch2.git
$ cd Catch2
$ cmake -Bbuild -H. -DBUILD_TESTING=OFF
$ sudo cmake --build build/ --target install
```

If you do not have superuser rights, you will also need to specify
[CMAKE_INSTALL_PREFIX](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html)
when configuring the build, and then modify your calls to
[find_package](https://cmake.org/cmake/help/latest/command/find_package.html)
accordingly.

## Installing Catch2 from vcpkg

Alternatively, you can build and install Catch2 using [vcpkg](https://github.com/microsoft/vcpkg/) dependency manager:
```
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg install catch2
```

The catch2 port in vcpkg is kept up to date by microsoft team members and community contributors.
If the version is out of date, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.

---

[Home](Readme.md#top)
