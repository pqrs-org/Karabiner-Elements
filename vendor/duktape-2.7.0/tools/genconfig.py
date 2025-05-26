#!/usr/bin/env python2
#
#  Process Duktape option metadata and produce various useful outputs:
#
#    - duk_config.h with specific or autodetected platform, compiler, and
#      architecture, forced options, sanity checks, etc
#    - option documentation for Duktape config options (DUK_USE_xxx)
#
#  Genconfig tries to build all outputs based on modular metadata, so that
#  managing a large number of config options (which is hard to avoid given
#  the wide range of targets Duktape supports) remains maintainable.
#
#  Genconfig does *not* try to support all exotic platforms out there.
#  Instead, the goal is to allow the metadata to be extended, or to provide
#  a reasonable starting point for manual duk_config.h tweaking.
#

import logging
import sys
logging.basicConfig(level=logging.INFO, stream=sys.stdout, format='%(name)-21s %(levelname)-7s %(message)s')
logger = logging.getLogger('genconfig.py')
logger.setLevel(logging.INFO)

import os
import re
import json
import yaml
import optparse
import tarfile
import tempfile
import atexit
import shutil
import logging
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

#
#  Globals holding scanned metadata, helper snippets, etc
#

# Metadata to scan from config files.
use_defs = None
use_defs_list = None
opt_defs = None
opt_defs_list = None
use_tags = None
use_tags_list = None
tags_meta = None
required_use_meta_keys = [
    'define',
    'introduced',
    'default',
    'tags',
    'description'
]
allowed_use_meta_keys = [
    'define',
    'introduced',
    'deprecated',
    'removed',
    'unused',
    'requires',
    'conflicts',
    'related',
    'default',
    'tags',
    'description',
    'warn_if_missing'
]
required_opt_meta_keys = [
    'define',
    'introduced',
    'tags',
    'description'
]
allowed_opt_meta_keys = [
    'define',
    'introduced',
    'deprecated',
    'removed',
    'unused',
    'requires',
    'conflicts',
    'related',
    'tags',
    'description'
]

# Preferred tag order for option documentation.
doc_tag_order = [
    'portability',
    'memory',
    'lowmemory',
    'ecmascript',
    'execution',
    'debugger',
    'debug',
    'development'
]

# Preferred tag order for generated C header files.
header_tag_order = doc_tag_order

# Helper headers snippets.
helper_snippets = None

# Assume these provides come from outside.
assumed_provides = {
    'DUK_SINGLE_FILE': True,         # compiling Duktape from a single source file (duktape.c) version
    'DUK_COMPILING_DUKTAPE': True,   # compiling Duktape (not user application)
    'DUK_CONFIG_H_INCLUDED': True,   # artifact, include guard
}

# Platform files must provide at least these (additional checks
# in validate_platform_file()).  Fill-ins provide missing optionals.
platform_required_provides = [
    'DUK_USE_OS_STRING'  # must be #define'd
]

# Architecture files must provide at least these (additional checks
# in validate_architecture_file()).  Fill-ins provide missing optionals.
architecture_required_provides = [
    'DUK_USE_ARCH_STRING'
]

# Compiler files must provide at least these (additional checks
# in validate_compiler_file()).  Fill-ins provide missing optionals.
compiler_required_provides = [
    # Compilers need a lot of defines; missing defines are automatically
    # filled in with defaults (which are mostly compiler independent), so
    # the requires define list is not very large.

    'DUK_USE_COMPILER_STRING',    # must be #define'd
    'DUK_USE_BRANCH_HINTS',       # may be #undef'd, as long as provided
    'DUK_USE_VARIADIC_MACROS',    # may be #undef'd, as long as provided
    'DUK_USE_UNION_INITIALIZERS'  # may be #undef'd, as long as provided
]

#
#  Miscellaneous helpers
#

def get_auto_delete_tempdir():
    tmpdir = tempfile.mkdtemp(suffix='-genconfig')
    def _f(dirname):
        logger.debug('Deleting temporary directory: %r' % dirname)
        if os.path.isdir(dirname) and '-genconfig' in dirname:
            shutil.rmtree(dirname)
    atexit.register(_f, tmpdir)
    return tmpdir

def strip_comments_from_lines(lines):
    # Not exact but close enough.  Doesn't handle string literals etc,
    # but these are not a concrete issue for scanning preprocessor
    # #define references.
    #
    # Comment contents are stripped of any DUK_ prefixed text to avoid
    # incorrect requires/provides detection.  Other comment text is kept;
    # in particular a "/* redefine */" comment must remain intact here.
    # (The 'redefine' hack is not actively needed now.)
    #
    # Avoid Python 2.6 vs. Python 2.7 argument differences.

    def censor(x):
        return re.sub(re.compile('DUK_\w+', re.MULTILINE), 'xxx', x.group(0))

    tmp = '\n'.join(lines)
    tmp = re.sub(re.compile('/\*.*?\*/', re.MULTILINE | re.DOTALL), censor, tmp)
    tmp = re.sub(re.compile('//.*?$', re.MULTILINE), censor, tmp)
    return tmp.split('\n')

# Header snippet representation: lines, provides defines, requires defines.
re_line_provides = re.compile(r'^#(?:define|undef)\s+(\w+).*$')
re_line_requires = re.compile(r'(DUK_[A-Z0-9_]+)')  # uppercase only, don't match DUK_USE_xxx for example
class Snippet:
    lines = None     # lines of text and/or snippets
    provides = None  # map from define to 'True' for now
    requires = None  # map from define to 'True' for now

    def __init__(self, lines, provides=None, requires=None, autoscan_requires=True, autoscan_provides=True):
        self.lines = []
        if not isinstance(lines, list):
            raise Exception('Snippet constructor must be a list (not e.g. a string): %s' % repr(lines))
        for line in lines:
            if isinstance(line, str):
                self.lines.append(line)
            elif isinstance(line, unicode):
                self.lines.append(line.encode('utf-8'))
            else:
                raise Exception('invalid line: %r' % line)
        self.provides = {}
        if provides is not None:
            for k in provides.keys():
                self.provides[k] = True
        self.requires = {}
        if requires is not None:
            for k in requires.keys():
                self.requires[k] = True

        stripped_lines = strip_comments_from_lines(lines)
        #for line in stripped_lines:
        #    logger.debug(line)

        for line in stripped_lines:
            # Careful with order, snippet may self-reference its own
            # defines in which case there's no outward dependency.
            # (This is not 100% because the order of require/provide
            # matters and this is not handled now.)
            #
            # Also, some snippets may #undef/#define another define but
            # they don't "provide" the define as such.  Such redefinitions
            # are marked "/* redefine */" in the snippets.  They're best
            # avoided (and not currently needed in Duktape 1.4.0).

            if autoscan_provides:
                m = re_line_provides.match(line)
                if m is not None and '/* redefine */' not in line and \
                    len(m.group(1)) > 0 and m.group(1)[-1] != '_':
                    # Don't allow e.g. DUK_USE_ which results from matching DUK_USE_xxx
                    #logger.debug('PROVIDES: %r' % m.group(1))
                    self.provides[m.group(1)] = True
            if autoscan_requires:
                matches = re.findall(re_line_requires, line)
                for m in matches:
                    if len(m) > 0 and m[-1] == '_':
                        # Don't allow e.g. DUK_USE_ which results from matching DUK_USE_xxx
                        pass
                    elif m[:7] == 'DUK_OPT':
                        #logger.warning('Encountered DUK_OPT_xxx in a header snippet: %s' % repr(line))
                        # DUK_OPT_xxx always come from outside
                        pass
                    elif m[:7] == 'DUK_USE':
                        # DUK_USE_xxx are internal and they should not be 'requirements'
                        pass
                    elif self.provides.has_key(m):
                        # Snippet provides it's own require; omit
                        pass
                    else:
                        #logger.debug('REQUIRES: %r' % m)
                        self.requires[m] = True

    def fromFile(cls, filename):
        lines = []
        with open(filename, 'rb') as f:
            for line in f:
                if line[-1] == '\n':
                    line = line[:-1]
                if line[:8] == '#snippet':
                    m = re.match(r'#snippet\s+"(.*?)"', line)
                    # XXX: better plumbing for lookup path
                    sub_fn = os.path.normpath(os.path.join(filename, '..', '..', 'header-snippets', m.group(1)))
                    logger.debug('#snippet ' + sub_fn)
                    sn = Snippet.fromFile(sub_fn)
                    lines += sn.lines
                else:
                    lines.append(line)
        return Snippet(lines, autoscan_requires=True, autoscan_provides=True)
    fromFile = classmethod(fromFile)

    def merge(cls, snippets):
        ret = Snippet([], [], [])
        for s in snippets:
            ret.lines += s.lines
            for k in s.provides.keys():
                ret.provides[k] = True
            for k in s.requires.keys():
                ret.requires[k] = True
        return ret
    merge = classmethod(merge)

# Helper for building a text file from individual lines, injected files, etc.
# Inserted values are converted to Snippets so that their provides/requires
# information can be tracked.  When non-C outputs are created, these will be
# bogus but ignored.
class FileBuilder:
    vals = None  # snippet list
    base_dir = None
    use_cpp_warning = False

    def __init__(self, base_dir=None, use_cpp_warning=False):
        self.vals = []
        self.base_dir = base_dir
        self.use_cpp_warning = use_cpp_warning

    def line(self, line):
        self.vals.append(Snippet([ line ]))

    def lines(self, lines):
        if len(lines) > 0 and lines[-1] == '\n':
            lines = lines[:-1]  # strip last newline to avoid empty line
        self.vals.append(Snippet(lines.split('\n')))

    def empty(self):
        self.vals.append(Snippet([ '' ]))

    def rst_heading(self, title, char, doubled=False):
        tmp = []
        if doubled:
            tmp.append(char * len(title))
        tmp.append(title)
        tmp.append(char * len(title))
        self.vals.append(Snippet(tmp))

    def snippet_relative(self, fn):
        sn = Snippet.fromFile(os.path.join(self.base_dir, fn))
        self.vals.append(sn)
        return sn

    def snippet_absolute(self, fn):
        sn = Snippet.fromFile(fn)
        self.vals.append(sn)
        return sn

    def cpp_error(self, msg):
        # XXX: assume no newlines etc
        self.vals.append(Snippet([ '#error %s' % msg ]))

    def cpp_warning(self, msg):
        # XXX: assume no newlines etc
        # XXX: support compiler specific warning mechanisms
        if self.use_cpp_warning:
            # C preprocessor '#warning' is often supported
            self.vals.append(Snippet([ '#warning %s' % msg ]))
        else:
            self.vals.append(Snippet([ '/* WARNING: %s */' % msg ]))

    def cpp_warning_or_error(self, msg, is_error=True):
        if is_error:
            self.cpp_error(msg)
        else:
            self.cpp_warning(msg)

    def chdr_comment_line(self, msg):
        self.vals.append(Snippet([ '/* %s */' % msg ]))

    def chdr_block_heading(self, msg):
        lines = []
        lines.append('')
        lines.append('/*')
        lines.append(' *  ' + msg)
        lines.append(' */')
        lines.append('')
        self.vals.append(Snippet(lines))

    def join(self):
        tmp = []
        for line in self.vals:
            if not isinstance(line, object):
                raise Exception('self.vals must be all snippets')
            for x in line.lines:  # x is a Snippet
                tmp.append(x)
        return '\n'.join(tmp)

    def fill_dependencies_for_snippets(self, idx_deps):
        fill_dependencies_for_snippets(self.vals, idx_deps)

# Insert missing define dependencies into index 'idx_deps' repeatedly
# until no unsatisfied dependencies exist.  This is used to pull in
# the required DUK_F_xxx helper defines without pulling them all in.
# The resolution mechanism also ensures dependencies are pulled in the
# correct order, i.e. DUK_F_xxx helpers may depend on each other (as
# long as there are no circular dependencies).
#
# XXX: this can be simplified a lot
def fill_dependencies_for_snippets(snippets, idx_deps):
    # graph[A] = [ B, ... ] <-> B, ... provide something A requires.
    graph = {}
    snlist = []
    resolved = []   # for printing only

    def add(sn):
        if sn in snlist:
            return  # already present
        snlist.append(sn)

        to_add = []

        for k in sn.requires.keys():
            if assumed_provides.has_key(k):
                continue

            found = False
            for sn2 in snlist:
                if sn2.provides.has_key(k):
                    if not graph.has_key(sn):
                        graph[sn] = []
                    graph[sn].append(sn2)
                    found = True  # at least one other node provides 'k'

            if not found:
                logger.debug('Resolving %r' % k)
                resolved.append(k)

                # Find a header snippet which provides the missing define.
                # Some DUK_F_xxx files provide multiple defines, so we don't
                # necessarily know the snippet filename here.

                sn_req = None
                for sn2 in helper_snippets:
                    if sn2.provides.has_key(k):
                        sn_req = sn2
                        break
                if sn_req is None:
                    logger.debug(repr(sn.lines))
                    raise Exception('cannot resolve missing require: %r' % k)

                # Snippet may have further unresolved provides; add recursively
                to_add.append(sn_req)

                if not graph.has_key(sn):
                    graph[sn] = []
                graph[sn].append(sn_req)

        for sn in to_add:
            add(sn)

    # Add original snippets.  This fills in the required nodes
    # recursively.
    for sn in snippets:
        add(sn)

    # Figure out fill-ins by looking for snippets not in original
    # list and without any unserialized dependent nodes.
    handled = {}
    for sn in snippets:
        handled[sn] = True
    keepgoing = True
    while keepgoing:
        keepgoing = False
        for sn in snlist:
            if handled.has_key(sn):
                continue

            success = True
            for dep in graph.get(sn, []):
                if not handled.has_key(dep):
                    success = False
            if success:
                snippets.insert(idx_deps, sn)
                idx_deps += 1
                snippets.insert(idx_deps, Snippet([ '' ]))
                idx_deps += 1
                handled[sn] = True
                keepgoing = True
                break

    # XXX: detect and handle loops cleanly
    for sn in snlist:
        if handled.has_key(sn):
            continue
        logger.debug('UNHANDLED KEY')
        logger.debug('PROVIDES: %r' % sn.provides)
        logger.debug('REQUIRES: %r' % sn.requires)
        logger.debug('\n'.join(sn.lines))

    #logger.debug(repr(graph))
    #logger.debug(repr(snlist))
    logger.debug('Resolved helper defines: %r' % resolved)
    logger.debug('Resolved %d helper defines' % len(resolved))

def serialize_snippet_list(snippets):
    ret = []

    emitted_provides = {}
    for k in assumed_provides.keys():
        emitted_provides[k] = True

    for sn in snippets:
        ret += sn.lines
        for k in sn.provides.keys():
            emitted_provides[k] = True
        for k in sn.requires.keys():
            if not emitted_provides.has_key(k):
                # XXX: conditional warning, happens in some normal cases
                logger.warning('define %r required, not provided so far' % k)
                pass

    return '\n'.join(ret)

def remove_duplicate_newlines(x):
    ret = []
    empty = False
    for line in x.split('\n'):
        if line == '':
            if empty:
                pass
            else:
                ret.append(line)
            empty = True
        else:
            empty = False
            ret.append(line)
    return '\n'.join(ret)

def scan_use_defs(dirname):
    global use_defs, use_defs_list
    use_defs = {}
    use_defs_list = []

    for fn in os.listdir(dirname):
        root, ext = os.path.splitext(fn)
        if not root.startswith('DUK_USE_') or ext != '.yaml':
            continue
        with open(os.path.join(dirname, fn), 'rb') as f:
            doc = yaml.load(f)
            if doc.get('example', False):
                continue
            if doc.get('unimplemented', False):
                logger.warning('unimplemented: %s' % fn)
                continue
            dockeys = doc.keys()
            for k in dockeys:
                if not k in allowed_use_meta_keys:
                    logger.warning('unknown key %s in metadata file %s' % (k, fn))
            for k in required_use_meta_keys:
                if not k in dockeys:
                    logger.warning('missing key %s in metadata file %s' % (k, fn))

            use_defs[doc['define']] = doc

    keys = use_defs.keys()
    keys.sort()
    for k in keys:
        use_defs_list.append(use_defs[k])

def scan_opt_defs(dirname):
    global opt_defs, opt_defs_list
    opt_defs = {}
    opt_defs_list = []

    for fn in os.listdir(dirname):
        root, ext = os.path.splitext(fn)
        if not root.startswith('DUK_OPT_') or ext != '.yaml':
            continue
        with open(os.path.join(dirname, fn), 'rb') as f:
            doc = yaml.load(f)
            if doc.get('example', False):
                continue
            if doc.get('unimplemented', False):
                logger.warning('unimplemented: %s' % fn)
                continue
            dockeys = doc.keys()
            for k in dockeys:
                if not k in allowed_opt_meta_keys:
                    logger.warning('unknown key %s in metadata file %s' % (k, fn))
            for k in required_opt_meta_keys:
                if not k in dockeys:
                    logger.warning('missing key %s in metadata file %s' % (k, fn))

            opt_defs[doc['define']] = doc

    keys = opt_defs.keys()
    keys.sort()
    for k in keys:
        opt_defs_list.append(opt_defs[k])

def scan_use_tags():
    global use_tags, use_tags_list
    use_tags = {}

    for doc in use_defs_list:
        for tag in doc.get('tags', []):
            use_tags[tag] = True

    use_tags_list = use_tags.keys()
    use_tags_list.sort()

def scan_tags_meta(filename):
    global tags_meta

    with open(filename, 'rb') as f:
        tags_meta = yaml.load(f)

def scan_helper_snippets(dirname):  # DUK_F_xxx snippets
    global helper_snippets
    helper_snippets = []

    for fn in os.listdir(dirname):
        if (fn[0:6] != 'DUK_F_'):
            continue
        logger.debug('Autoscanning snippet: %s' % fn)
        helper_snippets.append(Snippet.fromFile(os.path.join(dirname, fn)))

def get_opt_defs(removed=True, deprecated=True, unused=True):
    ret = []
    for doc in opt_defs_list:
        # XXX: aware of target version
        if removed == False and doc.get('removed', None) is not None:
            continue
        if deprecated == False and doc.get('deprecated', None) is not None:
            continue
        if unused == False and doc.get('unused', False) == True:
            continue
        ret.append(doc)
    return ret

def get_use_defs(removed=True, deprecated=True, unused=True):
    ret = []
    for doc in use_defs_list:
        # XXX: aware of target version
        if removed == False and doc.get('removed', None) is not None:
            continue
        if deprecated == False and doc.get('deprecated', None) is not None:
            continue
        if unused == False and doc.get('unused', False) == True:
            continue
        ret.append(doc)
    return ret

def validate_platform_file(filename):
    sn = Snippet.fromFile(filename)

    for req in platform_required_provides:
        if req not in sn.provides:
            raise Exception('Platform %s is missing %s' % (filename, req))

    # DUK_SETJMP, DUK_LONGJMP, DUK_JMPBUF_TYPE are optional, fill-in
    # provides if none defined.

def validate_architecture_file(filename):
    sn = Snippet.fromFile(filename)

    for req in architecture_required_provides:
        if req not in sn.provides:
            raise Exception('Architecture %s is missing %s' % (filename, req))

    # Byte order and alignment defines are allowed to be missing,
    # a fill-in will handle them.  This is necessary because for
    # some architecture byte order and/or alignment may vary between
    # targets and may be software configurable.

    # XXX: require automatic detection to be signaled?
    # e.g. define DUK_USE_ALIGN_BY -1
    #      define DUK_USE_BYTE_ORDER -1

def validate_compiler_file(filename):
    sn = Snippet.fromFile(filename)

    for req in compiler_required_provides:
        if req not in sn.provides:
            raise Exception('Compiler %s is missing %s' % (filename, req))

def get_tag_title(tag):
    meta = tags_meta.get(tag, None)
    if meta is None:
        return tag
    else:
        return meta.get('title', tag)

def get_tag_description(tag):
    meta = tags_meta.get(tag, None)
    if meta is None:
        return None
    else:
        return meta.get('description', None)

def get_tag_list_with_preferred_order(preferred):
    tags = []

    # Preferred tags first
    for tag in preferred:
        if tag not in tags:
            tags.append(tag)

    # Remaining tags in alphabetic order
    for tag in use_tags_list:
        if tag not in tags:
            tags.append(tag)

    logger.debug('Effective tag order: %r' % tags)
    return tags

def rst_format(text):
    # XXX: placeholder, need to decide on markup conventions for YAML files
    ret = []
    for para in text.split('\n'):
        if para == '':
            continue
        ret.append(para)
    return '\n\n'.join(ret)

def cint_encode(x):
    if not isinstance(x, (int, long)):
        raise Exception('invalid input: %r' % x)

    # XXX: unsigned constants?
    if x > 0x7fffffff or x < -0x80000000:
        return '%dLL' % x
    elif x > 0x7fff or x < -0x8000:
        return '%dL' % x
    else:
        return '%d' % x

def cstr_encode(x):
    if isinstance(x, unicode):
        x = x.encode('utf-8')
    if not isinstance(x, str):
        raise Exception('invalid input: %r' % x)

    res = '"'
    term = False
    has_terms = False
    for c in x:
        if term:
            # Avoid ambiguous hex escapes
            res += '" "'
            term = False
            has_terms = True
        o = ord(c)
        if o < 0x20 or o > 0x7e or c in '"\\':
            res += '\\x%02x' % o
            term = True
        else:
            res += c
    res += '"'

    if has_terms:
        res = '(' + res + ')'

    return res

#
#  Autogeneration of option documentation
#

# Shared helper to generate DUK_USE_xxx documentation.
# XXX: unfinished placeholder
def generate_option_documentation(opts, opt_list=None, rst_title=None, include_default=False):
    ret = FileBuilder(use_cpp_warning=opts.use_cpp_warning)

    tags = get_tag_list_with_preferred_order(doc_tag_order)

    title = rst_title
    ret.rst_heading(title, '=', doubled=True)

    handled = {}

    for tag in tags:
        first = True

        for doc in opt_list:
            if tag != doc['tags'][0]:  # sort under primary tag
                continue
            dname = doc['define']
            desc = doc.get('description', None)

            if handled.has_key(dname):
                raise Exception('define handled twice, should not happen: %r' % dname)
            handled[dname] = True

            if first:  # emit tag heading only if there are subsections
                ret.empty()
                ret.rst_heading(get_tag_title(tag), '=')

                tag_desc = get_tag_description(tag)
                if tag_desc is not None:
                    ret.empty()
                    ret.line(rst_format(tag_desc))
                first = False

            ret.empty()
            ret.rst_heading(dname, '-')

            if desc is not None:
                ret.empty()
                ret.line(rst_format(desc))

            if include_default:
                ret.empty()
                ret.line('Default: ``' + str(doc['default']) + '``')  # XXX: rst or other format

    for doc in opt_list:
        dname = doc['define']
        if not handled.has_key(dname):
            raise Exception('unhandled define (maybe missing from tags list?): %r' % dname)

    ret.empty()
    return ret.join()

def generate_config_option_documentation(opts):
    defs = get_use_defs()
    return generate_option_documentation(opts, opt_list=defs, rst_title='Duktape config options', include_default=True)

#
#  Helpers for duk_config.h generation
#

def get_forced_options(opts):
    # Forced options, last occurrence wins (allows a base config file to be
    # overridden by a more specific one).
    forced_opts = {}
    for val in opts.force_options_yaml:
        doc = yaml.load(StringIO(val))
        for k in doc.keys():
            if use_defs.has_key(k):
                pass  # key is known
            else:
                logger.warning('option override key %s not defined in metadata, ignoring' % k)
            forced_opts[k] = doc[k]  # shallow copy

    if len(forced_opts.keys()) > 0:
        logger.debug('Overrides: %s' % json.dumps(forced_opts))

    return forced_opts

# Emit a default #define / #undef for an option based on
# a config option metadata node (parsed YAML doc).
def emit_default_from_config_meta(ret, doc, forced_opts, undef_done, active_opts):
    defname = doc['define']
    defval = forced_opts.get(defname, doc['default'])

    # NOTE: careful with Python equality, e.g. "0 == False" is true.
    if isinstance(defval, bool) and defval == True:
        ret.line('#define ' + defname)
        active_opts[defname] = True
    elif isinstance(defval, bool) and defval == False:
        if not undef_done:
            ret.line('#undef ' + defname)
        else:
            # Default value is false, and caller has emitted
            # an unconditional #undef, so don't emit a duplicate
            pass
        active_opts[defname] = False
    elif isinstance(defval, (int, long)):
        # integer value
        ret.line('#define ' + defname + ' ' + cint_encode(defval))
        active_opts[defname] = True
    elif isinstance(defval, (str, unicode)):
        # verbatim value
        ret.line('#define ' + defname + ' ' + defval)
        active_opts[defname] = True
    elif isinstance(defval, dict):
        if defval.has_key('verbatim'):
            # verbatim text for the entire line
            ret.line(defval['verbatim'])
        elif defval.has_key('string'):
            # C string value
            ret.line('#define ' + defname + ' ' + cstr_encode(defval['string']))
        else:
            raise Exception('unsupported value for option %s: %r' % (defname, defval))
        active_opts[defname] = True
    else:
        raise Exception('unsupported value for option %s: %r' % (defname, defval))

# Add a header snippet for detecting presence of DUK_OPT_xxx feature
# options and warning/erroring if application defines them.  Useful for
# Duktape 2.x migration.
def add_legacy_feature_option_checks(opts, ret):
    ret.chdr_block_heading('Checks for legacy feature options (DUK_OPT_xxx)')
    ret.empty()

    defs = []
    for doc in get_opt_defs():
        if doc['define'] not in defs:
            defs.append(doc['define'])
    defs.sort()

    for optname in defs:
        ret.line('#if defined(%s)' % optname)
        ret.cpp_warning_or_error('unsupported legacy feature option %s used' % optname, opts.sanity_strict)
        ret.line('#endif')

    ret.empty()

# Add a header snippet for checking consistency of DUK_USE_xxx config
# options, e.g. inconsistent options, invalid option values.
def add_config_option_checks(opts, ret):
    ret.chdr_block_heading('Checks for config option consistency (DUK_USE_xxx)')
    ret.empty()

    defs = []
    for doc in get_use_defs():
        if doc['define'] not in defs:
            defs.append(doc['define'])
    defs.sort()

    for optname in defs:
        doc = use_defs[optname]
        dname = doc['define']

        # XXX: more checks

        if doc.get('removed', None) is not None:
            ret.line('#if defined(%s)' % dname)
            ret.cpp_warning_or_error('unsupported config option used (option has been removed): %s' % dname, opts.sanity_strict)
            ret.line('#endif')
        elif doc.get('deprecated', None) is not None:
            ret.line('#if defined(%s)' % dname)
            ret.cpp_warning_or_error('unsupported config option used (option has been deprecated): %s' % dname, opts.sanity_strict)
            ret.line('#endif')

        for req in doc.get('requires', []):
            ret.line('#if defined(%s) && !defined(%s)' % (dname, req))
            ret.cpp_warning_or_error('config option %s requires option %s (which is missing)' % (dname, req), opts.sanity_strict)
            ret.line('#endif')

        for req in doc.get('conflicts', []):
            ret.line('#if defined(%s) && defined(%s)' % (dname, req))
            ret.cpp_warning_or_error('config option %s conflicts with option %s (which is also defined)' % (dname, req), opts.sanity_strict)
            ret.line('#endif')

    ret.empty()
    ret.snippet_relative('cpp_exception_sanity.h.in')
    ret.empty()

# Add a header snippet for providing a __OVERRIDE_DEFINES__ section.
def add_override_defines_section(opts, ret):
    ret.empty()
    ret.line('/*')
    ret.line(' *  You may add overriding #define/#undef directives below for')
    ret.line(' *  customization.  You of course cannot un-#include or un-typedef')
    ret.line(' *  anything; these require direct changes above.')
    ret.line(' */')
    ret.empty()
    ret.line('/* __OVERRIDE_DEFINES__ */')
    ret.empty()

# Add a header snippet for conditional C/C++ include files.
def add_conditional_includes_section(opts, ret):
    ret.empty()
    ret.line('/*')
    ret.line(' *  Conditional includes')
    ret.line(' */')
    ret.empty()
    ret.snippet_relative('platform_conditionalincludes.h.in')
    ret.empty()

# Development time helper: add DUK_ACTIVE which provides a runtime C string
# indicating what DUK_USE_xxx config options are active at run time.  This
# is useful in genconfig development so that one can e.g. diff the active
# run time options of two headers.  This is intended just for genconfig
# development and is not available in normal headers.
def add_duk_active_defines_macro(ret):
    ret.chdr_block_heading('DUK_ACTIVE_DEFINES macro (development only)')

    idx = 0
    for doc in get_use_defs():
        defname = doc['define']

        ret.line('#if defined(%s)' % defname)
        ret.line('#define DUK_ACTIVE_DEF%d " %s"' % (idx, defname))
        ret.line('#else')
        ret.line('#define DUK_ACTIVE_DEF%d ""' % idx)
        ret.line('#endif')

        idx += 1

    tmp = []
    for i in xrange(idx):
        tmp.append('DUK_ACTIVE_DEF%d' % i)

    ret.line('#define DUK_ACTIVE_DEFINES ("Active: ["' + ' '.join(tmp) + ' " ]")')

#
#  duk_config.h generation
#

# Generate a duk_config.h where platform, architecture, and compiler are
# all either autodetected or specified by user.
#
# Autodetection is based on a configured list of supported platforms,
# architectures, and compilers.  For example, platforms.yaml defines the
# supported platforms and provides a helper define (DUK_F_xxx) to use for
# detecting that platform, and names the header snippet to provide the
# platform-specific definitions.  Necessary dependencies (DUK_F_xxx) are
# automatically pulled in.
#
# Automatic "fill ins" are used for mandatory platform, architecture, and
# compiler defines which have a reasonable portable default.  This reduces
# e.g. compiler-specific define count because there are a lot compiler
# macros which have a good default.
def generate_duk_config_header(opts, meta_dir):
    ret = FileBuilder(base_dir=os.path.join(meta_dir, 'header-snippets'), \
                      use_cpp_warning=opts.use_cpp_warning)

    # Parse forced options.  Warn about missing forced options when it is
    # strongly recommended that the option is provided.
    forced_opts = get_forced_options(opts)
    for doc in use_defs_list:
        if doc.get('warn_if_missing', False) and not forced_opts.has_key(doc['define']):
            # Awkward handling for DUK_USE_CPP_EXCEPTIONS + DUK_USE_FATAL_HANDLER.
            if doc['define'] == 'DUK_USE_FATAL_HANDLER' and forced_opts.has_key('DUK_USE_CPP_EXCEPTIONS'):
                pass  # DUK_USE_FATAL_HANDLER not critical with DUK_USE_CPP_EXCEPTIONS
            else:
                logger.warning('Recommended config option ' + doc['define'] + ' not provided')

    # Gather a map of "active options" for genbuiltins.py.  This is used to
    # implement proper optional built-ins, e.g. if a certain config option
    # (like DUK_USE_ES6_PROXY) is disabled, the corresponding objects and
    # properties are dropped entirely.  The mechanism is not perfect: it won't
    # detect fixup changes for example.
    active_opts = {}

    platforms = None
    with open(os.path.join(meta_dir, 'platforms.yaml'), 'rb') as f:
        platforms = yaml.load(f)
    architectures = None
    with open(os.path.join(meta_dir, 'architectures.yaml'), 'rb') as f:
        architectures = yaml.load(f)
    compilers = None
    with open(os.path.join(meta_dir, 'compilers.yaml'), 'rb') as f:
        compilers = yaml.load(f)

    # XXX: indicate feature option support, sanity checks enabled, etc
    # in general summary of options, perhaps genconfig command line?

    ret.line('/*')
    ret.line(' *  duk_config.h configuration header generated by genconfig.py.')
    ret.line(' *')
    ret.line(' *  Git commit: %s' % opts.git_commit or 'n/a')
    ret.line(' *  Git describe: %s' % opts.git_describe or 'n/a')
    ret.line(' *  Git branch: %s' % opts.git_branch or 'n/a')
    ret.line(' *')
    if opts.platform is not None:
        ret.line(' *  Platform: ' + opts.platform)
    else:
        ret.line(' *  Supported platforms:')
        for platf in platforms['autodetect']:
            ret.line(' *      - %s' % platf.get('name', platf.get('check')))
    ret.line(' *')
    if opts.architecture is not None:
        ret.line(' *  Architecture: ' + opts.architecture)
    else:
        ret.line(' *  Supported architectures:')
        for arch in architectures['autodetect']:
            ret.line(' *      - %s' % arch.get('name', arch.get('check')))
    ret.line(' *')
    if opts.compiler is not None:
        ret.line(' *  Compiler: ' + opts.compiler)
    else:
        ret.line(' *  Supported compilers:')
        for comp in compilers['autodetect']:
            ret.line(' *      - %s' % comp.get('name', comp.get('check')))
    ret.line(' *')
    ret.line(' */')
    ret.empty()
    ret.line('#if !defined(DUK_CONFIG_H_INCLUDED)')
    ret.line('#define DUK_CONFIG_H_INCLUDED')
    ret.empty()

    ret.chdr_block_heading('Intermediate helper defines')

    # DLL build affects visibility attributes on Windows but unfortunately
    # cannot be detected automatically from preprocessor defines or such.
    # DLL build status is hidden behind DUK_F_DLL_BUILD. and there are two
    ret.chdr_comment_line('DLL build detection')
    if opts.dll:
        ret.line('/* configured for DLL build */')
        ret.line('#define DUK_F_DLL_BUILD')
    else:
        ret.line('/* not configured for DLL build */')
        ret.line('#undef DUK_F_DLL_BUILD')
    ret.empty()

    idx_deps = len(ret.vals)  # position where to emit DUK_F_xxx dependencies

    # Feature selection, system include, Date provider
    # Most #include statements are here

    if opts.platform is not None:
        ret.chdr_block_heading('Platform: ' + opts.platform)

        ret.snippet_relative('platform_cppextras.h.in')
        ret.empty()

        # XXX: better to lookup platforms metadata
        include = 'platform_%s.h.in' % opts.platform
        abs_fn = os.path.join(meta_dir, 'platforms', include)
        validate_platform_file(abs_fn)
        ret.snippet_absolute(abs_fn)
    else:
        ret.chdr_block_heading('Platform autodetection')

        ret.snippet_relative('platform_cppextras.h.in')
        ret.empty()

        for idx, platf in enumerate(platforms['autodetect']):
            check = platf.get('check', None)
            include = platf['include']
            abs_fn = os.path.join(meta_dir, 'platforms', include)

            validate_platform_file(abs_fn)

            if idx == 0:
                ret.line('#if defined(%s)' % check)
            else:
                if check is None:
                    ret.line('#else')
                else:
                    ret.line('#elif defined(%s)' % check)
            ret.line('/* --- %s --- */' % platf.get('name', '???'))
            ret.snippet_absolute(abs_fn)
        ret.line('#endif  /* autodetect platform */')

    ret.empty()
    ret.snippet_relative('platform_sharedincludes.h.in')
    ret.empty()

    byteorder_provided_by_all = True  # byteorder provided by all architecture files
    alignment_provided_by_all = True  # alignment provided by all architecture files
    packedtval_provided_by_all = True # packed tval provided by all architecture files

    if opts.architecture is not None:
        ret.chdr_block_heading('Architecture: ' + opts.architecture)

        # XXX: better to lookup architectures metadata
        include = 'architecture_%s.h.in' % opts.architecture
        abs_fn = os.path.join(meta_dir, 'architectures', include)
        validate_architecture_file(abs_fn)
        sn = ret.snippet_absolute(abs_fn)
        if not sn.provides.get('DUK_USE_BYTEORDER', False):
            byteorder_provided_by_all = False
        if not sn.provides.get('DUK_USE_ALIGN_BY', False):
            alignment_provided_by_all = False
        if sn.provides.get('DUK_USE_PACKED_TVAL', False):
            ret.line('#define DUK_F_PACKED_TVAL_PROVIDED')  # signal to fillin
        else:
            packedtval_provided_by_all = False
    else:
        ret.chdr_block_heading('Architecture autodetection')

        for idx, arch in enumerate(architectures['autodetect']):
            check = arch.get('check', None)
            include = arch['include']
            abs_fn = os.path.join(meta_dir, 'architectures', include)

            validate_architecture_file(abs_fn)

            if idx == 0:
                ret.line('#if defined(%s)' % check)
            else:
                if check is None:
                    ret.line('#else')
                else:
                    ret.line('#elif defined(%s)' % check)
            ret.line('/* --- %s --- */' % arch.get('name', '???'))
            sn = ret.snippet_absolute(abs_fn)
            if not sn.provides.get('DUK_USE_BYTEORDER', False):
                byteorder_provided_by_all = False
            if not sn.provides.get('DUK_USE_ALIGN_BY', False):
                alignment_provided_by_all = False
            if sn.provides.get('DUK_USE_PACKED_TVAL', False):
                ret.line('#define DUK_F_PACKED_TVAL_PROVIDED')  # signal to fillin
            else:
                packedtval_provided_by_all = False
        ret.line('#endif  /* autodetect architecture */')

    ret.empty()

    if opts.compiler is not None:
        ret.chdr_block_heading('Compiler: ' + opts.compiler)

        # XXX: better to lookup compilers metadata
        include = 'compiler_%s.h.in' % opts.compiler
        abs_fn = os.path.join(meta_dir, 'compilers', include)
        validate_compiler_file(abs_fn)
        sn = ret.snippet_absolute(abs_fn)
    else:
        ret.chdr_block_heading('Compiler autodetection')

        for idx, comp in enumerate(compilers['autodetect']):
            check = comp.get('check', None)
            include = comp['include']
            abs_fn = os.path.join(meta_dir, 'compilers', include)

            validate_compiler_file(abs_fn)

            if idx == 0:
                ret.line('#if defined(%s)' % check)
            else:
                if check is None:
                    ret.line('#else')
                else:
                    ret.line('#elif defined(%s)' % check)
            ret.line('/* --- %s --- */' % comp.get('name', '???'))
            sn = ret.snippet_absolute(abs_fn)
        ret.line('#endif  /* autodetect compiler */')

    ret.empty()

    # DUK_F_UCLIBC is special because __UCLIBC__ is provided by an #include
    # file, so the check must happen after platform includes.  It'd be nice
    # for this to be automatic (e.g. DUK_F_UCLIBC.h.in could indicate the
    # dependency somehow).

    ret.snippet_absolute(os.path.join(meta_dir, 'helper-snippets', 'DUK_F_UCLIBC.h.in'))
    ret.empty()

    # XXX: platform/compiler could provide types; if so, need some signaling
    # defines like DUK_F_TYPEDEFS_DEFINED

    # Number types
    if opts.c99_types_only:
        ret.snippet_relative('types1.h.in')
        ret.line('/* C99 types assumed */')
        ret.snippet_relative('types_c99.h.in')
        ret.empty()
    else:
        ret.snippet_relative('types1.h.in')
        ret.line('#if defined(DUK_F_HAVE_INTTYPES)')
        ret.line('/* C99 or compatible */')
        ret.empty()
        ret.snippet_relative('types_c99.h.in')
        ret.empty()
        ret.line('#else  /* C99 types */')
        ret.empty()
        ret.snippet_relative('types_legacy.h.in')
        ret.empty()
        ret.line('#endif  /* C99 types */')
        ret.empty()
    ret.snippet_relative('types2.h.in')
    ret.empty()
    ret.snippet_relative('64bitops.h.in')
    ret.empty()

    # Platform, architecture, compiler fillins.  These are after all
    # detection so that e.g. DUK_SPRINTF() can be provided by platform
    # or compiler before trying a fill-in.

    ret.chdr_block_heading('Fill-ins for platform, architecture, and compiler')

    ret.snippet_relative('platform_fillins.h.in')
    ret.empty()
    ret.snippet_relative('architecture_fillins.h.in')
    if not byteorder_provided_by_all:
        ret.empty()
        ret.snippet_relative('byteorder_fillin.h.in')
    if not alignment_provided_by_all:
        ret.empty()
        ret.snippet_relative('alignment_fillin.h.in')
    ret.empty()
    ret.snippet_relative('compiler_fillins.h.in')
    ret.empty()
    ret.snippet_relative('inline_workaround.h.in')
    ret.empty()
    if not packedtval_provided_by_all:
        ret.empty()
        ret.snippet_relative('packed_tval_fillin.h.in')

    # Object layout
    ret.snippet_relative('object_layout.h.in')
    ret.empty()

    # Detect and reject 'fast math'
    ret.snippet_relative('reject_fast_math.h.in')
    ret.empty()

    # Emit forced options.  If a corresponding option is already defined
    # by a snippet above, #undef it first.

    tmp = Snippet(ret.join().split('\n'))
    first_forced = True
    for doc in get_use_defs(removed=not opts.omit_removed_config_options,
                            deprecated=not opts.omit_deprecated_config_options,
                            unused=not opts.omit_unused_config_options):
        defname = doc['define']

        if not forced_opts.has_key(defname):
            continue

        if not doc.has_key('default'):
            raise Exception('config option %s is missing default value' % defname)

        if first_forced:
            ret.chdr_block_heading('Forced options')
            first_forced = False

        undef_done = False
        if tmp.provides.has_key(defname):
            ret.line('#undef ' + defname)
            undef_done = True

        emit_default_from_config_meta(ret, doc, forced_opts, undef_done, active_opts)

    ret.empty()

    # If manually-edited snippets don't #define or #undef a certain
    # config option, emit a default value here.  This is useful to
    # fill-in for new config options not covered by manual snippets
    # (which is intentional).

    tmp = Snippet(ret.join().split('\n'))
    need = {}
    for doc in get_use_defs(removed=False):
        need[doc['define']] = True
    for k in tmp.provides.keys():
        if need.has_key(k):
            del need[k]
    need_keys = sorted(need.keys())

    if len(need_keys) > 0:
        ret.chdr_block_heading('Autogenerated defaults')

        for k in need_keys:
            logger.debug('config option %s not covered by manual snippets, emitting default automatically' % k)
            emit_default_from_config_meta(ret, use_defs[k], {}, False, active_opts)

        ret.empty()

    if len(opts.fixup_header_lines) > 0:
        ret.chdr_block_heading('Fixups')
        for line in opts.fixup_header_lines:
            ret.line(line)
        ret.empty()

    add_override_defines_section(opts, ret)

    # Some headers are only included if final DUK_USE_xxx option settings
    # indicate they're needed, for example C++ <exception>.
    add_conditional_includes_section(opts, ret)

    # Date provider snippet is after custom header and overrides, so that
    # the user may define e.g. DUK_USE_DATE_NOW_GETTIMEOFDAY in their
    # custom header.
    ret.snippet_relative('date_provider.h.in')
    ret.empty()

    ret.fill_dependencies_for_snippets(idx_deps)

    if opts.emit_legacy_feature_check:
        add_legacy_feature_option_checks(opts, ret)
    if opts.emit_config_sanity_check:
        add_config_option_checks(opts, ret)
    if opts.add_active_defines_macro:
        add_duk_active_defines_macro(ret)

    # Derived defines (DUK_USE_INTEGER_LE, etc) from DUK_USE_BYTEORDER.
    # Duktape internals currently rely on the derived defines.  This is
    # after sanity checks because the derived defines are marked removed.
    ret.snippet_relative('byteorder_derived.h.in')
    ret.empty()

    ret.line('#endif  /* DUK_CONFIG_H_INCLUDED */')
    ret.empty()  # for trailing newline
    return remove_duplicate_newlines(ret.join()), active_opts

#
#  Misc
#

# Validate DUK_USE_xxx config options found in source code against known
# config metadata.  Also warn about non-removed config options that are
# not found in the source.
def validate_config_options_in_source(fn):
    with open(fn, 'rb') as f:
        doc = json.loads(f.read())

    defs_used = {}

    for opt in doc.get('used_duk_use_options'):
        defs_used[opt] = True
        if opt == 'DUK_USE_xxx' or opt == 'DUK_USE_XXX':
            continue  # allow common placeholders
        meta = use_defs.get(opt)
        if meta is None:
            raise Exception('unknown config option in source code: %r' % opt)
        if meta.get('removed', None) is not None:
            #logger.info('removed config option in source code: %r' % opt)
            #raise Exception('removed config option in source code: %r' % opt)
            pass

    for meta in use_defs_list:
        if not defs_used.has_key(meta['define']):
            if not meta.has_key('removed'):
                logger.debug('config option %r not found in source code' % meta['define'])

#
#  Main
#

def add_genconfig_optparse_options(parser, direct=False):
    # Forced options from multiple sources are gathered into a shared list
    # so that the override order remains the same as on the command line.
    force_options_yaml = []
    def add_force_option_yaml(option, opt, value, parser):
        # XXX: check that YAML parses
        force_options_yaml.append(value)
    def add_force_option_file(option, opt, value, parser):
        # XXX: check that YAML parses
        with open(value, 'rb') as f:
            force_options_yaml.append(f.read())
    def add_force_option_define(option, opt, value, parser):
        defname, eq, defval = value.partition('=')
        if not eq:
            doc = { defname: True }
        else:
            defname, paren, defargs = defname.partition('(')
            if not paren:
                doc = { defname: defval }
            else:
                doc = { defname: { 'verbatim': '#define %s%s%s %s' % (defname, paren, defargs, defval) } }
        force_options_yaml.append(yaml.safe_dump(doc))
    def add_force_option_undefine(option, opt, value, parser):
        tmp = value.split('=')
        if len(tmp) == 1:
            doc = { tmp[0]: False }
        else:
            raise Exception('invalid option value: %r' % value)
        force_options_yaml.append(yaml.safe_dump(doc))

    fixup_header_lines = []
    def add_fixup_header_line(option, opt, value, parser):
        fixup_header_lines.append(value)
    def add_fixup_header_file(option, opt, value, parser):
        with open(value, 'rb') as f:
            for line in f:
                if line[-1] == '\n':
                    line = line[:-1]
                fixup_header_lines.append(line)

    if direct:
        parser.add_option('--metadata', dest='config_metadata', default=None, help='metadata directory')
        parser.add_option('--output', dest='output', default=None, help='output filename for C header or RST documentation file')
        parser.add_option('--output-active-options', dest='output_active_options', default=None, help='output JSON file with active config options information')
    else:
        # Different option name when called through configure.py,
        # also no --output option.
        parser.add_option('--config-metadata', dest='config_metadata', default=None, help='metadata directory (defaulted based on configure.py script path)')

    parser.add_option('--platform', dest='platform', default=None, help='platform (default is autodetect)')
    parser.add_option('--compiler', dest='compiler', default=None, help='compiler (default is autodetect)')
    parser.add_option('--architecture', dest='architecture', default=None, help='architecture (default is autodetec)')
    parser.add_option('--c99-types-only', dest='c99_types_only', action='store_true', default=False, help='assume C99 types, no legacy type detection')
    parser.add_option('--dll', dest='dll', action='store_true', default=False, help='dll build of Duktape, affects symbol visibility macros especially on Windows')
    parser.add_option('--support-feature-options', dest='support_feature_options', action='store_true', default=False, help=optparse.SUPPRESS_HELP)
    parser.add_option('--emit-legacy-feature-check', dest='emit_legacy_feature_check', action='store_true', default=False, help='emit preprocessor checks to reject legacy feature options (DUK_OPT_xxx)')
    parser.add_option('--emit-config-sanity-check', dest='emit_config_sanity_check', action='store_true', default=False, help='emit preprocessor checks for config option consistency (DUK_USE_xxx)')
    parser.add_option('--omit-removed-config-options', dest='omit_removed_config_options', action='store_true', default=False, help='omit removed config options from generated headers')
    parser.add_option('--omit-deprecated-config-options', dest='omit_deprecated_config_options', action='store_true', default=False, help='omit deprecated config options from generated headers')
    parser.add_option('--omit-unused-config-options', dest='omit_unused_config_options', action='store_true', default=False, help='omit unused config options from generated headers')
    parser.add_option('--add-active-defines-macro', dest='add_active_defines_macro', action='store_true', default=False, help='add DUK_ACTIVE_DEFINES macro, for development only')
    parser.add_option('--define', type='string', metavar='OPTION', dest='force_options_yaml', action='callback', callback=add_force_option_define, default=force_options_yaml, help='force #define option using a C compiler like syntax, e.g. "--define DUK_USE_DEEP_C_STACK" or "--define DUK_USE_TRACEBACK_DEPTH=10"')
    parser.add_option('-D', type='string', metavar='OPTION', dest='force_options_yaml', action='callback', callback=add_force_option_define, default=force_options_yaml, help='synonym for --define, e.g. "-DDUK_USE_DEEP_C_STACK" or "-DDUK_USE_TRACEBACK_DEPTH=10"')
    parser.add_option('--undefine', type='string', metavar='OPTION', dest='force_options_yaml', action='callback', callback=add_force_option_undefine, default=force_options_yaml, help='force #undef option using a C compiler like syntax, e.g. "--undefine DUK_USE_DEEP_C_STACK"')
    parser.add_option('-U', type='string', metavar='OPTION', dest='force_options_yaml', action='callback', callback=add_force_option_undefine, default=force_options_yaml, help='synonym for --undefine, e.g. "-UDUK_USE_DEEP_C_STACK"')
    parser.add_option('--option-yaml', type='string', metavar='YAML', dest='force_options_yaml', action='callback', callback=add_force_option_yaml, default=force_options_yaml, help='force option(s) using inline YAML (e.g. --option-yaml "DUK_USE_DEEP_C_STACK: true")')
    parser.add_option('--option-file', type='string', metavar='FILENAME', dest='force_options_yaml', action='callback', callback=add_force_option_file, default=force_options_yaml, help='YAML file(s) providing config option overrides')
    parser.add_option('--fixup-file', type='string', metavar='FILENAME', dest='fixup_header_lines', action='callback', callback=add_fixup_header_file, default=fixup_header_lines, help='C header snippet file(s) to be appended to generated header, useful for manual option fixups')
    parser.add_option('--fixup-line', type='string', metavar='LINE', dest='fixup_header_lines', action='callback', callback=add_fixup_header_line, default=fixup_header_lines, help='C header fixup line to be appended to generated header (e.g. --fixup-line "#define DUK_USE_FASTINT")')
    parser.add_option('--sanity-warning', dest='sanity_strict', action='store_false', default=True, help='emit a warning instead of #error for option sanity check issues')
    parser.add_option('--use-cpp-warning', dest='use_cpp_warning', action='store_true', default=False, help='emit a (non-portable) #warning when appropriate')

    if direct:
        parser.add_option('--used-stridx-metadata', dest='used_stridx_metadata', default=None, help='metadata for used stridx, bidx, DUK_USE_xxx')
        parser.add_option('--git-commit', dest='git_commit', default=None, help='git commit hash to be included in header comments')
        parser.add_option('--git-describe', dest='git_describe', default=None, help='git describe string to be included in header comments')
        parser.add_option('--git-branch', dest='git_branch', default=None, help='git branch string to be included in header comments')
        parser.add_option('--quiet', dest='quiet', action='store_true', default=False, help='Suppress info messages (show warnings)')
        parser.add_option('--verbose', dest='verbose', action='store_true', default=False, help='Show verbose debug messages')

def parse_options():
    commands = [
        'duk-config-header',
        'config-documentation'
    ]

    parser = optparse.OptionParser(
        usage='Usage: %prog [options] COMMAND',
        description='Generate a duk_config.h or config option documentation based on config metadata.',
        epilog='COMMAND can be one of: ' + ', '.join(commands) + '.'
    )

    add_genconfig_optparse_options(parser, direct=True)
    (opts, args) = parser.parse_args()

    return opts, args

def genconfig(opts, args):
    # Log level.
    if opts.quiet:
        logger.setLevel(logging.WARNING)
    elif opts.verbose:
        logger.setLevel(logging.DEBUG)

    if opts.support_feature_options:
        raise Exception('--support-feature-options and support for DUK_OPT_xxx feature options are obsolete, use DUK_USE_xxx config options instead')

    meta_dir = opts.config_metadata
    if opts.config_metadata is None:
        if os.path.isdir(os.path.join('.', 'config-options')):
            opts.config_metadata = '.'
    if opts.config_metadata is not None and os.path.isdir(opts.config_metadata):
        meta_dir = opts.config_metadata
        metadata_src_text = 'Using metadata directory: %r' % meta_dir
    else:
        raise Exception('metadata argument must be a directory (tar.gz no longer supported)')

    scan_helper_snippets(os.path.join(meta_dir, 'helper-snippets'))
    scan_use_defs(os.path.join(meta_dir, 'config-options'))
    scan_opt_defs(os.path.join(meta_dir, 'feature-options'))
    scan_use_tags()
    scan_tags_meta(os.path.join(meta_dir, 'tags.yaml'))
    logger.debug('%s, scanned%d DUK_USE_XXX, %d helper snippets' % \
        (metadata_src_text, len(use_defs.keys()), len(helper_snippets)))
    logger.debug('Tags: %r' % use_tags_list)

    if opts.used_stridx_metadata is not None:
        validate_config_options_in_source(opts.used_stridx_metadata)

    if len(args) == 0:
        raise Exception('missing command')
    cmd = args[0]

    if cmd == 'duk-config-header':
        # Generate a duk_config.h header with platform, compiler, and
        # architecture either autodetected (default) or specified by
        # user.
        desc = [
            'platform=' + ('any', opts.platform)[opts.platform is not None],
            'architecture=' + ('any', opts.architecture)[opts.architecture is not None],
            'compiler=' + ('any', opts.compiler)[opts.compiler is not None]
        ]
        if opts.dll:
            desc.append('dll mode')
        logger.info('Creating duk_config.h: ' + ', '.join(desc))
        result, active_opts = generate_duk_config_header(opts, meta_dir)
        with open(opts.output, 'wb') as f:
            f.write(result)
        logger.debug('Wrote duk_config.h to ' + str(opts.output))
        if opts.output_active_options is not None:
            with open(opts.output_active_options, 'wb') as f:
                f.write(json.dumps(active_opts, indent=4))
            logger.debug('Wrote active options JSON metadata to ' + str(opts.output_active_options))
    elif cmd == 'feature-documentation':
        raise Exception('The feature-documentation command has been removed along with DUK_OPT_xxx feature option support')
    elif cmd == 'config-documentation':
        logger.info('Creating config option documentation')
        result = generate_config_option_documentation(opts)
        with open(opts.output, 'wb') as f:
            f.write(result)
        logger.debug('Wrote config option documentation to ' + str(opts.output))
    else:
        raise Exception('invalid command: %r' % cmd)

def main():
    opts, args = parse_options()
    genconfig(opts, args)

if __name__ == '__main__':
    main()
