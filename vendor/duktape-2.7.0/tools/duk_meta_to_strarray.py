#!/usr/bin/env python2
#
#  Create an array of C strings with Duktape built-in strings.
#  Useful when using external strings.
#

import os
import sys
import json

def to_c_string(x):
    res = '"'
    term = False
    for i, c in enumerate(x):
        if term:
            term = False
            res += '" "'

        o = ord(c)
        if o < 0x20 or o > 0x7e or c in '\'"\\':
            # Terminate C string so that escape doesn't become
            # ambiguous
            res += '\\x%02x' % o
            term = True
        else:
            res += c
    res += '"'
    return res

def main():
    f = open(sys.argv[1], 'rb')
    d = f.read()
    f.close()
    meta = json.loads(d)

    print('const char *duk_builtin_strings[] = {')

    strlist = meta['builtin_strings_base64']
    for i in xrange(len(strlist)):
        s = strlist[i]
        if i == len(strlist) - 1:
            print('    %s' % to_c_string(s.decode('base64')))
        else:
            print('    %s,' % to_c_string(s.decode('base64')))

    print('};')

if __name__ == '__main__':
    main()
