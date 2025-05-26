#!/usr/bin/env python2
#
#  Combine a set of a source files into a single C file.
#
#  Overview of the process:
#
#    * Parse user supplied C files.  Add automatic #undefs at the end
#      of each C file to avoid defines bleeding from one file to another.
#
#    * Combine the C files in specified order.  If sources have ordering
#      dependencies (depends on application), order may matter.
#
#    * Process #include statements in the combined source, categorizing
#      them either as "internal" (found in specified include path) or
#      "external".  Internal includes, unless explicitly excluded, are
#      inlined into the result while extenal includes are left as is.
#      Duplicate internal #include statements are replaced with a comment.
#
#  At every step, source and header lines are represented with explicit
#  line objects which keep track of original filename and line.  The
#  output contains #line directives, if requested, to ensure error
#  throwing and other diagnostic info will work in a useful manner when
#  deployed.  It's also possible to generate a combined source with no
#  #line directives.
#
#  Making the process deterministic is important, so that if users have
#  diffs that they apply to the combined source, such diffs would apply
#  for as long as possible.
#
#  Limitations and notes:
#
#    * While there are automatic #undef's for #define's introduced in each
#      C file, it's not possible to "undefine" structs, unions, etc.  If
#      there are structs/unions/typedefs with conflicting names, these
#      have to be resolved in the source files first.
#
#    * Because duplicate #include statements are suppressed, currently
#      assumes #include statements are not conditional.
#
#    * A system header might be #include'd in multiple source files with
#      different feature defines (like _BSD_SOURCE).  Because the #include
#      file will only appear once in the resulting source, the first
#      occurrence wins.  The result may not work correctly if the feature
#      defines must actually be different between two or more source files.
#

import logging
import sys
logging.basicConfig(level=logging.INFO, stream=sys.stdout, format='%(name)-21s %(levelname)-7s %(message)s')
logger = logging.getLogger('combine_src.py')
logger.setLevel(logging.INFO)

import os
import re
import json
import optparse
import logging

# Include path for finding include files which are amalgamated.
include_paths = []

# Include files specifically excluded from being inlined.
include_excluded = []

class File:
    filename_full = None
    filename = None
    lines = None

    def __init__(self, filename, lines):
        self.filename = os.path.basename(filename)
        self.filename_full = filename
        self.lines = lines

class Line:
    filename_full = None
    filename = None
    lineno = None
    data = None

    def __init__(self, filename, lineno, data):
        self.filename = os.path.basename(filename)
        self.filename_full = filename
        self.lineno = lineno
        self.data = data

def readFile(filename):
    lines = []

    with open(filename, 'rb') as f:
        lineno = 0
        for line in f:
            lineno += 1
            if len(line) > 0 and line[-1] == '\n':
                line = line[:-1]
            lines.append(Line(filename, lineno, line))

    return File(filename, lines)

def lookupInclude(incfn):
    re_sep = re.compile(r'/|\\')

    inccomp = re.split(re_sep, incfn)  # split include path, support / and \

    for path in include_paths:
        fn = apply(os.path.join, [ path ] + inccomp)
        if os.path.exists(fn):
            return fn  # Return full path to first match

    return None

def addAutomaticUndefs(f):
    defined = {}

    re_def = re.compile(r'#define\s+(\w+).*$')
    re_undef = re.compile(r'#undef\s+(\w+).*$')

    for line in f.lines:
        m = re_def.match(line.data)
        if m is not None:
            #logger.debug('DEFINED: %s' % repr(m.group(1)))
            defined[m.group(1)] = True
        m = re_undef.match(line.data)
        if m is not None:
            # Could just ignore #undef's here: we'd then emit
            # reliable #undef's (though maybe duplicates) at
            # the end.
            #logger.debug('UNDEFINED: %s' % repr(m.group(1)))
            if defined.has_key(m.group(1)):
                del defined[m.group(1)]

    # Undefine anything that seems to be left defined.  This not a 100%
    # process because some #undef's might be conditional which we don't
    # track at the moment.  Note that it's safe to #undef something that's
    # not defined.

    keys = sorted(defined.keys())  # deterministic order
    if len(keys) > 0:
        #logger.debug('STILL DEFINED: %r' % repr(defined.keys()))
        f.lines.append(Line(f.filename, len(f.lines) + 1, ''))
        f.lines.append(Line(f.filename, len(f.lines) + 1, '/* automatic undefs */'))
        for k in keys:
            logger.debug('automatic #undef for ' + k)
            f.lines.append(Line(f.filename, len(f.lines) + 1, '#undef %s' % k))

def createCombined(files, prologue_filename, line_directives):
    res = []
    line_map = []   # indicate combined source lines where uncombined file/line would change
    metadata = {
        'line_map': line_map
    }

    emit_state = [ None, None ]  # curr_filename, curr_lineno

    def emit(line):
        if isinstance(line, (str, unicode)):
            res.append(line)
            emit_state[1] += 1
        else:
            if line.filename != emit_state[0] or line.lineno != emit_state[1]:
                if line_directives:
                    res.append('#line %d "%s"' % (line.lineno, line.filename))
                line_map.append({ 'original_file': line.filename,
                                  'original_line': line.lineno,
                                  'combined_line': len(res) + 1 })
            res.append(line.data)
            emit_state[0] = line.filename
            emit_state[1] = line.lineno + 1

    included = {}  # headers already included

    if prologue_filename is not None:
        with open(prologue_filename, 'rb') as f:
            for line in f.read().split('\n'):
                res.append(line)

    re_inc = re.compile(r'^#include\s+(<|\")(.*?)(>|\").*$')

    # Process a file, appending it to the result; the input may be a
    # source or an include file.  #include directives are handled
    # recursively.
    def processFile(f):
        logger.debug('Process file: ' + f.filename)

        for line in f.lines:
            if not line.data.startswith('#include'):
                emit(line)
                continue

            m = re_inc.match(line.data)
            if m is None:
                raise Exception('Couldn\'t match #include line: %s' % repr(line.data))
            incpath = m.group(2)
            if incpath in include_excluded:
                # Specific include files excluded from the
                # inlining / duplicate suppression process.
                emit(line)  # keep as is
                continue

            if included.has_key(incpath):
                # We suppress duplicate includes, both internal and
                # external, based on the assumption that includes are
                # not behind #if defined() checks.  This is the case for
                # Duktape (except for the include files excluded).
                emit('/* #include %s -> already included */' % incpath)
                continue
            included[incpath] = True

            # An include file is considered "internal" and is amalgamated
            # if it is found in the include path provided by the user.

            incfile = lookupInclude(incpath)
            if incfile is not None:
                logger.debug('Include considered internal: %s -> %s' % (repr(line.data), repr(incfile)))
                emit('/* #include %s */' % incpath)
                processFile(readFile(incfile))
            else:
                logger.debug('Include considered external: %s' % repr(line.data))
                emit(line)  # keep as is

    for f in files:
        processFile(f)

    return '\n'.join(res) + '\n', metadata

def main():
    global include_paths, include_excluded

    parser = optparse.OptionParser()
    parser.add_option('--include-path', dest='include_paths', action='append', default=[], help='Include directory for "internal" includes, can be specified multiple times')
    parser.add_option('--include-exclude', dest='include_excluded', action='append', default=[], help='Include file excluded from being considered internal (even if found in include dirs)')
    parser.add_option('--prologue', dest='prologue', help='Prologue to prepend to start of file')
    parser.add_option('--output-source', dest='output_source', help='Output source filename')
    parser.add_option('--output-metadata', dest='output_metadata', help='Output metadata filename')
    parser.add_option('--line-directives', dest='line_directives', action='store_true', default=False, help='Use #line directives in combined source')
    parser.add_option('--quiet', dest='quiet', action='store_true', default=False, help='Suppress info messages (show warnings)')
    parser.add_option('--verbose', dest='verbose', action='store_true', default=False, help='Show verbose debug messages')
    (opts, args) = parser.parse_args()

    assert(opts.include_paths is not None)
    include_paths = opts.include_paths  # global for easy access
    include_excluded = opts.include_excluded
    assert(opts.output_source)
    assert(opts.output_metadata)

    # Log level.
    if opts.quiet:
        logger.setLevel(logging.WARNING)
    elif opts.verbose:
        logger.setLevel(logging.DEBUG)

    # Read input files, add automatic #undefs
    sources = args
    files = []
    for fn in sources:
        res = readFile(fn)
        logger.debug('Add automatic undefs for: ' + fn)
        addAutomaticUndefs(res)
        files.append(res)

    combined_source, metadata = \
        createCombined(files, opts.prologue, opts.line_directives)
    with open(opts.output_source, 'wb') as f:
        f.write(combined_source)
    with open(opts.output_metadata, 'wb') as f:
        f.write(json.dumps(metadata, indent=4))

    logger.info('Combined %d source files, %d bytes written to %s' % (len(files), len(combined_source), opts.output_source))

if __name__ == '__main__':
    main()
