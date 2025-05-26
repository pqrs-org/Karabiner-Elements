#!/usr/bin/env python2
#
#  UnicodeData.txt may contain ranges in addition to individual characters.
#  Unpack the ranges into individual characters for the other scripts to use.
#

import os
import sys
import optparse

def main():
    parser = optparse.OptionParser()
    parser.add_option('--unicode-data', dest='unicode_data')
    parser.add_option('--output', dest='output')
    parser.add_option('--quiet', dest='quiet', action='store_true', default=False, help='Suppress info messages (show warnings)')
    parser.add_option('--verbose', dest='verbose', action='store_true', default=False, help='Show verbose debug messages')
    (opts, args) = parser.parse_args()
    assert(opts.unicode_data is not None)
    assert(opts.output is not None)

    f_in = open(opts.unicode_data, 'rb')
    f_out = open(opts.output, 'wb')
    while True:
        line = f_in.readline()
        if line == '' or line == '\n':
            break
        parts = line.split(';')  # keep newline
        if parts[1].endswith('First>'):
            line2 = f_in.readline()
            parts2 = line2.split(';')
            if not parts2[1].endswith('Last>'):
                raise Exception('cannot parse range')
            cp1 = long(parts[0], 16)
            cp2 = long(parts2[0], 16)

            tmp = parts[1:]
            tmp[0] = '-""-'
            suffix = ';'.join(tmp)
            f_out.write(line)
            for i in xrange(cp1 + 1, cp2):
                f_out.write('%04X;%s' % (i, suffix))
            f_out.write(line2)
        else:
            f_out.write(line)

    f_in.close()
    f_out.flush()
    f_out.close()

if __name__ == '__main__':
    main()
