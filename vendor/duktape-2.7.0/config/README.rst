=================
Duktape genconfig
=================

``genconfig`` is a helper script for coming up with a ``duk_config.h`` for
compiling Duktape for your platform.

To support this:

* It helps to create a ``duk_config.h`` for your platform/compiler
  combination.  You can give a base configuration and then force certain
  values manually based on a YAML configuration file.

* It autogenerates documentation for config options based on option metadata
  files written in YAML.

NOTE: ``tools/configure.py`` is now the preferred tool for preparing a config
header (using genconfig.py) and Duktape source files (using other tools) for
build.
