#!/usr/bin/env python2
#
#  Resolve a line number in the combined source into an uncombined file/line
#  using a dist/src/duk_source_meta.json file.
#
#  Usage: $ python resolve_combined_lineno.py dist/src/duk_source_meta.json 12345
#

import os
import sys
import json

def main():
    with open(sys.argv[1], 'rb') as f:
        metadata = json.loads(f.read())
    lineno = int(sys.argv[2])

    for e in reversed(metadata['line_map']):
        if lineno >= e['combined_line']:
            orig_lineno = e['original_line'] + (lineno - e['combined_line'])
            print('%s:%d -> %s:%d' % ('duktape.c', lineno,
                                      e['original_file'], orig_lineno))
            break

if __name__ == '__main__':
    main()
