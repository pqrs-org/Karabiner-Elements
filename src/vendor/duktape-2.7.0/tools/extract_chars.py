#!/usr/bin/env python2
#
#  Select a set of Unicode characters (based on included/excluded categories
#  etc) and write out a compact bitstream for matching a character against
#  the set at runtime.  This is for the slow path, where we're especially
#  concerned with compactness.  A C source file with the table is written,
#  together with a matching C header.
#
#  Unicode categories (such as 'Z') can be used.  Two pseudo-categories
#  are also available for exclusion only: ASCII and NONBMP.  "ASCII"
#  category excludes ASCII codepoints which is useful because C code
#  typically contains an ASCII fast path so ASCII characters don't need
#  to be considered in the Unicode tables.  "NONBMP" excludes codepoints
#  above U+FFFF which is useful because such codepoints don't need to be
#  supported in standard ECMAScript.
#

import os
import sys
import math
import optparse

import dukutil

def read_unicode_data(unidata, catsinc, catsexc, filterfunc):
    "Read UnicodeData.txt, including lines matching catsinc unless excluded by catsexc or filterfunc."
    res = []
    f = open(unidata, 'rb')

    def filter_none(cp):
        return True
    if filterfunc is None:
        filterfunc = filter_none

    # The Unicode parsing is slow enough to warrant some speedups.
    exclude_cat_exact = {}
    for cat in catsexc:
        exclude_cat_exact[cat] = True
    include_cat_exact = {}
    for cat in catsinc:
        include_cat_exact[cat] = True

    for line in f:
        #line = line.strip()
        parts = line.split(';')

        codepoint = parts[0]
        if not filterfunc(long(codepoint, 16)):
            continue

        category = parts[2]
        if exclude_cat_exact.has_key(category):
            continue  # quick reject

        rejected = False
        for cat in catsexc:
            if category.startswith(cat) or codepoint == cat:
                rejected = True
                break
        if rejected:
            continue

        if include_cat_exact.has_key(category):
            res.append(line)
            continue

        accepted = False
        for cat in catsinc:
            if category.startswith(cat) or codepoint == cat:
                accepted = True
                break
        if accepted:
            res.append(line)

    f.close()

    # Sort based on Unicode codepoint
    def mycmp(a,b):
        t1 = a.split(';')
        t2 = b.split(';')
        n1 = long(t1[0], 16)
        n2 = long(t2[0], 16)
        return cmp(n1, n2)

    res.sort(cmp=mycmp)

    return res

def scan_ranges(lines):
    "Scan continuous ranges from (filtered) UnicodeData.txt lines."
    ranges = []
    range_start = None
    prev = None

    for line in lines:
        t = line.split(';')
        n = long(t[0], 16)
        if range_start is None:
            range_start = n
        else:
            if n == prev + 1:
                # continue range
                pass
            else:
                ranges.append((range_start, prev))
                range_start = n
        prev = n

    if range_start is not None:
        ranges.append((range_start, prev))

    return ranges

def generate_png(lines, fname):
    "Generate an illustrative PNG of the character set."
    from PIL import Image

    m = {}
    for line in lines:
        t = line.split(';')
        n = long(t[0], 16)
        m[n] = 1

    codepoints = 0x10ffff + 1
    width = int(256)
    height = int(math.ceil(float(codepoints) / float(width)))
    im = Image.new('RGB', (width, height))
    black = (0,0,0)
    white = (255,255,255)
    for cp in xrange(codepoints):
        y = cp / width
        x = cp % width

        if m.has_key(long(cp)):
            im.putpixel((x,y), black)
        else:
            im.putpixel((x,y), white)

    im.save(fname)

def generate_match_table1(ranges):
    "Unused match table format."

    # This is an earlier match table format which is no longer used.
    # IdentifierStart-UnicodeLetter has 445 ranges and generates a
    # match table of 2289 bytes.

    data = []
    prev_re = None

    def genrange(rs, re):
        if (rs > re):
            raise Exception('assumption failed: rs=%d re=%d' % (rs, re))

        while True:
            now = re - rs + 1
            if now > 255:
                now = 255
                data.append(now)    # range now
                data.append(0)        # skip 0
                rs = rs + now
            else:
                data.append(now)    # range now
                break

    def genskip(ss, se):
        if (ss > se):
            raise Exception('assumption failed: ss=%d se=%s' % (ss, se))

        while True:
            now = se - ss + 1
            if now > 255:
                now = 255
                data.append(now)    # skip now
                data.append(0)        # range 0
                ss = ss + now
            else:
                data.append(now)    # skip now
                break

    for rs, re in ranges:
        if prev_re is not None:
            genskip(prev_re + 1, rs - 1)
        genrange(rs, re)
        prev_re = re

    num_entries = len(data)

    # header: start of first range
    #         num entries
    hdr = []
    hdr.append(ranges[0][0] >> 8)    # XXX: check that not 0x10000 or over
    hdr.append(ranges[0][1] & 0xff)
    hdr.append(num_entries >> 8)
    hdr.append(num_entries & 0xff)

    return hdr + data

def generate_match_table2(ranges):
    "Unused match table format."

    # Another attempt at a match table which is also unused.
    # Total tables for all current classes is now 1472 bytes.

    data = []

    def enc(x):
        while True:
            if x < 0x80:
                data.append(x)
                break
            data.append(0x80 + (x & 0x7f))
            x = x >> 7

    prev_re = 0

    for rs, re in ranges:
        r1 = rs - prev_re    # 1 or above (no unjoined ranges)
        r2 = re - rs        # 0 or above
        enc(r1)
        enc(r2)
        prev_re = re

    enc(0)    # end marker

    return data

def generate_match_table3(ranges):
    "Current match table format."

    # Yet another attempt, similar to generate_match_table2 except
    # in packing format.
    #
    # Total match size now (at time of writing): 1194 bytes.
    #
    # This is the current encoding format used in duk_lexer.c.

    be = dukutil.BitEncoder()

    freq = [0] * (0x10ffff + 1)  # informative

    def enc(x):
        freq[x] += 1

        if x <= 0x0e:
            # 4-bit encoding
            be.bits(x, 4)
            return
        x -= 0x0e + 1
        if x <= 0xfd:
            # 12-bit encoding
            be.bits(0x0f, 4)
            be.bits(x, 8)
            return
        x -= 0xfd + 1
        if x <= 0xfff:
            # 24-bit encoding
            be.bits(0x0f, 4)
            be.bits(0xfe, 8)
            be.bits(x, 12)
            return
        x -= 0xfff + 1
        if True:
            # 36-bit encoding
            be.bits(0x0f, 4)
            be.bits(0xff, 8)
            be.bits(x, 24)
            return

        raise Exception('cannot encode')

    prev_re = 0

    for rs, re in ranges:
        r1 = rs - prev_re    # 1 or above (no unjoined ranges)
        r2 = re - rs        # 0 or above
        enc(r1)
        enc(r2)
        prev_re = re

    enc(0)    # end marker

    data, nbits = be.getBytes(), be.getNumBits()
    return data, freq

def main():
    parser = optparse.OptionParser()
    parser.add_option('--unicode-data', dest='unicode_data')      # UnicodeData.txt
    parser.add_option('--special-casing', dest='special_casing')  # SpecialCasing.txt
    parser.add_option('--include-categories', dest='include_categories')
    parser.add_option('--exclude-categories', dest='exclude_categories', default='NONE')
    parser.add_option('--out-source', dest='out_source')
    parser.add_option('--out-header', dest='out_header')
    parser.add_option('--out-png', dest='out_png')
    parser.add_option('--table-name', dest='table_name', default='match_table')
    (opts, args) = parser.parse_args()

    unidata = opts.unicode_data
    catsinc = []
    if opts.include_categories != '':
        catsinc = opts.include_categories.split(',')
    catsexc = []
    if opts.exclude_categories != 'NONE':
        catsexc = opts.exclude_categories.split(',')

    print 'CATSEXC: %s' % repr(catsexc)
    print 'CATSINC: %s' % repr(catsinc)

    # pseudocategories
    filter_ascii = ('ASCII' in catsexc)
    filter_nonbmp = ('NONBMP' in catsexc)

    # Read raw result
    def filter1(x):
        if filter_ascii and x <= 0x7f:
            # exclude ascii
            return False
        if filter_nonbmp and x >= 0x10000:
            # exclude non-bmp
            return False
        return True

    print('read unicode data')
    uni_filtered = read_unicode_data(unidata, catsinc, catsexc, filter1)
    print('done reading unicode data')

    # Raw output
    #print('RAW OUTPUT:')
    #print('===========')
    #print('\n'.join(uni_filtered))

    # Scan ranges
    #print('')
    #print('RANGES:')
    #print('=======')
    ranges = scan_ranges(uni_filtered)
    #for i in ranges:
    #    if i[0] == i[1]:
    #        print('0x%04x' % i[0])
    #    else:
    #        print('0x%04x ... 0x%04x' % (i[0], i[1]))
    #print('')
    print('%d ranges total' % len(ranges))

    # Generate match table
    #print('')
    #print('MATCH TABLE:')
    #print('============')
    #matchtable1 = generate_match_table1(ranges)
    #matchtable2 = generate_match_table2(ranges)
    matchtable3, freq = generate_match_table3(ranges)
    #print 'match table: %s' % repr(matchtable3)
    print 'match table length: %d bytes' % len(matchtable3)
    print 'encoding freq:'
    for i in xrange(len(freq)):
        if freq[i] == 0:
            continue
        print '  %6d: %d' % (i, freq[i])

    print('')
    print('MATCH C TABLE -> file %s' % repr(opts.out_header))

    # Create C source and header files
    genc = dukutil.GenerateC()
    genc.emitHeader('extract_chars.py')
    genc.emitArray(matchtable3, opts.table_name, size=len(matchtable3), typename='duk_uint8_t', intvalues=True, const=True)
    if opts.out_source is not None:
        f = open(opts.out_source, 'wb')
        f.write(genc.getString())
        f.close()

    genc = dukutil.GenerateC()
    genc.emitHeader('extract_chars.py')
    genc.emitLine('extern const duk_uint8_t %s[%d];' % (opts.table_name, len(matchtable3)))
    if opts.out_header is not None:
        f = open(opts.out_header, 'wb')
        f.write(genc.getString())
        f.close()

    # Image (for illustrative purposes only)
    if opts.out_png is not None:
        generate_png(uni_filtered, opts.out_png)

if __name__ == '__main__':
    main()
