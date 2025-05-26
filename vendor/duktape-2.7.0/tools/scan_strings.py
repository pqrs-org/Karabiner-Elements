#!/usr/bin/env python2
#
#  Scan potential external strings from ECMAScript and C files.
#
#  Very simplistic example with a lot of limitations:
#
#    - Doesn't handle multiple variables in a variable declaration
#
#    - Only extracts strings from C files, these may correspond to
#      Duktape/C bindings (but in many cases don't)
#

import os
import sys
import re
import json

strmap = {}

# ECMAScript function declaration
re_funcname = re.compile(r'function\s+(\w+)', re.UNICODE)

# ECMAScript variable declaration
# XXX: doesn't handle multiple variables
re_vardecl = re.compile(r'var\s+(\w+)', re.UNICODE)

# ECMAScript variable assignment
re_varassign = re.compile(r'(\w+)\s*=\s*', re.UNICODE)

# ECMAScript dotted property reference (also matches numbers like
# '4.0', which are separately rejected below)
re_propref = re.compile(r'(\w+(?:\.\w+)+)', re.UNICODE)
re_digits = re.compile(r'^\d+$', re.UNICODE)

# ECMAScript or C string literal
re_strlit_dquot = re.compile(r'("(?:\\"|\\\\|[^"])*")', re.UNICODE)
re_strlit_squot = re.compile(r'(\'(?:\\\'|\\\\|[^\'])*\')', re.UNICODE)

def strDecode(x):
    # Need to decode hex, unicode, and other escapes.  Python syntax
    # is close enough to C and ECMAScript so use eval for now.

    try:
        return eval('u' + x)  # interpret as unicode string
    except:
        sys.stderr.write('Failed to parse: ' + repr(x) + ', ignoring\n')
        return None

def scan(f, fn):
    global strmap

    # Scan rules depend on file type
    if fn[-2:] == '.c':
        use_funcname = False
        use_vardecl = False
        use_varassign = False
        use_propref = False
        use_strlit_dquot = True
        use_strlit_squot = False
    else:
        use_funcname = True
        use_vardecl = True
        use_varassign = True
        use_propref = True
        use_strlit_dquot = True
        use_strlit_squot = True

    for line in f:
        # Assume input data is UTF-8
        line = line.decode('utf-8')

        if use_funcname:
            for m in re_funcname.finditer(line):
                strmap[m.group(1)] = True

        if use_vardecl:
            for m in re_vardecl.finditer(line):
                strmap[m.group(1)] = True

        if use_varassign:
            for m in re_varassign.finditer(line):
                strmap[m.group(1)] = True

        if use_propref:
            for m in re_propref.finditer(line):
                parts = m.group(1).split('.')
                if re_digits.match(parts[0]) is not None:
                    # Probably a number ('4.0' or such)
                    pass
                else:
                    for part in parts:
                        strmap[part] = True

        if use_strlit_dquot:
            for m in re_strlit_dquot.finditer(line):
                s = strDecode(m.group(1))
                if s is not None:
                    strmap[s] = True

        if use_strlit_squot:
            for m in re_strlit_squot.finditer(line):
                s = strDecode(m.group(1))
                if s is not None:
                    strmap[s] = True

def main():
    for fn in sys.argv[1:]:
        f = open(fn, 'rb')
        scan(f, fn)
        f.close()

    strs = []
    strs_base64 = []
    doc = {
        # Strings as Unicode strings
        'scanned_strings': strs,

        # Strings as base64-encoded UTF-8 data, which should be ready
        # to be used in C code (Duktape internal string representation
        # is UTF-8)
        'scanned_strings_base64': strs_base64
    }
    k = strmap.keys()
    k.sort()
    for s in k:
        strs.append(s)
        t = s.encode('utf-8').encode('base64')
        if len(t) > 0 and t[-1] == '\n':
            t = t[0:-1]
        strs_base64.append(t)

    print(json.dumps(doc, indent=4, ensure_ascii=True, sort_keys=True))

if __name__ == '__main__':
    main()
