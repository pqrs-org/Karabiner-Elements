<a id="top"></a>
# CI and other odd pieces

**Contents**<br>
[Continuous Integration systems](#continuous-integration-systems)<br>
[Other reporters](#other-reporters)<br>
[Low-level tools](#low-level-tools)<br>
[CMake](#cmake)<br>

This page talks about how Catch integrates with Continuous Integration 
Build Systems may refer to low-level tools, like CMake, or larger systems that run on servers, like Jenkins or TeamCity. This page will talk about both.

## Continuous Integration systems

Probably the most important aspect to using Catch with a build server is the use of different reporters. Catch comes bundled with three reporters that should cover the majority of build servers out there - although adding more for better integration with some is always a possibility (currently we also offer TeamCity, TAP, Automake and SonarQube reporters).

Two of these reporters are built in (XML and JUnit) and the third (TeamCity) is included as a separate header. It's possible that the other two may be split out in the future too - as that would make the core of Catch smaller for those that don't need them.

### XML Reporter
```-r xml``` 

The XML Reporter writes in an XML format that is specific to Catch. 

The advantage of this format is that it corresponds well to the way Catch works (especially the more unusual features, such as nested sections) and is a fully streaming format - that is it writes output as it goes, without having to store up all its results before it can start writing.

The disadvantage is that, being specific to Catch, no existing build servers understand the format natively. It can be used as input to an XSLT transformation that could convert it to, say, HTML - although this loses the streaming advantage, of course.

### JUnit Reporter
```-r junit```

The JUnit Reporter writes in an XML format that mimics the JUnit ANT schema.

The advantage of this format is that the JUnit Ant schema is widely understood by most build servers and so can usually be consumed with no additional work.

The disadvantage is that this schema was designed to correspond to how JUnit works - and there is a significant mismatch with how Catch works. Additionally the format is not streamable (because opening elements hold counts of failed and passing tests as attributes) - so the whole test run must complete before it can be written.

## Other reporters
Other reporters are not part of the single-header distribution and need
to be downloaded and included separately. All reporters are stored in
`single_include` directory in the git repository, and are named
`catch_reporter_*.hpp`. For example, to use the TeamCity reporter you
need to download `single_include/catch_reporter_teamcity.hpp` and include
it after Catch itself.

```cpp
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "catch_reporter_teamcity.hpp"
```

### TeamCity Reporter
```-r teamcity```

The TeamCity Reporter writes TeamCity service messages to stdout. In order to be able to use this reporter an additional header must also be included.

Being specific to TeamCity this is the best reporter to use with it - but it is completely unsuitable for any other purpose. It is a streaming format (it writes as it goes) - although test results don't appear in the TeamCity interface until the completion of a suite (usually the whole test run).

### Automake Reporter
```-r automake```

The Automake Reporter writes out the [meta tags](https://www.gnu.org/software/automake/manual/html_node/Log-files-generation-and-test-results-recording.html#Log-files-generation-and-test-results-recording) expected by automake via `make check`.

### TAP (Test Anything Protocol) Reporter
```-r tap```

Because of the incremental nature of Catch's test suites and ability to run specific tests, our implementation of TAP reporter writes out the number of tests in a suite last.

### SonarQube Reporter
```-r sonarqube```
[SonarQube Generic Test Data](https://docs.sonarqube.org/latest/analysis/generic-test/) XML format for tests metrics.

## Low-level tools

### Precompiled headers (PCHs)

Catch offers prototypal support for being included in precompiled headers, but because of its single-header nature it does need some actions by the user:
* The precompiled header needs to define `CATCH_CONFIG_ALL_PARTS`
* The implementation file needs to
  * undefine `TWOBLUECUBES_SINGLE_INCLUDE_CATCH_HPP_INCLUDED`
  * define `CATCH_CONFIG_IMPL_ONLY`
  * define `CATCH_CONFIG_MAIN` or `CATCH_CONFIG_RUNNER`
  * include "catch.hpp" again


### CodeCoverage module (GCOV, LCOV...)

If you are using GCOV tool to get testing coverage of your code, and are not sure how to integrate it with CMake and Catch, there should be an external example over at https://github.com/fkromer/catch_cmake_coverage


### pkg-config

Catch2 provides a rudimentary pkg-config integration, by registering itself
under the name `catch2`. This means that after Catch2 is installed, you
can use `pkg-config` to get its include path: `pkg-config --cflags catch2`.

### gdb and lldb scripts

Catch2's `contrib` folder also contains two simple debugger scripts,
`gdbinit` for `gdb` and `lldbinit` for `lldb`. If loaded into their
respective debugger, these will tell it to step over Catch2's internals
when stepping through code.


## CMake

[As it has been getting kinda long, the documentation of Catch2's
integration with CMake has been moved to its own page.](cmake-integration.md#top)


---

[Home](Readme.md#top)
