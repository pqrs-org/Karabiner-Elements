===================
JSON/CBOR converter
===================

Overview
========

A simple command line utility to convert between JSON and CBOR.  Basic usage::

    $ make jsoncbor
    [...]
    $ cat test.json | ./jsoncbor -e   # writes CBOR to stdout
    $ cat test.cbor | ./jsoncbor -d   # writes JSON to stdout

CBOR objects are decoded into ECMAScript objects, with non-string keys
coerced into strings.
