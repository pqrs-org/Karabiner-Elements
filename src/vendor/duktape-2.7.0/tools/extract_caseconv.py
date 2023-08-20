#!/usr/bin/env python2
#
#  Extract rules for Unicode case conversion, specifically the behavior
#  required by ECMAScript E5 in Sections 15.5.4.16 to 15.5.4.19.  The
#  bitstream encoded rules are used for the slow path at run time, so
#  compactness is favored over speed.
#
#  There is no support for context or locale sensitive rules, as they
#  are handled directly in C code before consulting tables generated
#  here.  ECMAScript requires case conversion both with and without
#  locale/language specific rules (e.g. String.prototype.toLowerCase()
#  and String.prototype.toLocaleLowerCase()), so they are best handled
#  in C anyway.
#
#  Case conversion rules for ASCII are also excluded as they are handled
#  by the C fast path.  Rules for non-BMP characters (codepoints above
#  U+FFFF) are omitted as they're not required for standard ECMAScript.
#

import os
import sys
import re
import math
import optparse

import dukutil

class UnicodeData:
    """Read UnicodeData.txt into an internal representation."""

    def __init__(self, filename):
        self.data = self.read_unicode_data(filename)
        print('read %d unicode data entries' % len(self.data))

    def read_unicode_data(self, filename):
        res = []
        f = open(filename, 'rb')
        for line in f:
            if line.startswith('#'):
                continue
            line = line.strip()
            if line == '':
                continue
            parts = line.split(';')
            if len(parts) != 15:
                raise Exception('invalid unicode data line')
            res.append(parts)
        f.close()

        # Sort based on Unicode codepoint.
        def mycmp(a,b):
            return cmp(long(a[0], 16), long(b[0], 16))

        res.sort(cmp=mycmp)
        return res

class SpecialCasing:
    """Read SpecialCasing.txt into an internal representation."""

    def __init__(self, filename):
        self.data = self.read_special_casing_data(filename)
        print('read %d special casing entries' % len(self.data))

    def read_special_casing_data(self, filename):
        res = []
        f = open(filename, 'rb')
        for line in f:
            try:
                idx = line.index('#')
                line = line[:idx]
            except ValueError:
                pass
            line = line.strip()
            if line == '':
                continue
            parts = line.split(';')
            parts = [i.strip() for i in parts]
            while len(parts) < 6:
                parts.append('')
            res.append(parts)
        f.close()
        return res

def parse_unicode_sequence(x):
    """Parse a Unicode sequence like ABCD 1234 into a unicode string."""
    res = ''
    for i in x.split(' '):
        i = i.strip()
        if i == '':
            continue
        res += unichr(long(i, 16))
    return res

def get_base_conversion_maps(unicode_data):
    """Create case conversion tables without handling special casing yet."""

    uc = {}        # uppercase, codepoint (number) -> string
    lc = {}        # lowercase
    tc = {}        # titlecase

    for x in unicode_data.data:
        c1 = long(x[0], 16)

        # just 16-bit support needed
        if c1 >= 0x10000:
            continue

        if x[12] != '':
            # field 12: simple uppercase mapping
            c2 = parse_unicode_sequence(x[12])
            uc[c1] = c2
            tc[c1] = c2    # titlecase default == uppercase, overridden below if necessary
        if x[13] != '':
            # field 13: simple lowercase mapping
            c2 = parse_unicode_sequence(x[13])
            lc[c1] = c2
        if x[14] != '':
            # field 14: simple titlecase mapping
            c2 = parse_unicode_sequence(x[14])
            tc[c1] = c2

    return uc, lc, tc

def update_special_casings(uc, lc, tc, special_casing):
    """Update case conversion tables with special case conversion rules."""

    for x in special_casing.data:
        c1 = long(x[0], 16)

        if x[4] != '':
            # conditions
            continue

        lower = parse_unicode_sequence(x[1])
        title = parse_unicode_sequence(x[2])
        upper = parse_unicode_sequence(x[3])

        if len(lower) > 1:
            lc[c1] = lower
        if len(upper) > 1:
            uc[c1] = upper
        if len(title) > 1:
            tc[c1] = title

        print('- special case: %d %d %d' % (len(lower), len(upper), len(title)))

def remove_ascii_part(convmap):
    """Remove ASCII case conversion parts (handled by C fast path)."""

    for i in xrange(128):
        if convmap.has_key(i):
            del convmap[i]

def scan_range_with_skip(convmap, start_idx, skip):
    """Scan for a range of continuous case conversion with a certain 'skip'."""

    conv_i = start_idx
    if not convmap.has_key(conv_i):
        return None, None, None
    elif len(convmap[conv_i]) > 1:
        return None, None, None
    else:
        conv_o = ord(convmap[conv_i])

    start_i = conv_i
    start_o = conv_o

    while True:
        new_i = conv_i + skip
        new_o = conv_o + skip

        if not convmap.has_key(new_i):
            break
        if len(convmap[new_i]) > 1:
            break
        if ord(convmap[new_i]) != new_o:
            break

        conv_i = new_i
        conv_o = new_o

    # [start_i,conv_i] maps to [start_o,conv_o], ignore ranges of 1 char.
    count = (conv_i - start_i) / skip + 1
    if count <= 1:
        return None, None, None

    # We have an acceptable range, remove them from the convmap here.
    for i in xrange(start_i, conv_i + skip, skip):
        del convmap[i]

    return start_i, start_o, count

def find_first_range_with_skip(convmap, skip):
    """Find first range with a certain 'skip' value."""

    for i in xrange(65536):
        start_i, start_o, count = scan_range_with_skip(convmap, i, skip)
        if start_i is None:
            continue
        return start_i, start_o, count

    return None, None, None

def generate_caseconv_tables(convmap):
    """Generate bit-packed case conversion table for a given conversion map."""

    # The bitstream encoding is based on manual inspection for whatever
    # regularity the Unicode case conversion rules have.
    #
    # Start with a full description of case conversions which does not
    # cover all codepoints; unmapped codepoints convert to themselves.
    # Scan for range-to-range mappings with a range of skips starting from 1.
    # Whenever a valid range is found, remove it from the map.  Finally,
    # output the remaining case conversions (1:1 and 1:n) on a per codepoint
    # basis.
    #
    # This is very slow because we always scan from scratch, but its the
    # most reliable and simple way to scan

    print('generate caseconv tables')

    ranges = []        # range mappings (2 or more consecutive mappings with a certain skip)
    singles = []       # 1:1 character mappings
    multis = []        # 1:n character mappings

    # Ranges with skips

    for skip in xrange(1,6+1):    # skips 1...6 are useful
        while True:
            start_i, start_o, count = find_first_range_with_skip(convmap, skip)
            if start_i is None:
                break
            print('- skip %d: %d %d %d' % (skip, start_i, start_o, count))
            ranges.append([start_i, start_o, count, skip])

    # 1:1 conversions

    k = convmap.keys()
    k.sort()
    for i in k:
        if len(convmap[i]) > 1:
            continue
        singles.append([i, ord(convmap[i])])    # codepoint, codepoint
        del convmap[i]

    # There are many mappings to 2-char sequences with latter char being U+0399.
    # These could be handled as a special case, but we don't do that right now.
    #
    # [8064L, u'\u1f08\u0399']
    # [8065L, u'\u1f09\u0399']
    # [8066L, u'\u1f0a\u0399']
    # [8067L, u'\u1f0b\u0399']
    # [8068L, u'\u1f0c\u0399']
    # [8069L, u'\u1f0d\u0399']
    # [8070L, u'\u1f0e\u0399']
    # [8071L, u'\u1f0f\u0399']
    # ...
    #
    # tmp = {}
    # k = convmap.keys()
    # k.sort()
    # for i in k:
    #    if len(convmap[i]) == 2 and convmap[i][1] == u'\u0399':
    #        tmp[i] = convmap[i][0]
    #        del convmap[i]
    # print(repr(tmp))
    #
    # skip = 1
    # while True:
    #    start_i, start_o, count = find_first_range_with_skip(tmp, skip)
    #    if start_i is None:
    #        break
    #    print('- special399, skip %d: %d %d %d' % (skip, start_i, start_o, count))
    # print(len(tmp.keys()))
    # print(repr(tmp))
    # XXX: need to put 12 remaining mappings back to convmap

    # 1:n conversions

    k = convmap.keys()
    k.sort()
    for i in k:
        multis.append([i, convmap[i]])        # codepoint, string
        del convmap[i]

    for t in singles:
        print '- singles: ' + repr(t)

    for t in multis:
        print '- multis: ' + repr(t)

    print '- range mappings: %d' % len(ranges)
    print '- single character mappings: %d' % len(singles)
    print '- complex mappings (1:n): %d' % len(multis)
    print '- remaining (should be zero): %d' % len(convmap.keys())

    # XXX: opportunities for diff encoding skip=3 ranges?
    prev = None
    for t in ranges:
        # range: [start_i, start_o, count, skip]
        if t[3] != 3:
            continue
        if prev is not None:
            print '- %d %d' % (t[0] - prev[0], t[1] - prev[1])
        else:
            print '- start: %d %d' % (t[0], t[1])
        prev = t

    # Bit packed encoding.

    be = dukutil.BitEncoder()

    for curr_skip in xrange(1, 7):    # 1...6
        count = 0
        for r in ranges:
            start_i, start_o, r_count, skip = r[0], r[1], r[2], r[3]
            if skip != curr_skip:
                continue
            count += 1
        be.bits(count, 6)
        print('- encode: skip=%d, count=%d' % (curr_skip, count))

        for r in ranges:
            start_i, start_o, r_count, skip = r[0], r[1], r[2], r[3]
            if skip != curr_skip:
                continue
            be.bits(start_i, 16)
            be.bits(start_o, 16)
            be.bits(r_count, 7)
    be.bits(0x3f, 6)    # maximum count value = end of skips

    count = len(singles)
    be.bits(count, 7)
    for t in singles:
        cp_i, cp_o = t[0], t[1]
        be.bits(cp_i, 16)
        be.bits(cp_o, 16)

    count = len(multis)
    be.bits(count, 7)
    for t in multis:
        cp_i, str_o = t[0], t[1]
        be.bits(cp_i, 16)
        be.bits(len(str_o), 2)
        for i in xrange(len(str_o)):
            be.bits(ord(str_o[i]), 16)

    return be.getBytes(), be.getNumBits()

def generate_regexp_canonicalize_tables(convmap):
    """Generate tables for case insensitive RegExp normalization."""

    # Generate a direct codepoint lookup for canonicalizing BMP range.

    def generate_canontab():
        res = []
        highest_nonid = -1

        for cp in xrange(65536):
            res_cp = cp  # default to as is
            if convmap.has_key(cp):
                tmp = convmap[cp]
                if len(tmp) == 1:
                    # If multiple codepoints from input, ignore.
                    res_cp = ord(tmp[0])
            if cp >= 0x80 and res_cp < 0x80:
                res_cp = cp  # If non-ASCII mapped to ASCII, ignore.
            if cp != res_cp:
                highest_nonid = cp
            res.append(res_cp)

        # At the moment this is 65370, which means there's very little
        # gain in assuming 1:1 mapping above a certain BMP codepoint
        # (though we do assume 1:1 mapping for above BMP codepoints).
        print('- highest non-identity mapping: %d' % highest_nonid)

        return res

    print('generate canontab')
    canontab = generate_canontab()

    # Figure out which BMP values are never the result of canonicalization.
    # Such codepoints are "don't care" in the sense that they are never
    # matched against at runtime: ranges are canonicalized at compile time,
    # and codepoint being matched is also canonicalized at run time.
    # (Currently unused.)

    def generate_dontcare():
        res = [ True ] * 65536
        for cp in canontab:
            res[cp] = False
        res_count = 0
        for x in res:
            if x:
                res_count += 1
        print('- %d dontcare codepoints' % res_count)
        return res

    print('generate canon dontcare')
    dontcare = generate_dontcare()

    # Generate maximal continuous ranges for canonicalization.  A continuous
    # range is a sequence with N codepoints where IN+i canonicalizes to OUT+i
    # for fixed IN, OUT, and i in 0...N-1.  There are unfortunately >1000
    # of these ranges, mostly because there are a lot of individual exceptions.
    # (Currently unused.)

    canon_ranges = []
    for cp in xrange(65536):
       canon_ranges.append([ cp, canontab[cp], 1 ])  # 1 codepoint ranges at first
    def merge_compatible_nogap(rng1, rng2):
        # Merge adjacent ranges if continuity allows.
        if rng1[0] + rng1[2] == rng2[0] and \
           rng1[1] + rng1[2] == rng2[1]:
            return [ rng1[0], rng1[1], rng1[2] + rng2[2] ]
        return None
    def merge_check_nogap():
        len_start = len(canon_ranges)
        for i in xrange(len(canon_ranges) - 1):
            j = i + 1
            rng1 = canon_ranges[i]
            rng2 = canon_ranges[j]
            if rng1 is None or rng2 is None: continue
            merged = merge_compatible_nogap(rng1, rng2)
            if merged is not None:
                canon_ranges[j] = None
                canon_ranges[i] = merged
        filtered = []
        for x in canon_ranges:
            if x is not None:
                filtered.append(x)
        len_end = len(filtered)
        if len_end < len_start:
            return filtered
        return None

    print('generate canon_ranges')
    while True:
        # Starting from individual ranges of 1 codepoint, merge adjacent
        # ranges until no more ranges can be merged.
        t = merge_check_nogap()
        if t is None:
            break
        canon_ranges = t
    print('- %d ranges' % len(canon_ranges))
    #for rng in canon_ranges:
    #    print('canon_ranges:')
    #    print(repr(rng))

    # Generate true/false ranges for BMP codepoints where:
    # - A codepoint is flagged true if continuity is broken at that point, so
    #   an explicit codepoint canonicalization is needed at runtime.
    # - A codepoint is flagged false if case conversion is continuous from the
    #   previous codepoint, i.e. out_curr = out_prev + 1.
    #
    # The result is a lot of small ranges due to a lot of small 'false' ranges.
    # Reduce the range set by checking if adjacent 'true' ranges have at most
    # false_limit 'false' entries between them.  If so, force the 'false'
    # entries to 'true' (safe but results in an unnecessary runtime codepoint
    # lookup) and merge the three ranges into a larger 'true' range.
    #
    # (Currently unused.)

    def generate_needcheck_straight():
        res = [ True ] * 65536
        assert(canontab[0] == 0)  # can start from in == out == 0
        prev_in = -1
        prev_out = -1
        for i in xrange(65536):
            # First create a straight true/false bitmap for BMP.
            curr_in = i
            curr_out = canontab[i]
            if prev_in + 1 == curr_in and prev_out + 1 == curr_out:
                res[i] = False
            prev_in = curr_in
            prev_out = curr_out
        return res
    def generate_needcheck_ranges(data):
        # Generate maximal accurate ranges.
        prev = None
        count = 0
        ranges = []
        for i in data:
            if prev is None or prev != i:
                if prev is not None:
                    ranges.append([ prev, count ])
                prev = i
                count = 1
            else:
                count += 1
        if prev is not None:
            ranges.append([ prev, count ])
        return ranges
    def fillin_needcheck_ranges(data, false_limit):
        # Fill in TRUE-FALSE*N-TRUE gaps into TRUE-TRUE*N-TRUE which is
        # safe (leads to an unnecessary runtime check) but reduces
        # range data size considerably.
        res = []
        for r in data:
            res.append([ r[0], r[1] ])
        while True:
            found = False
            for i in xrange(len(res) - 2):
                r1 = res[i]
                r2 = res[i + 1]
                r3 = res[i + 2]
                if r1[0] == True and r2[0] == False and r3[0] == True and \
                   r2[1] <= false_limit:
                    #print('fillin %d falses' % r2[1])
                    res.pop(i + 2)
                    res.pop(i + 1)
                    res[i] = [ True, r1[1] + r2[1] + r3[1] ]
                    found = True
                    break
            if not found:
                break
        return res

    print('generate needcheck straight')
    needcheck = generate_needcheck_straight()

    print('generate needcheck without false fillins')
    needcheck_ranges1 = generate_needcheck_ranges(needcheck)
    print('- %d ranges' % len(needcheck_ranges1))
    #print(needcheck_ranges1)

    print('generate needcheck with false fillins')
    needcheck_ranges2 = fillin_needcheck_ranges(needcheck_ranges1, 11)
    print('- %d ranges' % len(needcheck_ranges2))
    #print(needcheck_ranges2)

    # Generate a bitmap for BMP, divided into N-codepoint blocks, with each
    # bit indicating: "entire codepoint block canonicalizes continuously, and
    # the block is continuous with the previous and next block".  A 'true'
    # entry allows runtime code to just skip the block, advancing 'in' and
    # 'out' by the block size, with no codepoint conversion.  The block size
    # should be large enough to produce a relatively small lookup table, but
    # small enough to reduce codepoint conversions to a manageable number
    # because the conversions are (currently) quite slow.  This matters
    # especially for case-insensitive RegExps; without any optimization,
    # /[\u0000-\uffff]/i requires 65536 case conversions for runtime
    # normalization.

    block_shift = 5
    block_size = 1 << block_shift
    block_mask = block_size - 1
    num_blocks = 65536 / block_size

    def generate_block_bits(check_continuity):
        res = [ True ] * num_blocks
        for i in xrange(num_blocks):
            base_in = i * block_size
            base_out = canontab[base_in]
            if check_continuity:
                lower = -1   # [-1,block_size]
                upper = block_size + 1
            else:
                lower = 0    # [0,block_size-1]
                upper = block_size
            for j in xrange(lower, upper):
                cp = base_in + j
                if cp >= 0x0000 and cp <= 0xffff and canontab[cp] != base_out + j:
                   res[i] = False
                   break
        return res

    def dump_block_bitmap(bits):
        tmp = ''.join([ ({ True: 'x', False: '.' })[b] for b in bits])
        tmp = re.sub(r'.{64}', lambda x: x.group(0) + '\n', tmp)
        blocks_true = tmp.count('x')
        blocks_false = tmp.count('.')
        print('%d codepoint blocks are continuous, %d blocks are not' % (blocks_true, blocks_false))
        sys.stdout.write(tmp)
        #print(bits)

    def dump_test_lookup(bits):
        sys.stdout.write('duk_uint8_t test = {');
        for b in bits:
            if b:
                sys.stdout.write('1,')
            else:
                sys.stdout.write('0,')
        sys.stdout.write('};\n')

    def convert_to_bitmap(bits):
        # C code looks up bits as:
        #   index = codepoint >> N
        #   bitnum = codepoint & mask
        #   bitmask = 1 << bitnum
        # So block 0 is mask 0x01 of first byte, block 1 is mask 0x02 of
        # first byte, etc.
        res = []
        curr = 0
        mask = 0x01
        for b in bits:
            if b:
                curr += mask
            mask = mask * 2
            if mask == 0x100:
                res.append(curr)
                curr = 0
                mask = 0x01
        assert(mask == 0x01)  # no leftover
        return res

    print('generate canon block bitmap without continuity')
    block_bits1 = generate_block_bits(False)
    dump_block_bitmap(block_bits1)
    dump_test_lookup(block_bits1)

    print('generate canon block bitmap with continuity')
    block_bits2 = generate_block_bits(True)
    dump_block_bitmap(block_bits2)
    dump_test_lookup(block_bits2)

    print('generate final canon bitmap')
    block_bitmap = convert_to_bitmap(block_bits2)
    print('- %d bytes' % len(block_bitmap))
    print('- ' + repr(block_bitmap))
    canon_bitmap = {
        'data': block_bitmap,
        'block_size': block_size,
        'block_shift': block_shift,
        'block_mask': block_mask
    }

    # This is useful to figure out corner case test cases.
    print('canon blocks which are different with and without continuity check')
    for i in xrange(num_blocks):
        if block_bits1[i] != block_bits2[i]:
            print('- block %d ([%d,%d]) differs' % (i, i * block_size, i * block_size + block_size - 1))

    return canontab, canon_bitmap

def clonedict(x):
    "Shallow clone of input dict."
    res = {}
    for k in x.keys():
        res[k] = x[k]
    return res

def main():
    parser = optparse.OptionParser()
    parser.add_option('--command', dest='command', default='caseconv_bitpacked')
    parser.add_option('--unicode-data', dest='unicode_data')
    parser.add_option('--special-casing', dest='special_casing')
    parser.add_option('--out-source', dest='out_source')
    parser.add_option('--out-header', dest='out_header')
    parser.add_option('--table-name-lc', dest='table_name_lc', default='caseconv_lc')
    parser.add_option('--table-name-uc', dest='table_name_uc', default='caseconv_uc')
    parser.add_option('--table-name-re-canon-lookup', dest='table_name_re_canon_lookup', default='caseconv_re_canon_lookup')
    parser.add_option('--table-name-re-canon-bitmap', dest='table_name_re_canon_bitmap', default='caseconv_re_canon_bitmap')
    (opts, args) = parser.parse_args()

    unicode_data = UnicodeData(opts.unicode_data)
    special_casing = SpecialCasing(opts.special_casing)

    uc, lc, tc = get_base_conversion_maps(unicode_data)
    update_special_casings(uc, lc, tc, special_casing)

    if opts.command == 'caseconv_bitpacked':
        # XXX: ASCII and non-BMP filtering could be an option but is now hardcoded

        # ASCII is handled with 'fast path' so not needed here.
        t = clonedict(uc)
        remove_ascii_part(t)
        uc_bytes, uc_nbits = generate_caseconv_tables(t)

        t = clonedict(lc)
        remove_ascii_part(t)
        lc_bytes, lc_nbits = generate_caseconv_tables(t)

        # Generate C source and header files.
        genc = dukutil.GenerateC()
        genc.emitHeader('extract_caseconv.py')
        genc.emitArray(uc_bytes, opts.table_name_uc, size=len(uc_bytes), typename='duk_uint8_t', intvalues=True, const=True)
        genc.emitArray(lc_bytes, opts.table_name_lc, size=len(lc_bytes), typename='duk_uint8_t', intvalues=True, const=True)
        f = open(opts.out_source, 'wb')
        f.write(genc.getString())
        f.close()

        genc = dukutil.GenerateC()
        genc.emitHeader('extract_caseconv.py')
        genc.emitLine('extern const duk_uint8_t %s[%d];' % (opts.table_name_uc, len(uc_bytes)))
        genc.emitLine('extern const duk_uint8_t %s[%d];' % (opts.table_name_lc, len(lc_bytes)))
        f = open(opts.out_header, 'wb')
        f.write(genc.getString())
        f.close()
    elif opts.command == 're_canon_lookup':
        # Direct canonicalization lookup for case insensitive regexps, includes ascii part.
        t = clonedict(uc)
        re_canon_lookup, re_canon_bitmap = generate_regexp_canonicalize_tables(t)

        genc = dukutil.GenerateC()
        genc.emitHeader('extract_caseconv.py')
        genc.emitArray(re_canon_lookup, opts.table_name_re_canon_lookup, size=len(re_canon_lookup), typename='duk_uint16_t', intvalues=True, const=True)
        f = open(opts.out_source, 'wb')
        f.write(genc.getString())
        f.close()

        genc = dukutil.GenerateC()
        genc.emitHeader('extract_caseconv.py')
        genc.emitLine('extern const duk_uint16_t %s[%d];' % (opts.table_name_re_canon_lookup, len(re_canon_lookup)))
        f = open(opts.out_header, 'wb')
        f.write(genc.getString())
        f.close()
    elif opts.command == 're_canon_bitmap':
        # N-codepoint block bitmap for skipping continuous codepoint blocks
        # quickly.
        t = clonedict(uc)
        re_canon_lookup, re_canon_bitmap = generate_regexp_canonicalize_tables(t)

        genc = dukutil.GenerateC()
        genc.emitHeader('extract_caseconv.py')
        genc.emitArray(re_canon_bitmap['data'], opts.table_name_re_canon_bitmap, size=len(re_canon_bitmap['data']), typename='duk_uint8_t', intvalues=True, const=True)
        f = open(opts.out_source, 'wb')
        f.write(genc.getString())
        f.close()

        genc = dukutil.GenerateC()
        genc.emitHeader('extract_caseconv.py')
        genc.emitDefine('DUK_CANON_BITMAP_BLKSIZE', re_canon_bitmap['block_size'])
        genc.emitDefine('DUK_CANON_BITMAP_BLKSHIFT', re_canon_bitmap['block_shift'])
        genc.emitDefine('DUK_CANON_BITMAP_BLKMASK', re_canon_bitmap['block_mask'])
        genc.emitLine('extern const duk_uint8_t %s[%d];' % (opts.table_name_re_canon_bitmap, len(re_canon_bitmap['data'])))
        f = open(opts.out_header, 'wb')
        f.write(genc.getString())
        f.close()
    else:
        raise Exception('invalid command: %r' % opts.command)

if __name__ == '__main__':
    main()
