<a id="top"></a>
# Contributing to Catch

**Contents**<br>
[Branches](#branches)<br>
[Directory structure](#directory-structure)<br>
[Testing your changes](#testing-your-changes)<br>
[Documenting your code](#documenting-your-code)<br>
[Code constructs to watch out for](#code-constructs-to-watch-out-for)<br>

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
__Note:__ When running your tests with multi-configuration generators like
Visual Studio, you might get errors "Test not available without configuration."
You then have to pick one configuration (e.g. ` -C Debug`) in the `ctest` call.

If you added new tests, approval tests are very likely to fail. If they
do not, it means that your changes weren't run as part of them. This
_might_ be intentional, but usually is not.

The approval tests compare current output of the SelfTest binary in various
configurations against known good outputs. The reason it fails is,
_usually_, that you've added new tests but have not yet approved the changes
they introduce. This is done with the `scripts/approve.py` script, but
before you do so, you need to check that the introduced changes are indeed
intentional.


## Documenting your code

If you have added new feature to Catch2, it needs documentation, so that
other people can use it as well. This section collects some technical
information that you will need for updating Catch2's documentation, and
possibly some generic advise as well.

First, the technicalities:

* We introduced version tags to the documentation, which show users in
which version a specific feature was introduced. This means that newly
written documentation should be tagged with a placeholder, that will
be replaced with the actual version upon release. There are 2 styles
of placeholders used through the documentation, you should pick one that
fits your text better (if in doubt, take a look at the existing version
tags for other features).
  * `> [Introduced](link-to-issue-or-PR) in Catch X.Y.Z` - this
  placeholder is usually used after a section heading
  * `> X (Y and Z) was [introduced](link-to-issue-or-PR) in Catch X.Y.Z`
  - this placeholder is used when you need to tag a subpart of something,
  e.g. list
* Crosslinks to different pages should target the `top` anchor, like this
`[link to contributing](contributing.md#top)`.
* If you have introduced a new document, there is a simple template you
should use. It provides you with the top anchor mentioned above, and also
with a backlink to the top of the documentation:
```markdown
<a id="top"></a>
# Cool feature

Text that explains how to use the cool feature.


---

[Home](Readme.md#top)
```
* For pages with more than 4 subheadings, we provide a table of contents
(ToC) at the top of the page. Because GitHub markdown does not support
automatic generation of ToC, it has to be handled semi-manually. Thus,
if you've added a new subheading to some page, you should add it to the
ToC. This can be done either manually, or by running the
`updateDocumentToC.py` script in the `scripts/` folder.


Now, for the generic tips:
  * Usage examples are good
  * Don't be afraid to introduce new pages
  * Try to be reasonably consistent with the surrounding documentation




## Code constructs to watch out for

This section is a (sadly incomplete) listing of various constructs that
are problematic and are not always caught by our CI infrastructure.

### Naked exceptions and exceptions-related function

If you are throwing an exception, it should be done via `CATCH_ERROR`
or `CATCH_RUNTIME_ERROR` in `catch_enforce.h`. These macros will handle
the differences between compilation with or without exceptions for you.
However, some platforms (IAR) also have problems with exceptions-related
functions, such as `std::current_exceptions`. We do not have IAR in our
CI, but luckily there should not be too many reasons to use these.
However, if you do, they should be kept behind a
`CATCH_CONFIG_DISABLE_EXCEPTIONS` macro.

### Unqualified usage of functions from C's stdlib

If you are using a function from C's stdlib, please include the header
as `<cfoo>` and call the function qualified. The common knowledge that
there is no difference is wrong, QNX and VxWorks won't compile if you
include the header as `<cfoo>` and call the function unqualified.


----

_This documentation will always be in-progress as new information comes
up, but we are trying to keep it as up to date as possible._

---

[Home](Readme.md#top)
