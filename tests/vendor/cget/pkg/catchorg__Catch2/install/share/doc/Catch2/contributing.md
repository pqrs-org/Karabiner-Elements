<a id="top"></a>
# Contributing to Catch

So you want to contribute something to Catch? That's great! Whether it's a bug fix, a new feature, support for
additional compilers - or just a fix to the documentation - all contributions are very welcome and very much appreciated.
Of course so are bug reports and other comments and questions.

If you are contributing to the code base there are a few simple guidelines to keep in mind. This also includes notes to
help you find your way around. As this is liable to drift out of date please raise an issue or, better still, a pull
request for this file, if you notice that.

## Branches

Ongoing development is currently on _master_. At some point an integration branch will be set-up and PRs should target
 that - but for now it's all against master. You may see feature branches come and go from time to time, too.

## Directory structure

_Users_ of Catch primarily use the single header version. _Maintainers_ should work with the full source (which is still,
primarily, in headers). This can be found in the `include` folder. There are a set of test files, currently under
`projects/SelfTest`. The test app can be built via CMake from the `CMakeLists.txt` file in the root, or you can generate
project files for Visual Studio, XCode, and others (instructions in the `projects` folder). If you have access to CLion,
it can work with the CMake file directly.

As well as the runtime test files you'll also see a `SurrogateCpps` directory under `projects/SelfTest`.
This contains a set of .cpp files that each `#include` a single header.
While these files are not essential to compilation they help to keep the implementation headers self-contained.
At time of writing this set is not complete but has reasonable coverage.
If you add additional headers please try to remember to add a surrogate cpp for it.

The other directories are `scripts` which contains a set of python scripts to help in testing Catch as well as
generating the single include, and `docs`, which contains the documentation as a set of markdown files.

__When submitting a pull request please do not include changes to the single include, or to the version number file
as these are managed by the scripts!__


## Testing your changes

Obviously all changes to Catch's code should be tested. If you added new
functionality, you should add tests covering and showcasing it. Even if you have
only made changes to Catch internals (i.e. you implemented some performance
improvements), you should still test your changes.

This means 2 things

* Compiling Catch's SelfTest project:
```
$ cd Catch2
$ cmake -Bdebug-build -H. -DCMAKE_BUILD_TYPE=Debug
$ cmake --build debug-build
```
because code that does not compile is evidently incorrect. Obviously,
you are not expected to have access to all the compilers and platforms
supported by Catch2, but you should at least smoke test your changes
on your platform. Our CI pipeline will check your PR against most of
the supported platforms, but it takes an hour to finish -- compiling
locally takes just a few minutes.


* Running the tests via CTest:
```
$ cd debug-build
$ ctest -j 2 --output-on-failure
```
If you added new tests, approval tests are very likely to fail. If they
do not, it means that your changes weren't run as part of them. This
_might_ be intentional, but usually is not.

The approval tests compare current output of the SelfTest binary in various
configurations against known good outputs. The reason it fails is,
_usually_, that you've added new tests but have not yet approved the changes
they introduce. This is done with the `scripts/approve.py` script, but
before you do so, you need to check that the introduced changes are indeed
intentional.



 *this document is still in-progress...*

---

[Home](Readme.md#top)
