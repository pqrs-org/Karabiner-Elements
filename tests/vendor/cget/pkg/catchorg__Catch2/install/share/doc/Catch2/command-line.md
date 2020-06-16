<a id="top"></a>
# Command line

**Contents**<br>
[Specifying which tests to run](#specifying-which-tests-to-run)<br>
[Choosing a reporter to use](#choosing-a-reporter-to-use)<br>
[Breaking into the debugger](#breaking-into-the-debugger)<br>
[Showing results for successful tests](#showing-results-for-successful-tests)<br>
[Aborting after a certain number of failures](#aborting-after-a-certain-number-of-failures)<br>
[Listing available tests, tags or reporters](#listing-available-tests-tags-or-reporters)<br>
[Sending output to a file](#sending-output-to-a-file)<br>
[Naming a test run](#naming-a-test-run)<br>
[Eliding assertions expected to throw](#eliding-assertions-expected-to-throw)<br>
[Make whitespace visible](#make-whitespace-visible)<br>
[Warnings](#warnings)<br>
[Reporting timings](#reporting-timings)<br>
[Load test names to run from a file](#load-test-names-to-run-from-a-file)<br>
[Just test names](#just-test-names)<br>
[Specify the order test cases are run](#specify-the-order-test-cases-are-run)<br>
[Specify a seed for the Random Number Generator](#specify-a-seed-for-the-random-number-generator)<br>
[Identify framework and version according to the libIdentify standard](#identify-framework-and-version-according-to-the-libidentify-standard)<br>
[Wait for key before continuing](#wait-for-key-before-continuing)<br>
[Specify the number of benchmark samples to collect](#specify-the-number-of-benchmark-samples-to-collect)<br>
[Specify the number of resamples for bootstrapping](#specify-the-number-of-resamples-for-bootstrapping)<br>
[Specify the confidence-interval for bootstrapping](#specify-the-confidence-interval-for-bootstrapping)<br>
[Disable statistical analysis of collected benchmark samples](#disable-statistical-analysis-of-collected-benchmark-samples)<br>
[Specify the amount of time in milliseconds spent on warming up each test](#specify-the-amount-of-time-in-milliseconds-spent-on-warming-up-each-test)<br>
[Usage](#usage)<br>
[Specify the section to run](#specify-the-section-to-run)<br>
[Filenames as tags](#filenames-as-tags)<br>
[Override output colouring](#override-output-colouring)<br>

Catch works quite nicely without any command line options at all - but for those times when you want greater control the following options are available.
Click one of the following links to take you straight to that option - or scroll on to browse the available options.

<a href="#specifying-which-tests-to-run">               `    <test-spec> ...`</a><br />
<a href="#usage">                                       `    -h, -?, --help`</a><br />
<a href="#listing-available-tests-tags-or-reporters">   `    -l, --list-tests`</a><br />
<a href="#listing-available-tests-tags-or-reporters">   `    -t, --list-tags`</a><br />
<a href="#showing-results-for-successful-tests">        `    -s, --success`</a><br />
<a href="#breaking-into-the-debugger">                  `    -b, --break`</a><br />
<a href="#eliding-assertions-expected-to-throw">        `    -e, --nothrow`</a><br />
<a href="#invisibles">                                  `    -i, --invisibles`</a><br />
<a href="#sending-output-to-a-file">                    `    -o, --out`</a><br />
<a href="#choosing-a-reporter-to-use">                  `    -r, --reporter`</a><br />
<a href="#naming-a-test-run">                           `    -n, --name`</a><br />
<a href="#aborting-after-a-certain-number-of-failures"> `    -a, --abort`</a><br />
<a href="#aborting-after-a-certain-number-of-failures"> `    -x, --abortx`</a><br />
<a href="#warnings">                                    `    -w, --warn`</a><br />
<a href="#reporting-timings">                           `    -d, --durations`</a><br />
<a href="#input-file">                                  `    -f, --input-file`</a><br />
<a href="#run-section">                                 `    -c, --section`</a><br />
<a href="#filenames-as-tags">                           `    -#, --filenames-as-tags`</a><br />


</br>

<a href="#list-test-names-only">                        `    --list-test-names-only`</a><br />
<a href="#listing-available-tests-tags-or-reporters">   `    --list-reporters`</a><br />
<a href="#order">                                       `    --order`</a><br />
<a href="#rng-seed">                                    `    --rng-seed`</a><br />
<a href="#libidentify">                                 `    --libidentify`</a><br />
<a href="#wait-for-keypress">                           `    --wait-for-keypress`</a><br />
<a href="#benchmark-samples">                           `    --benchmark-samples`</a><br />
<a href="#benchmark-resamples">                         `    --benchmark-resamples`</a><br />
<a href="#benchmark-confidence-interval">               `    --benchmark-confidence-interval`</a><br />
<a href="#benchmark-no-analysis">                       `    --benchmark-no-analysis`</a><br />
<a href="#benchmark-warmup-time">                       `    --benchmark-warmup-time`</a><br />
<a href="#use-colour">                                  `    --use-colour`</a><br />

</br>



<a id="specifying-which-tests-to-run"></a>
## Specifying which tests to run

<pre>&lt;test-spec> ...</pre>

Test cases, wildcarded test cases, tags and tag expressions are all passed directly as arguments. Tags are distinguished by being enclosed in square brackets.

If no test specs are supplied then all test cases, except "hidden" tests, are run.
A test is hidden by giving it any tag starting with (or just) a period (```.```) - or, in the deprecated case, tagged ```[hide]``` or given name starting with `'./'`. To specify hidden tests from the command line ```[.]``` or ```[hide]``` can be used *regardless of how they were declared*.

Specs must be enclosed in quotes if they contain spaces. If they do not contain spaces the quotes are optional.

Wildcards consist of the `*` character at the beginning and/or end of test case names and can substitute for any number of any characters (including none).

Test specs are case insensitive.

If a spec is prefixed with `exclude:` or the `~` character then the pattern matches an exclusion. This means that tests matching the pattern are excluded from the set - even if a prior inclusion spec included them. Subsequent inclusion specs will take precedence, however.
Inclusions and exclusions are evaluated in left-to-right order.

Test case examples:

<pre>thisTestOnly            Matches the test case called, 'thisTestOnly'
"this test only"        Matches the test case called, 'this test only'
these*                  Matches all cases starting with 'these'
exclude:notThis         Matches all tests except, 'notThis'
~notThis                Matches all tests except, 'notThis'
~*private*              Matches all tests except those that contain 'private'
a* ~ab* abc             Matches all tests that start with 'a', except those that
                        start with 'ab', except 'abc', which is included
-# [#somefile]          Matches all tests from the file 'somefile.cpp'
</pre>

Names within square brackets are interpreted as tags.
A series of tags form an AND expression whereas a comma-separated sequence forms an OR expression. e.g.:

<pre>[one][two],[three]</pre>
This matches all tests tagged `[one]` and `[two]`, as well as all tests tagged `[three]`

Test names containing special characters, such as `,` or `[` can specify them on the command line using `\`.
`\` also escapes itself.

<a id="choosing-a-reporter-to-use"></a>
## Choosing a reporter to use

<pre>-r, --reporter &lt;reporter></pre>

A reporter is an object that formats and structures the output of running tests, and potentially summarises the results. By default a console reporter is used that writes, IDE friendly, textual output. Catch comes bundled with some alternative reporters, but more can be added in client code.<br />
The bundled reporters are:

<pre>-r console
-r compact
-r xml
-r junit
</pre>

The JUnit reporter is an xml format that follows the structure of the JUnit XML Report ANT task, as consumed by a number of third-party tools, including Continuous Integration servers such as Hudson. If not otherwise needed, the standard XML reporter is preferred as this is a streaming reporter, whereas the Junit reporter needs to hold all its results until the end so it can write the overall results into attributes of the root node.

<a id="breaking-into-the-debugger"></a>
## Breaking into the debugger
<pre>-b, --break</pre>

Under most debuggers Catch2 is capable of automatically breaking on a test
failure. This allows the user to see the current state of the test during
failure.

<a id="showing-results-for-successful-tests"></a>
## Showing results for successful tests
<pre>-s, --success</pre>

Usually you only want to see reporting for failed tests. Sometimes it's useful to see *all* the output (especially when you don't trust that that test you just added worked first time!).
To see successful, as well as failing, test results just pass this option. Note that each reporter may treat this option differently. The Junit reporter, for example, logs all results regardless.

<a id="aborting-after-a-certain-number-of-failures"></a>
## Aborting after a certain number of failures
<pre>-a, --abort
-x, --abortx [&lt;failure threshold>]
</pre>

If a ```REQUIRE``` assertion fails the test case aborts, but subsequent test cases are still run.
If a ```CHECK``` assertion fails even the current test case is not aborted.

Sometimes this results in a flood of failure messages and you'd rather just see the first few. Specifying ```-a``` or ```--abort``` on its own will abort the whole test run on the first failed assertion of any kind. Use ```-x``` or ```--abortx``` followed by a number to abort after that number of assertion failures.

<a id="listing-available-tests-tags-or-reporters"></a>
## Listing available tests, tags or reporters
<pre>-l, --list-tests
-t, --list-tags
--list-reporters
</pre>

```-l``` or ```--list-tests``` will list all registered tests, along with any tags.
If one or more test-specs have been supplied too then only the matching tests will be listed.

```-t``` or ```--list-tags``` lists all available tags, along with the number of test cases they match. Again, supplying test specs limits the tags that match.

```--list-reporters``` lists the available reporters.

<a id="sending-output-to-a-file"></a>
## Sending output to a file
<pre>-o, --out &lt;filename>
</pre>

Use this option to send all output to a file. By default output is sent to stdout (note that uses of stdout and stderr *from within test cases* are redirected and included in the report - so even stderr will effectively end up on stdout).

<a id="naming-a-test-run"></a>
## Naming a test run
<pre>-n, --name &lt;name for test run></pre>

If a name is supplied it will be used by the reporter to provide an overall name for the test run. This can be useful if you are sending to a file, for example, and need to distinguish different test runs - either from different Catch executables or runs of the same executable with different options. If not supplied the name is defaulted to the name of the executable.

<a id="eliding-assertions-expected-to-throw"></a>
## Eliding assertions expected to throw
<pre>-e, --nothrow</pre>

Skips all assertions that test that an exception is thrown, e.g. ```REQUIRE_THROWS```.

These can be a nuisance in certain debugging environments that may break when exceptions are thrown (while this is usually optional for handled exceptions, it can be useful to have enabled if you are trying to track down something unexpected).

Sometimes exceptions are expected outside of one of the assertions that tests for them (perhaps thrown and caught within the code-under-test). The whole test case can be skipped when using ```-e``` by marking it with the ```[!throws]``` tag.

When running with this option any throw checking assertions are skipped so as not to contribute additional noise. Be careful if this affects the behaviour of subsequent tests.

<a id="invisibles"></a>
## Make whitespace visible
<pre>-i, --invisibles</pre>

If a string comparison fails due to differences in whitespace - especially leading or trailing whitespace - it can be hard to see what's going on.
This option transforms tabs and newline characters into ```\t``` and ```\n``` respectively when printing.

<a id="warnings"></a>
## Warnings
<pre>-w, --warn &lt;warning name></pre>

Enables reporting of suspicious test states. There are currently two
available warnings

```
    NoAssertions   // Fail test case / leaf section if no assertions
                   // (e.g. `REQUIRE`) is encountered.
    NoTests        // Return non-zero exit code when no test cases were run
                   // Also calls reporter's noMatchingTestCases method
```


<a id="reporting-timings"></a>
## Reporting timings
<pre>-d, --durations &lt;yes/no></pre>

When set to ```yes``` Catch will report the duration of each test case, in milliseconds. Note that it does this regardless of whether a test case passes or fails. Note, also, the certain reporters (e.g. Junit) always report test case durations regardless of this option being set or not.

<a id="input-file"></a>
## Load test names to run from a file
<pre>-f, --input-file &lt;filename></pre>

Provide the name of a file that contains a list of test case names - one per line. Blank lines are skipped and anything after the comment character, ```#```, is ignored.

A useful way to generate an initial instance of this file is to use the <a href="#list-test-names-only">list-test-names-only</a> option. This can then be manually curated to specify a specific subset of tests - or in a specific order.

<a id="list-test-names-only"></a>
## Just test names
<pre>--list-test-names-only</pre>

This option lists all available tests in a non-indented form, one on each line. This makes it ideal for saving to a file and feeding back into the <a href="#input-file">```-f``` or ```--input-file```</a> option.


<a id="order"></a>
## Specify the order test cases are run
<pre>--order &lt;decl|lex|rand&gt;</pre>

Test cases are ordered one of three ways:

### decl
Declaration order (this is the default order if no --order argument is provided).
Tests in the same TU are sorted using their declaration orders, different
TUs are in an implementation (linking) dependent order.


### lex
Lexicographic order. Tests are sorted by their name, their tags are ignored.


### rand

Randomly sorted. The order is dependent on Catch2's random seed (see
[`--rng-seed`](#rng-seed)), and is subset invariant. What this means
is that as long as the random seed is fixed, running only some tests
(e.g. via tag) does not change their relative order.

> The subset stability was introduced in Catch2 v2.12.0


<a id="rng-seed"></a>
## Specify a seed for the Random Number Generator
<pre>--rng-seed &lt;'time'|number&gt;</pre>

Sets a seed for the random number generator using ```std::srand()```. 
If a number is provided this is used directly as the seed so the random pattern is repeatable.
Alternatively if the keyword ```time``` is provided then the result of calling ```std::time(0)``` is used and so the pattern becomes unpredictable. In some cases, you might need to pass the keyword ```time``` in double quotes instead of single quotes.

In either case the actual value for the seed is printed as part of Catch's output so if an issue is discovered that is sensitive to test ordering the ordering can be reproduced - even if it was originally seeded from ```std::time(0)```.

<a id="libidentify"></a>
## Identify framework and version according to the libIdentify standard
<pre>--libidentify</pre>

See [The LibIdentify repo for more information and examples](https://github.com/janwilmans/LibIdentify).

<a id="wait-for-keypress"></a>
## Wait for key before continuing
<pre>--wait-for-keypress &lt;never|start|exit|both&gt;</pre>

Will cause the executable to print a message and wait until the return/ enter key is pressed before continuing -
either before running any tests, after running all tests - or both, depending on the argument.

<a id="benchmark-samples"></a>
## Specify the number of benchmark samples to collect
<pre>--benchmark-samples &lt;# of samples&gt;</pre>

> [Introduced](https://github.com/catchorg/Catch2/issues/1616) in Catch 2.9.0.

When running benchmarks a number of "samples" is collected. This is the base data for later statistical analysis.
Per sample a clock resolution dependent number of iterations of the user code is run, which is independent of the number of samples. Defaults to 100.

<a id="benchmark-resamples"></a>
## Specify the number of resamples for bootstrapping
<pre>--benchmark-resamples &lt;# of resamples&gt;</pre>

> [Introduced](https://github.com/catchorg/Catch2/issues/1616) in Catch 2.9.0.

After the measurements are performed, statistical [bootstrapping] is performed
on the samples. The number of resamples for that bootstrapping is configurable
but defaults to 100000. Due to the bootstrapping it is possible to give
estimates for the mean and standard deviation. The estimates come with a lower
bound and an upper bound, and the confidence interval (which is configurable but
defaults to 95%).

 [bootstrapping]: http://en.wikipedia.org/wiki/Bootstrapping_%28statistics%29

<a id="benchmark-confidence-interval"></a>
## Specify the confidence-interval for bootstrapping
<pre>--benchmark-confidence-interval &lt;confidence-interval&gt;</pre>

> [Introduced](https://github.com/catchorg/Catch2/issues/1616) in Catch 2.9.0.

The confidence-interval is used for statistical bootstrapping on the samples to
calculate the upper and lower bounds of mean and standard deviation.
Must be between 0 and 1 and defaults to 0.95.

<a id="benchmark-no-analysis"></a>
## Disable statistical analysis of collected benchmark samples
<pre>--benchmark-no-analysis</pre>

> [Introduced](https://github.com/catchorg/Catch2/issues/1616) in Catch 2.9.0.

When this flag is specified no bootstrapping or any other statistical analysis is performed.
Instead the user code is only measured and the plain mean from the samples is reported.

<a id="benchmark-warmup-time"></a>
## Specify the amount of time in milliseconds spent on warming up each test
<pre>--benchmark-warmup-time</pre>

> [Introduced](https://github.com/catchorg/Catch2/pull/1844) in Catch 2.11.2.

Configure the amount of time spent warming up each test.

<a id="usage"></a>
## Usage
<pre>-h, -?, --help</pre>

Prints the command line arguments to stdout


<a id="run-section"></a>
## Specify the section to run
<pre>-c, --section &lt;section name&gt;</pre>

To limit execution to a specific section within a test case, use this option one or more times.
To narrow to sub-sections use multiple instances, where each subsequent instance specifies a deeper nesting level.

E.g. if you have:

<pre>
TEST_CASE( "Test" ) {
  SECTION( "sa" ) {
    SECTION( "sb" ) {
      /*...*/
    }
    SECTION( "sc" ) {
      /*...*/
    }
  }
  SECTION( "sd" ) {
    /*...*/
  }
}
</pre>

Then you can run `sb` with:
<pre>./MyExe Test -c sa -c sb</pre>

Or run just `sd` with:
<pre>./MyExe Test -c sd</pre>

To run all of `sa`, including `sb` and `sc` use:
<pre>./MyExe Test -c sa</pre>

There are some limitations of this feature to be aware of:
- Code outside of sections being skipped will still be executed - e.g. any set-up code in the TEST_CASE before the
start of the first section.</br>
- At time of writing, wildcards are not supported in section names.
- If you specify a section without narrowing to a test case first then all test cases will be executed 
(but only matching sections within them).


<a id="filenames-as-tags"></a>
## Filenames as tags
<pre>-#, --filenames-as-tags</pre>

When this option is used then every test is given an additional tag which is formed of the unqualified 
filename it is found in, with any extension stripped, prefixed with the `#` character.

So, for example,  tests within the file `~\Dev\MyProject\Ferrets.cpp` would be tagged `[#Ferrets]`.

<a id="use-colour"></a>
## Override output colouring
<pre>--use-colour &lt;yes|no|auto&gt;</pre>

Catch colours output for terminals, but omits colouring when it detects that
output is being sent to a pipe. This is done to avoid interfering with automated
processing of output.

`--use-colour yes` forces coloured output, `--use-colour no` disables coloured
output. The default behaviour is `--use-colour auto`.

---

[Home](Readme.md#top)
