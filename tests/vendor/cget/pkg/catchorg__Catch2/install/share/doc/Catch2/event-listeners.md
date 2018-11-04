<a id="top"></a>
# Event Listeners

A `Listener` is a class you can register with Catch that will then be passed events,
such as a test case starting or ending, as they happen during a test run.
`Listeners` are actually types of `Reporters`, with a few small differences:
 
1. Once registered in code they are automatically used - you don't need to specify them on the command line
2. They are called in addition to (just before) any reporters, and you can register multiple listeners.
3. They derive from `Catch::TestEventListenerBase`, which has default stubs for all the events,
so you are not forced to implement events you're not interested in.
4. You register a listener with `CATCH_REGISTER_LISTENER`


## Implementing a Listener
Simply derive a class from `Catch::TestEventListenerBase` and implement the methods you are interested in, either in
the main source file (i.e. the one that defines `CATCH_CONFIG_MAIN` or `CATCH_CONFIG_RUNNER`), or in a
file that defines `CATCH_CONFIG_EXTERNAL_INTERFACES`.

Then register it using `CATCH_REGISTER_LISTENER`.

For example ([complete source code](../examples/210-Evt-EventListeners.cpp)):

```c++
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

struct MyListener : Catch::TestEventListenerBase {

    using TestEventListenerBase::TestEventListenerBase; // inherit constructor

    virtual void testCaseStarting( Catch::TestCaseInfo const& testInfo ) override {
        // Perform some setup before a test case is run
    }
    
    virtual void testCaseEnded( Catch::TestCaseStats const& testCaseStats ) override {
        // Tear-down after a test case is run
    }    
};
CATCH_REGISTER_LISTENER( MyListener )
```

_Note that you should not use any assertion macros within a Listener!_ 

## Events that can be hooked

The following are the methods that can be overridden in the Listener:

```c++
// The whole test run, starting and ending
virtual void testRunStarting( TestRunInfo const& testRunInfo );
virtual void testRunEnded( TestRunStats const& testRunStats );

// Test cases starting and ending
virtual void testCaseStarting( TestCaseInfo const& testInfo );
virtual void testCaseEnded( TestCaseStats const& testCaseStats );

// Sections starting and ending
virtual void sectionStarting( SectionInfo const& sectionInfo );
virtual void sectionEnded( SectionStats const& sectionStats );

// Assertions before/ after
virtual void assertionStarting( AssertionInfo const& assertionInfo );
virtual bool assertionEnded( AssertionStats const& assertionStats );

// A test is being skipped (because it is "hidden")
virtual void skipTest( TestCaseInfo const& testInfo );
```

More information about the events (e.g. name of the test case) is contained in the structs passed as arguments -
just look in the source code to see what fields are available. 

---

[Home](Readme.md#top)
