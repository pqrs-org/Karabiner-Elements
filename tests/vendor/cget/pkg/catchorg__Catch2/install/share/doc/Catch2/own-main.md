<a id="top"></a>
# Supplying main() yourself

**Contents**<br>
[Let Catch take full control of args and config](#let-catch-take-full-control-of-args-and-config)<br>
[Amending the config](#amending-the-config)<br>
[Adding your own command line options](#adding-your-own-command-line-options)<br>
[Version detection](#version-detection)<br>

The easiest way to use Catch is to let it supply ```main()``` for you and handle configuring itself from the command line.

This is achieved by writing ```#define CATCH_CONFIG_MAIN``` before the ```#include "catch.hpp"``` in *exactly one* source file.

Sometimes, though, you need to write your own version of main(). You can do this by writing ```#define CATCH_CONFIG_RUNNER``` instead. Now you are free to write ```main()``` as normal and call into Catch yourself manually. You now have a lot of flexibility - but here are three recipes to get your started:

**Important note: you can only provide `main` in the same file you defined `CATCH_CONFIG_RUNNER`.**

## Let Catch take full control of args and config

If you just need to have code that executes before and/ or after Catch this is the simplest option.

```c++
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main( int argc, char* argv[] ) {
  // global setup...

  int result = Catch::Session().run( argc, argv );

  // global clean-up...

  return result;
}
```

## Amending the config

If you still want Catch to process the command line, but you want to programmatically tweak the config, you can do so in one of two ways:

```c++
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main( int argc, char* argv[] )
{
  Catch::Session session; // There must be exactly one instance
 
  // writing to session.configData() here sets defaults
  // this is the preferred way to set them
    
  int returnCode = session.applyCommandLine( argc, argv );
  if( returnCode != 0 ) // Indicates a command line error
        return returnCode;
 
  // writing to session.configData() or session.Config() here 
  // overrides command line args
  // only do this if you know you need to

  int numFailed = session.run();
  
  // numFailed is clamped to 255 as some unices only use the lower 8 bits.
  // This clamping has already been applied, so just return it here
  // You can also do any post run clean-up here
  return numFailed;
}
```

Take a look at the definitions of Config and ConfigData to see what you can do with them.

To take full control of the config simply omit the call to ```applyCommandLine()```.

## Adding your own command line options

Catch embeds a powerful command line parser called [Clara](https://github.com/philsquared/Clara). 
As of Catch2 (and Clara 1.0) Clara allows you to write _composable_ option and argument parsers, 
so extending Catch's own command line options is now easy.

```c++
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main( int argc, char* argv[] )
{
  Catch::Session session; // There must be exactly one instance
  
  int height = 0; // Some user variable you want to be able to set
  
  // Build a new parser on top of Catch's
  using namespace Catch::clara;
  auto cli 
    = session.cli() // Get Catch's composite command line parser
    | Opt( height, "height" ) // bind variable to a new option, with a hint string
        ["-g"]["--height"]    // the option names it will respond to
        ("how high?");        // description string for the help output
        
  // Now pass the new composite back to Catch so it uses that
  session.cli( cli ); 
  
  // Let Catch (using Clara) parse the command line
  int returnCode = session.applyCommandLine( argc, argv );
  if( returnCode != 0 ) // Indicates a command line error
      return returnCode;

  // if set on the command line then 'height' is now set at this point
  if( height > 0 )
      std::cout << "height: " << height << std::endl;

  return session.run();
}
```

See the [Clara documentation](https://github.com/philsquared/Clara/blob/master/README.md) for more details.


## Version detection

Catch provides a triplet of macros providing the header's version, 

* `CATCH_VERSION_MAJOR`
* `CATCH_VERSION_MINOR`
* `CATCH_VERSION_PATCH`

these macros expand into a single number, that corresponds to the appropriate
part of the version. As an example, given single header version v2.3.4,
the macros would expand into `2`, `3`, and `4` respectively.


---

[Home](Readme.md#top)
