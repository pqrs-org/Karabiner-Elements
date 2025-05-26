#!/usr/bin/env python2
#
#  Utility to dump bytecode into a human readable form.
#

import os
import sys
import optparse
import struct
import yaml

script_path = sys.path[0]  # http://stackoverflow.com/questions/4934806/how-can-i-find-scripts-directory-with-python

ops = None

def decode_string(buf, off):
    strlen, = struct.unpack('>L', buf[off:off + 4])
    off += 4
    strdata = buf[off:off + strlen]
    off += strlen

    return off, strdata

def sanitize_string(val):
    # Don't try to UTF-8 decode, just escape non-printable ASCII.
    def f(c):
        if ord(c) < 0x20 or ord(c) > 0x7e or c in '\'"':
            return '\\x%02x' % ord(c)
        else:
            return c

    return "'" + ''.join(map(f, val)) + "'"

def decode_sanitize_string(buf, off):
    off, val = decode_string(buf, off)
    return off, sanitize_string(val)

def dump_ins(ins, x):
    global ops

    if ops is None:
        return ''

    pc = x / 4
    args = []

    op = ops[ins & 0xff]
    comments = []

    if 'args' in op:
        for j in xrange(len(op['args'])):
            A = (ins >> 8) & 0xff
            B = (ins >> 16) & 0xff
            C = (ins >> 24) & 0xff
            BC = (ins >> 16) & 0xffff
            ABC = (ins >> 8) & 0xffffff

            Bconst = ins & 0x1
            Cconst = ins & 0x2

            if op['args'][j] == 'A_R':
                args.append('r' + str(A))

            elif op['args'][j] == 'A_RI':
                args.append('r' + str(A) + '(indirect)')

            elif op['args'][j] == 'A_C':
                args.append('c' + str(A))

            elif op['args'][j] == 'A_H':
                args.append(hex(A))

            elif op['args'][j] == 'A_I':
                args.append(str(A))

            elif op['args'][j] == 'A_B':
                args.append('true' if A else 'false')

            elif op['args'][j] == 'B_RC':
                args.append('c' if Bconst else 'r' + str(B))

            elif op['args'][j] == 'B_R':
                args.append('r' + str(B))

            elif op['args'][j] == 'B_RI':
                args.append('r' + str(B) + '(indirect)')

            elif op['args'][j] == 'B_C':
                args.append('c' + str(B))

            elif op['args'][j] == 'B_H':
                args.append(hex(B))

            elif op['args'][j] == 'B_I':
                args.append(str(B))

            elif op['args'][j] == 'C_RC':
                args.append('c' if Cconst else 'r' + str(C))

            elif op['args'][j] == 'C_R':
                args.append('r' + str(C))

            elif op['args'][j] == 'C_RI':
                args.append('r' + str(C) + '(indirect)')

            elif op['args'][j] == 'C_C':
                args.append('c' + str(C))

            elif op['args'][j] == 'C_H':
                args.append(hex(C))

            elif op['args'][j] == 'C_I':
                args.append(str(C))

            elif op['args'][j] == 'BC_R':
                args.append('r' + str(BC))

            elif op['args'][j] == 'BC_C':
                args.append('c' + str(BC))

            elif op['args'][j] == 'BC_H':
                args.append(hex(BC))

            elif op['args'][j] == 'BC_I':
                args.append(str(BC))

            elif op['args'][j] == 'ABC_H':
                args.append(hex(ABC))

            elif op['args'][j] == 'ABC_I':
                args.append(str(ABC))

            elif op['args'][j] == 'BC_LDINT':
                args.append(hex(BC - (1 << 15)))

            elif op['args'][j] == 'BC_LDINTX':
                args.append(hex(BC))
            elif op['args'][j] == 'ABC_JUMP':
                pc_add = ABC - (1 << 23) + 1
                pc_dst = pc + pc_add
                args.append(str(pc_dst) + ' (' + ('+' if pc_add >= 0 else '') + str(pc_add) + ')')
            else:
                args.append('?')

    if 'flags' in op:
        for f in op['flags']:
            if ins & f['mask']:
                comments.append(f['name'])

    if len(args) > 0:
        res = '%-12s %s' % (op['name'], ', '.join(args))
    else:
        res = op['name']
    if len(comments) > 0:
        res = '%-44s ; %s' % (res, ', '.join(comments))

    return res

def dump_function(buf, off, ind):
    count_inst, count_const, count_funcs = struct.unpack('>LLL', buf[off:off + 12])
    off += 12
    print('%sInstructions: %d' % (ind, count_inst))
    print('%sConstants: %d' % (ind, count_const))
    print('%sInner functions: %d' % (ind, count_funcs))

    # Line numbers present, assuming debugger support; otherwise 0.
    nregs, nargs, start_line, end_line = struct.unpack('>HHLL', buf[off:off + 12])
    off += 12
    print('%sNregs: %d' % (ind, nregs))
    print('%sNargs: %d' % (ind, nargs))
    print('%sStart line number: %d' % (ind, start_line))
    print('%sEnd line number: %d' % (ind, end_line))

    compfunc_flags, = struct.unpack('>L', buf[off:off + 4])
    off += 4
    print('%sduk_hcompiledfunction flags: 0x%08x' % (ind, compfunc_flags))

    for i in xrange(count_inst):
        ins, = struct.unpack('>L', buf[off:off + 4])
        off += 4
        code = dump_ins(ins, i)
        print('%s  %06d: %08lx %s' % (ind, i, ins, code))

    print('%sConstants:' % ind)
    for i in xrange(count_const):
        const_type, = struct.unpack('B', buf[off:off + 1])
        off += 1

        if const_type == 0x00:
            off, strdata = decode_sanitize_string(buf, off)
            print('%s  %06d: %s' % (ind, i, strdata))
        elif const_type == 0x01:
            num, = struct.unpack('>d', buf[off:off + 8])
            off += 8
            print('%s  %06d: %f' % (ind, i, num))
        else:
            raise Exception('invalid constant type: %d' % const_type)

    for i in xrange(count_funcs):
        print('%sInner function %d:' % (ind, i))
        off = dump_function(buf, off, ind + '  ')

    val, = struct.unpack('>L', buf[off:off + 4])
    off += 4
    print('%s.length: %d' % (ind, val))
    off, val = decode_sanitize_string(buf, off)
    print('%s.name: %s' % (ind, val))
    off, val = decode_sanitize_string(buf, off)
    print('%s.fileName: %s' % (ind, val))
    off, val = decode_string(buf, off)  # actually a buffer
    print('%s._Pc2line: %s' % (ind, val.encode('hex')))

    while True:
        off, name = decode_string(buf, off)
        if name == '':
            break
        name = sanitize_string(name)
        val, = struct.unpack('>L', buf[off:off + 4])
        off += 4
        print('%s_Varmap[%s] = %d' % (ind, name, val))

    num_formals, = struct.unpack('>L', buf[off:off + 4])
    off += 4
    if num_formals != 0xffffffff:
        print('%s_Formals: %d formal arguments' % (ind, num_formals))
        for idx in xrange(num_formals):
            off, name = decode_string(buf, off)
            name = sanitize_string(name)
            print('%s_Formals[%d] = %s' % (ind, idx, name))
    else:
        print('%s_Formals: absent' % ind)

    return off

def dump_bytecode(buf, off, ind):
    sig, = struct.unpack('B', buf[off:off + 1])
    print('%sSignature byte: 0x%02x' % (ind, sig))
    off += 1
    if sig == 0xff:
        raise Exception('pre-Duktape 2.2 0xFF signature byte (signature byte is 0xBF since Duktape 2.2)')
    if sig != 0xbf:
        raise Exception('invalid signature byte: %d' % sig)

    off = dump_function(buf, off, ind + '  ')

    return off

def main():
    global ops

    for ops_path in [ '.',
                      os.path.join('..', 'debugger'),
                      script_path,
                      os.path.join(script_path, '..', 'debugger') ]:
        fn = os.path.join(ops_path, 'duk_opcodes.yaml')
        if os.path.isfile(fn):
            with open(fn) as f:
                ops = yaml.load(f)['opcodes']

    if ops is None:
        print('WARN: duk_opcodes.yaml not found, unable do dump opcodes!')

    parser = optparse.OptionParser()
    parser.add_option('--hex-decode', dest='hex_decode', default=False, action='store_true',
                      help='Input file is ASCII hex encoded, decode before dump')
    (opts, args) = parser.parse_args()

    with open(args[0], 'rb') as f:
        d = f.read()
        if opts.hex_decode:
            d = d.strip()
            d = d.decode('hex')
    dump_bytecode(d, 0, '')

if __name__ == '__main__':
    main()
