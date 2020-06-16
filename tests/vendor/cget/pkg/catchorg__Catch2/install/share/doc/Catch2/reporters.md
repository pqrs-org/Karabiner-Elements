<a id="top"></a>
# Reporters

Catch has a modular reporting system and comes bundled with a handful of useful reporters built in.
You can also write your own reporters.

## Using different reporters

The reporter to use can easily be controlled from the command line.
To specify a reporter use [`-r` or `--reporter`](command-line.md#choosing-a-reporter-to-use), followed by the name of the reporter, e.g.:

```
-r xml
```

If you don't specify a reporter then the console reporter is used by default.
There are four reporters built in to the single include:

* `console` writes as lines of text, formatted to a typical terminal width, with colours if a capable terminal is detected.
* `compact` similar to `console` but optimised for minimal output - each entry on one line
* `junit` writes xml that corresponds to Ant's [junitreport](http://help.catchsoftware.com/display/ET/JUnit+Format) target. Useful for build systems that understand Junit.
Because of the way the junit format is structured the run must complete before anything is written. 
* `xml` writes an xml format tailored to Catch. Unlike `junit` this is a streaming format so results are delivered progressively.

There are a few additional reporters, for specific build systems, in the Catch repository (in `include\reporters`) which you can `#include` in your project if you would like to make use of them.
Do this in one source file - the same one you have `CATCH_CONFIG_MAIN` or `CATCH_CONFIG_RUNNER`.

* `teamcity` writes the native, streaming, format that [TeamCity](https://www.jetbrains.com/teamcity/) understands. 
Use this when building as part of a TeamCity build to see results as they happen ([code example](../examples/207-Rpt-TeamCityReporter.cpp)).
* `tap` writes in the TAP ([Test Anything Protocol](https://en.wikipedia.org/wiki/Test_Anything_Protocol)) format.
* `automake` writes in a format that correspond to [automake  .trs](https://www.gnu.org/software/automake/manual/html_node/Log-files-generation-and-test-results-recording.html) files
* `sonarqube` writes the [SonarQube Generic Test Data](https://docs.sonarqube.org/latest/analysis/generic-test/) XML format.

You see what reporters are available from the command line by running with `--list-reporters`.

By default all these reports are written to stdout, but can be redirected to a file with [`-o` or `--out`](command-line.md#sending-output-to-a-file)

## Writing your own reporter

You can write your own custom reporter and register it with Catch.
At time of writing the interface is subject to some changes so is not, yet, documented here.
If you are determined you shouldn't have too much trouble working it out from the existing implementations -
but do keep in mind upcoming changes (these will be minor, simplifying, changes such as not needing to forward calls to the base class).

---

[Home](Readme.md#top)
