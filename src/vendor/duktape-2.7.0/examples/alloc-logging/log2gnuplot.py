#!/usr/bin/env python2
#
#  Analyze allocator logs and write total-bytes-in-use after every
#  operation to stdout.  The output can be gnuplotted as:
#
#  $ python log2gnuplot.py </tmp/duk-alloc-log.txt >/tmp/output.txt
#  $ gnuplot
#  > plot "output.txt" with lines
#

import os
import sys

def main():
    allocated = 0

    for line in sys.stdin:
        line = line.strip()
        parts = line.split(' ')

        # A ptr/NULL/FAIL size
        # F ptr/NULL size
        # R ptr/NULL oldsize ptr/NULL/FAIL newsize

        # Note: duk-low doesn't log oldsize (uses -1 instead)

        if parts[0] == 'A':
            if parts[1] != 'NULL' and parts[1] != 'FAIL':
                allocated += long(parts[2])
        elif parts[0] == 'F':
            allocated -= long(parts[2])
        elif parts[0] == 'R':
            allocated -= long(parts[2])
            if parts[3] != 'NULL' and parts[3] != 'FAIL':
                allocated += long(parts[4])
        print(allocated)

    print(allocated)

if __name__ == '__main__':
    main()
