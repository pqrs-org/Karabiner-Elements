#!/usr/bin/env python2
#
#  Extract unique DUK_USE_xxx flags from current code base:
#
#    $ python extract_unique_options.py ../src-input/*.c ../src-input/*.h ../src-input/*.h.in
#

import os, sys, re

# DUK_USE_xxx/DUK_OPT_xxx are used as placeholders and not matched
# (only uppercase allowed)
re_use = re.compile(r'DUK_USE_[A-Z0-9_]+')
re_opt = re.compile(r'DUK_OPT_[A-Z0-9_]+')  # removed in Duktape 2.x; match anyway just in case

def main():
    uses = {}
    opts = {}

    for fn in sys.argv[1:]:
        f = open(fn, 'rb')
        for line in f:
            for t in re.findall(re_use, line):
                if t[-1] != '_':  # skip e.g. 'DUK_USE_'
                    uses[t] = True
            for t in re.findall(re_opt, line):
                if t[-1] != '_':
                    opts[t] = True
        f.close()

    k = opts.keys()
    k.sort()
    for i in k:
        print(i)

    k = uses.keys()
    k.sort()
    for i in k:
        print(i)

if __name__ == '__main__':
    main()
