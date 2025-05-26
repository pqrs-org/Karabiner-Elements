#!/usr/bin/env python2
#
#  Prepare a duk_config.h and combined/separate sources for compilation,
#  given user supplied config options, built-in metadata, Unicode tables, etc.
#
#  This is intended to be the main tool application build scripts would use
#  before their build step, so convenient, versions, Python compatibility,
#  etc all matter.
#
#  When obsoleting options, leave the option definitions behind (with
#  help=optparse.SUPPRESS_HELP) and give useful suggestions when obsolete
#  options are used.  This makes it easier for users to fix their build
#  scripts.
#

import logging
import sys
logging.basicConfig(level=logging.INFO, stream=sys.stdout, format='%(name)-21s %(levelname)-7s %(message)s')
logger = logging.getLogger('configure.py')
logger.setLevel(logging.INFO)

import os
import re
import shutil
import glob
import optparse
import tarfile
import json
import tempfile
import subprocess
import atexit

def import_warning(module, aptPackage, pipPackage):
    sys.stderr.write('\n')
    sys.stderr.write('*** NOTE: Could not "import %s".  Install it using e.g.:\n' % module)
    sys.stderr.write('\n')
    sys.stderr.write('    # Linux\n')
    sys.stderr.write('    $ sudo apt-get install %s\n' % aptPackage)
    sys.stderr.write('\n')
    sys.stderr.write('    # Windows\n')
    sys.stderr.write('    > pip install %s\n' % pipPackage)

try:
    import yaml
except ImportError:
    import_warning('yaml', 'python-yaml', 'PyYAML')
    sys.exit(1)

import genconfig

# Helpers

def exec_get_stdout(cmd, input=None, default=None, print_stdout=False):
    try:
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        ret = proc.communicate(input=input)
        if print_stdout:
            sys.stdout.write(ret[0])
            sys.stdout.flush()
        if proc.returncode != 0:
            sys.stdout.write(ret[1])  # print stderr on error
            sys.stdout.flush()
            if default is not None:
                logger.info('WARNING: command %r failed, return default' % cmd)
                return default
            raise Exception('command failed, return code %d: %r' % (proc.returncode, cmd))
        return ret[0]
    except:
        if default is not None:
            logger.info('WARNING: command %r failed, return default' % cmd)
            return default
        raise

def exec_print_stdout(cmd, input=None):
    ret = exec_get_stdout(cmd, input=input, print_stdout=True)

def mkdir(path):
    os.mkdir(path)

def copy_file(src, dst):
    with open(src, 'rb') as f_in:
        with open(dst, 'wb') as f_out:
            f_out.write(f_in.read())

def copy_files(filelist, srcdir, dstdir):
    for i in filelist:
        copy_file(os.path.join(srcdir, i), os.path.join(dstdir, i))

def copy_and_replace(src, dst, rules):
    # Read and write separately to allow in-place replacement
    keys = sorted(rules.keys())
    res = []
    with open(src, 'rb') as f_in:
        for line in f_in:
            for k in keys:
                line = line.replace(k, rules[k])
            res.append(line)
    with open(dst, 'wb') as f_out:
        f_out.write(''.join(res))

def copy_and_cquote(src, dst):
    with open(src, 'rb') as f_in:
        with open(dst, 'wb') as f_out:
            f_out.write('/*\n')
            for line in f_in:
                line = line.decode('utf-8')
                f_out.write(' *  ')
                for c in line:
                    if (ord(c) >= 0x20 and ord(c) <= 0x7e) or (c in '\x0a'):
                        f_out.write(c.encode('ascii'))
                    else:
                        f_out.write('\\u%04x' % ord(c))
            f_out.write(' */\n')

def read_file(src, strip_last_nl=False):
    with open(src, 'rb') as f:
        data = f.read()
        if len(data) > 0 and data[-1] == '\n':
            data = data[:-1]
        return data

def delete_matching_files(dirpath, cb):
    for fn in os.listdir(dirpath):
        if os.path.isfile(os.path.join(dirpath, fn)) and cb(fn):
            logger.debug('Deleting %r' % os.path.join(dirpath, fn))
            os.unlink(os.path.join(dirpath, fn))

def create_targz(dstfile, filelist):
    # https://docs.python.org/2/library/tarfile.html#examples

    def _add(tf, fn):  # recursive add
        logger.debug('Adding to tar: ' + fn)
        if os.path.isdir(fn):
            for i in sorted(os.listdir(fn)):
                _add(tf, os.path.join(fn, i))
        elif os.path.isfile(fn):
            tf.add(fn)
        else:
            raise Exception('invalid file: %r' % fn)

    with tarfile.open(dstfile, 'w:gz') as tf:
        for fn in filelist:
            _add(tf, fn)

def cstring(x):
    return '"' + x + '"'  # good enough for now

# DUK_VERSION is grepped from duktape.h.in: it is needed for the
# public API and we want to avoid defining it in two places.
def get_duk_version(apiheader_filename):
    r = re.compile(r'^#define\s+DUK_VERSION\s+(.*?)L?\s*$')
    with open(apiheader_filename, 'rb') as f:
        for line in f:
            m = r.match(line)
            if m is not None:
                duk_version = int(m.group(1))
                duk_major = duk_version / 10000
                duk_minor = (duk_version % 10000) / 100
                duk_patch = duk_version % 100
                duk_version_formatted = '%d.%d.%d' % (duk_major, duk_minor, duk_patch)
                return duk_version, duk_major, duk_minor, duk_patch, duk_version_formatted

    raise Exception('cannot figure out duktape version')

# Option parsing

def main():
    parser = optparse.OptionParser(
        usage='Usage: %prog [options]',
        description='Prepare Duktape source files and a duk_config.h configuration header for compilation. ' + \
                    'Source files can be combined (amalgamated) or kept separate. ' + \
                    'See http://wiki.duktape.org/Configuring.html for examples.'
    )

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
        tmp = value.split('=')
        if len(tmp) == 1:
            doc = { tmp[0]: True }
        elif len(tmp) == 2:
            doc = { tmp[0]: tmp[1] }
        else:
            raise Exception('invalid option value: %r' % value)
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

    # Options for configure.py tool itself.
    parser.add_option('--source-directory', dest='source_directory', default=None, help='Directory with raw input sources (defaulted based on configure.py script path)')
    parser.add_option('--output-directory', dest='output_directory', default=None, help='Directory for output files (created automatically if it doesn\'t exist, reused if safe)')
    parser.add_option('--license-file', dest='license_file', default=None, help='Source for LICENSE.txt (defaulted based on configure.py script path)')
    parser.add_option('--authors-file', dest='authors_file', default=None, help='Source for AUTHORS.rst (defaulted based on configure.py script path)')
    parser.add_option('--git-commit', dest='git_commit', default=None, help='Force git commit hash')
    parser.add_option('--git-describe', dest='git_describe', default=None, help='Force git describe')
    parser.add_option('--git-branch', dest='git_branch', default=None, help='Force git branch name')
    parser.add_option('--duk-dist-meta', dest='duk_dist_meta', default=None, help='duk_dist_meta.json to read git commit etc info from')

    # Options for combining sources.
    parser.add_option('--separate-sources', dest='separate_sources', action='store_true', default=False, help='Output separate sources instead of combined source (default is combined)')
    parser.add_option('--line-directives', dest='line_directives', action='store_true', default=False, help='Output #line directives in combined source (default is false)')

    # Options forwarded to genbuiltins.py.
    parser.add_option('--rom-support', dest='rom_support', action='store_true', help='Add support for ROM strings/objects (increases duktape.c size considerably)')
    parser.add_option('--rom-auto-lightfunc', dest='rom_auto_lightfunc', action='store_true', default=False, help='Convert ROM built-in function properties into lightfuncs automatically whenever possible')
    parser.add_option('--user-builtin-metadata', dest='obsolete_builtin_metadata', default=None, help=optparse.SUPPRESS_HELP)
    parser.add_option('--builtin-file', dest='builtin_files', metavar='FILENAME', action='append', default=[], help='Built-in string/object YAML metadata to be applied over default built-ins (multiple files may be given, applied in sequence)')

    # Options for Unicode.
    parser.add_option('--unicode-data', dest='unicode_data', default=None, help='Provide custom UnicodeData.txt')
    parser.add_option('--special-casing', dest='special_casing', default=None, help='Provide custom SpecialCasing.txt')

    # Options forwarded to genconfig.py.
    genconfig.add_genconfig_optparse_options(parser)

    # Log level options.
    parser.add_option('--quiet', dest='quiet', action='store_true', default=False, help='Suppress info messages (show warnings)')
    parser.add_option('--verbose', dest='verbose', action='store_true', default=False, help='Show verbose debug messages')

    (opts, args) = parser.parse_args()
    if len(args) > 0:
        raise Exception('unexpected arguments: %r' % args)

    if opts.obsolete_builtin_metadata is not None:
        raise Exception('--user-builtin-metadata has been removed, use --builtin-file instead')

    # Log level.
    forward_loglevel = []
    if opts.quiet:
        logger.setLevel(logging.WARNING)
        forward_loglevel = [ '--quiet' ]
    elif opts.verbose:
        logger.setLevel(logging.DEBUG)
        forward_loglevel = [ '--verbose' ]

    # Figure out directories, git info, etc

    entry_cwd = os.getcwd()
    script_path = sys.path[0]  # http://stackoverflow.com/questions/4934806/how-can-i-find-scripts-directory-with-python

    def default_from_script_path(optname, orig, alternatives):
        if orig is not None:
            orig = os.path.abspath(orig)
            if os.path.exists(orig):
                logger.debug(optname + ' ' + orig)
                return orig
            else:
                raise Exception('invalid argument to ' + optname)
        for alt in alternatives:
            cand = os.path.abspath(os.path.join(script_path, '..', alt))
            if os.path.exists(cand):
                logger.debug('default ' + optname + ' to ' + cand)
                return cand
        raise Exception('no ' + optname + ' and cannot default based on script path')

    if opts.output_directory is None:
        raise Exception('missing --output-directory')
    opts.output_directory = os.path.abspath(opts.output_directory)
    outdir = opts.output_directory

    opts.source_directory = default_from_script_path('--source-directory', opts.source_directory, [ 'src-input' ])
    srcdir = opts.source_directory

    opts.config_metadata = default_from_script_path('--config-metadata', opts.config_metadata, [ 'config' ])

    opts.license_file = default_from_script_path('--license-file', opts.license_file, [ 'LICENSE.txt' ])
    license_file = opts.license_file

    opts.authors_file = default_from_script_path('--authors-file', opts.authors_file, [ 'AUTHORS.rst' ])
    authors_file = opts.authors_file

    duk_dist_meta = None
    if opts.duk_dist_meta is not None:
        with open(opts.duk_dist_meta, 'rb') as f:
            duk_dist_meta = json.loads(f.read())

    duk_version, duk_major, duk_minor, duk_patch, duk_version_formatted = \
        get_duk_version(os.path.join(srcdir, 'duktape.h.in'))

    git_commit = None
    git_branch = None
    git_describe = None

    if duk_dist_meta is not None:
        git_commit = duk_dist_meta['git_commit']
        git_branch = duk_dist_meta['git_branch']
        git_describe = duk_dist_meta['git_describe']

    if opts.git_commit is not None:
        git_commit = opts.git_commit
    if opts.git_describe is not None:
        git_describe = opts.git_describe
    if opts.git_branch is not None:
        git_branch = opts.git_branch

    if git_commit is None:
        logger.debug('Git commit not specified, autodetect from current directory')
        git_commit = exec_get_stdout([ 'git', 'rev-parse', 'HEAD' ], default='external').strip()
    if git_describe is None:
        logger.debug('Git describe not specified, autodetect from current directory')
        git_describe = exec_get_stdout([ 'git', 'describe', '--always', '--dirty' ], default='external').strip()
    if git_branch is None:
        logger.debug('Git branch not specified, autodetect from current directory')
        git_branch = exec_get_stdout([ 'git', 'rev-parse', '--abbrev-ref', 'HEAD' ], default='external').strip()

    git_commit = str(git_commit)
    git_describe = str(git_describe)
    git_branch = str(git_branch)

    git_commit_cstring = cstring(git_commit)
    git_describe_cstring = cstring(git_describe)
    git_branch_cstring = cstring(git_branch)

    if opts.unicode_data is None:
        unicode_data = os.path.join(srcdir, 'UnicodeData.txt')
    else:
        unicode_data = opts.unicode_data
    if opts.special_casing is None:
        special_casing = os.path.join(srcdir, 'SpecialCasing.txt')
    else:
        special_casing = opts.special_casing

    logger.info('Configuring Duktape version %s, commit %s, describe %s, branch %s' % \
                (duk_version_formatted, git_commit, git_describe, git_branch))
    logger.info('  - source input directory: ' + opts.source_directory)
    logger.info('  - license file: ' + opts.license_file)
    logger.info('  - authors file: ' + opts.authors_file)
    logger.info('  - config metadata directory: ' + opts.config_metadata)
    logger.info('  - output directory: ' + opts.output_directory)

    # Create output directory.  If the directory already exists, reuse it but
    # only when it's safe to do so, i.e. it contains only known output files.
    allow_outdir_reuse = True
    outdir_whitelist = [ 'duk_config.h', 'duktape.c', 'duktape.h', 'duk_source_meta.json' ]
    if os.path.exists(outdir):
        if not allow_outdir_reuse:
            raise Exception('configure target directory %s already exists, please delete it first' % repr(outdir))
        for fn in os.listdir(outdir):
            if fn == '.' or fn == '..' or (fn in outdir_whitelist and os.path.isfile(os.path.join(outdir, fn))):
                continue
            else:
                raise Exception('configure target directory %s already exists, cannot reuse because it contains unknown files such as %s' % (repr(outdir), repr(fn)))
        logger.info('Reusing output directory (already exists but contains only safe, known files)')
        for fn in outdir_whitelist:
            if os.path.isfile(os.path.join(outdir, fn)):
                os.unlink(os.path.join(outdir, fn))
    else:
        logger.debug('Output directory doesn\'t exist, create it')
        os.mkdir(outdir)

    # Temporary directory.
    tempdir = tempfile.mkdtemp(prefix='tmp-duk-prepare-')
    atexit.register(shutil.rmtree, tempdir)
    mkdir(os.path.join(tempdir, 'src'))
    logger.debug('Using temporary directory %r' % tempdir)

    # Separate sources are mostly copied as is at present.
    copy_files([
        'duk_alloc_default.c',
        'duk_api_internal.h',
        'duk_api_stack.c',
        'duk_api_heap.c',
        'duk_api_buffer.c',
        'duk_api_call.c',
        'duk_api_codec.c',
        'duk_api_compile.c',
        'duk_api_bytecode.c',
        'duk_api_inspect.c',
        'duk_api_memory.c',
        'duk_api_object.c',
        'duk_api_random.c',
        'duk_api_string.c',
        'duk_api_time.c',
        'duk_api_debug.c',
        'duk_bi_array.c',
        'duk_bi_boolean.c',
        'duk_bi_buffer.c',
        'duk_bi_cbor.c',
        'duk_bi_date.c',
        'duk_bi_date_unix.c',
        'duk_bi_date_windows.c',
        'duk_bi_duktape.c',
        'duk_bi_encoding.c',
        'duk_bi_error.c',
        'duk_bi_function.c',
        'duk_bi_global.c',
        'duk_bi_json.c',
        'duk_bi_math.c',
        'duk_bi_number.c',
        'duk_bi_object.c',
        'duk_bi_performance.c',
        'duk_bi_pointer.c',
        'duk_bi_protos.h',
        'duk_bi_reflect.c',
        'duk_bi_regexp.c',
        'duk_bi_string.c',
        'duk_bi_promise.c',
        'duk_bi_proxy.c',
        'duk_bi_symbol.c',
        'duk_bi_thread.c',
        'duk_bi_thrower.c',
        'duk_dblunion.h',
        'duk_debug_fixedbuffer.c',
        'duk_debug.h',
        'duk_debug_macros.c',
        'duk_debug_vsnprintf.c',
        'duk_debugger.c',
        'duk_debugger.h',
        'duk_error_augment.c',
        'duk_error.h',
        'duk_error_longjmp.c',
        'duk_error_macros.c',
        'duk_error_misc.c',
        'duk_error_throw.c',
        'duk_fltunion.h',
        'duk_forwdecl.h',
        'duk_harray.h',
        'duk_hboundfunc.h',
        'duk_hbuffer_alloc.c',
        'duk_hbuffer_assert.c',
        'duk_hbuffer.h',
        'duk_hbuffer_ops.c',
        'duk_hbufobj.h',
        'duk_hbufobj_misc.c',
        'duk_hcompfunc.h',
        'duk_heap_alloc.c',
        'duk_heap.h',
        'duk_heap_hashstring.c',
        'duk_heaphdr.h',
        'duk_heaphdr_assert.c',
        'duk_heap_finalize.c',
        'duk_heap_markandsweep.c',
        'duk_heap_memory.c',
        'duk_heap_misc.c',
        'duk_heap_refcount.c',
        'duk_heap_stringcache.c',
        'duk_heap_stringtable.c',
        'duk_hnatfunc.h',
        'duk_hobject_alloc.c',
        'duk_hobject_assert.c',
        'duk_hobject_class.c',
        'duk_hobject_enum.c',
        'duk_hobject.h',
        'duk_hobject_misc.c',
        'duk_hobject_pc2line.c',
        'duk_hobject_props.c',
        'duk_hproxy.h',
        'duk_hstring.h',
        'duk_hstring_assert.c',
        'duk_hstring_misc.c',
        'duk_hthread_alloc.c',
        'duk_hthread_builtins.c',
        'duk_hthread.h',
        'duk_hthread_misc.c',
        'duk_hthread_stacks.c',
        'duk_henv.h',
        'duk_internal.h',
        'duk_jmpbuf.h',
        'duk_exception.h',
        'duk_js_arith.c',
        'duk_js_bytecode.h',
        'duk_js_call.c',
        'duk_js_compiler.c',
        'duk_js_compiler.h',
        'duk_js_executor.c',
        'duk_js.h',
        'duk_json.h',
        'duk_js_ops.c',
        'duk_js_var.c',
        'duk_lexer.c',
        'duk_lexer.h',
        'duk_numconv.c',
        'duk_numconv.h',
        'duk_refcount.h',
        'duk_regexp_compiler.c',
        'duk_regexp_executor.c',
        'duk_regexp.h',
        'duk_tval.c',
        'duk_tval.h',
        'duk_unicode.h',
        'duk_unicode_support.c',
        'duk_unicode_tables.c',
        'duk_util.h',
        'duk_util_bitdecoder.c',
        'duk_util_bitencoder.c',
        'duk_util_hashbytes.c',
        'duk_util_tinyrandom.c',
        'duk_util_bufwriter.c',
        'duk_util_double.c',
        'duk_util_cast.c',
        'duk_util_memory.c',
        'duk_util_memrw.c',
        'duk_util_misc.c',
        'duk_selftest.c',
        'duk_selftest.h',
        'duk_strings.h',
        'duk_replacements.c',
        'duk_replacements.h'
    ], srcdir, os.path.join(tempdir, 'src'))

    # Build temp versions of LICENSE.txt and AUTHORS.rst for embedding into
    # autogenerated C/H files.

    copy_and_cquote(license_file, os.path.join(tempdir, 'LICENSE.txt.tmp'))
    copy_and_cquote(authors_file, os.path.join(tempdir, 'AUTHORS.rst.tmp'))

    # Scan used stridx, bidx, config options, etc.

    res = exec_get_stdout([
        sys.executable,
        os.path.join(script_path, 'scan_used_stridx_bidx.py')
    ] + glob.glob(os.path.join(srcdir, '*.c')) \
      + glob.glob(os.path.join(srcdir, '*.h')) \
      + glob.glob(os.path.join(srcdir, '*.h.in'))
    )
    with open(os.path.join(tempdir, 'duk_used_stridx_bidx_defs.json.tmp'), 'wb') as f:
        f.write(res)

    # Create a duk_config.h.
    # XXX: might be easier to invoke genconfig directly, but there are a few
    # options which currently conflict (output file, git commit info, etc).
    def forward_genconfig_options():
        res = []
        res += [ '--metadata', os.path.abspath(opts.config_metadata) ]  # rename option, --config-metadata => --metadata
        if opts.platform is not None:
            res += [ '--platform', opts.platform ]
        if opts.compiler is not None:
            res += [ '--compiler', opts.compiler ]
        if opts.architecture is not None:
            res += [ '--architecture', opts.architecture ]
        if opts.c99_types_only:
            res += [ '--c99-types-only' ]
        if opts.dll:
            res += [ '--dll' ]
        if opts.support_feature_options:
            res += [ '--support-feature-options' ]
        if opts.emit_legacy_feature_check:
            res += [ '--emit-legacy-feature-check' ]
        if opts.emit_config_sanity_check:
            res += [ '--emit-config-sanity-check' ]
        if opts.omit_removed_config_options:
            res += [ '--omit-removed-config-options' ]
        if opts.omit_deprecated_config_options:
            res += [ '--omit-deprecated-config-options' ]
        if opts.omit_unused_config_options:
            res += [ '--omit-unused-config-options' ]
        if opts.add_active_defines_macro:
            res += [ '--add-active-defines-macro' ]
        if len(opts.force_options_yaml) > 0:
            # Use temporary files so that large option sets don't create
            # excessively large commands.
            for idx,i in enumerate(opts.force_options_yaml):
                tmpfn = os.path.join(tempdir, 'genconfig%d.yaml' % idx)
                with open(tmpfn, 'wb') as f:
                    f.write(i)
                with open(tmpfn, 'rb') as f:
                    logger.debug(f.read())
                res += [ '--option-file', tmpfn ]
        for i in opts.fixup_header_lines:
            res += [ '--fixup-line', i ]
        if not opts.sanity_strict:
            res += [ '--sanity-warning' ]
        if opts.use_cpp_warning:
            res += [ '--use-cpp-warning' ]
        return res

    cmd = [
        sys.executable, os.path.join(script_path, 'genconfig.py'),
        '--output', os.path.join(tempdir, 'duk_config.h.tmp'),
        '--output-active-options', os.path.join(tempdir, 'duk_config_active_options.json'),
        '--git-commit', git_commit, '--git-describe', git_describe, '--git-branch', git_branch,
        '--used-stridx-metadata', os.path.join(tempdir, 'duk_used_stridx_bidx_defs.json.tmp')
    ]
    cmd += forward_genconfig_options()
    cmd += [
        'duk-config-header'
    ] + forward_loglevel
    logger.debug(repr(cmd))
    exec_print_stdout(cmd)

    copy_file(os.path.join(tempdir, 'duk_config.h.tmp'), os.path.join(outdir, 'duk_config.h'))

    # Build duktape.h from parts, with some git-related replacements.
    # The only difference between single and separate file duktape.h
    # is the internal DUK_SINGLE_FILE define.
    #
    # Newline after 'i \':
    # http://stackoverflow.com/questions/25631989/sed-insert-line-command-osx
    copy_and_replace(os.path.join(srcdir, 'duktape.h.in'), os.path.join(tempdir, 'duktape.h'), {
        '@DUK_SINGLE_FILE@': '#define DUK_SINGLE_FILE',
        '@LICENSE_TXT@': read_file(os.path.join(tempdir, 'LICENSE.txt.tmp'), strip_last_nl=True),
        '@AUTHORS_RST@': read_file(os.path.join(tempdir, 'AUTHORS.rst.tmp'), strip_last_nl=True),
        '@DUK_VERSION_FORMATTED@': duk_version_formatted,
        '@GIT_COMMIT@': git_commit,
        '@GIT_COMMIT_CSTRING@': git_commit_cstring,
        '@GIT_DESCRIBE@': git_describe,
        '@GIT_DESCRIBE_CSTRING@': git_describe_cstring,
        '@GIT_BRANCH@': git_branch,
        '@GIT_BRANCH_CSTRING@': git_branch_cstring
    })

    if opts.separate_sources:
        # keep the line so line numbers match between the two variant headers
        copy_and_replace(os.path.join(tempdir, 'duktape.h'), os.path.join(outdir, 'duktape.h'), {
            '#define DUK_SINGLE_FILE': '#undef DUK_SINGLE_FILE'
        })
    else:
        copy_file(os.path.join(tempdir, 'duktape.h'), os.path.join(outdir, 'duktape.h'))

    # Autogenerated strings and built-in files
    #
    # There are currently no profile specific variants of strings/builtins, but
    # this will probably change when functions are added/removed based on profile.

    cmd = [
        sys.executable,
        os.path.join(script_path, 'genbuiltins.py'),
    ]
    cmd += [
        '--git-commit', git_commit,
        '--git-branch', git_branch,
        '--git-describe', git_describe,
        '--duk-version', str(duk_version)
    ]
    cmd += [
        '--used-stridx-metadata', os.path.join(tempdir, 'duk_used_stridx_bidx_defs.json.tmp'),
        '--strings-metadata', os.path.join(srcdir, 'strings.yaml'),
        '--objects-metadata', os.path.join(srcdir, 'builtins.yaml'),
        '--active-options', os.path.join(tempdir, 'duk_config_active_options.json'),
        '--out-header', os.path.join(tempdir, 'src', 'duk_builtins.h'),
        '--out-source', os.path.join(tempdir, 'src', 'duk_builtins.c'),
        '--out-metadata-json', os.path.join(tempdir, 'genbuiltins_metadata.json')
    ]
    cmd.append('--ram-support')  # enable by default
    if opts.rom_support:
        # ROM string/object support is not enabled by default because
        # it increases the generated duktape.c considerably.
        logger.debug('Enabling --rom-support for genbuiltins.py')
        cmd.append('--rom-support')
    if opts.rom_auto_lightfunc:
        logger.debug('Enabling --rom-auto-lightfunc for genbuiltins.py')
        cmd.append('--rom-auto-lightfunc')
    for fn in opts.builtin_files:
        logger.debug('Forwarding --builtin-file %s' % fn)
        cmd.append('--builtin-file')
        cmd.append(fn)
    cmd += forward_loglevel
    logger.debug(repr(cmd))
    exec_print_stdout(cmd)

    # Autogenerated Unicode files
    #
    # Note: not all of the generated headers are used.  For instance, the
    # match table for "WhiteSpace-Z" is not used, because a custom piece
    # of code handles that particular match.
    #
    # UnicodeData.txt contains ranges expressed like this:
    #
    #   4E00;<CJK Ideograph, First>;Lo;0;L;;;;;N;;;;;
    #   9FCB;<CJK Ideograph, Last>;Lo;0;L;;;;;N;;;;;
    #
    # These are currently decoded into individual characters as a prestep.
    #
    # For IDPART:
    #   UnicodeCombiningMark -> categories Mn, Mc
    #   UnicodeDigit -> categories Nd
    #   UnicodeConnectorPunctuation -> categories Pc

    # Whitespace (unused now)
    WHITESPACE_INCL='Zs'  # USP = Any other Unicode space separator
    WHITESPACE_EXCL='NONE'

    # Unicode letter (unused now)
    LETTER_INCL='Lu,Ll,Lt,Lm,Lo'
    LETTER_EXCL='NONE'
    LETTER_NOA_INCL='Lu,Ll,Lt,Lm,Lo'
    LETTER_NOA_EXCL='ASCII'
    LETTER_NOABMP_INCL=LETTER_NOA_INCL
    LETTER_NOABMP_EXCL='ASCII,NONBMP'

    # Identifier start
    # E5 Section 7.6
    IDSTART_INCL='Lu,Ll,Lt,Lm,Lo,Nl,0024,005F'
    IDSTART_EXCL='NONE'
    IDSTART_NOA_INCL='Lu,Ll,Lt,Lm,Lo,Nl,0024,005F'
    IDSTART_NOA_EXCL='ASCII'
    IDSTART_NOABMP_INCL=IDSTART_NOA_INCL
    IDSTART_NOABMP_EXCL='ASCII,NONBMP'

    # Identifier start - Letter: allows matching of (rarely needed) 'Letter'
    # production space efficiently with the help of IdentifierStart.  The
    # 'Letter' production is only needed in case conversion of Greek final
    # sigma.
    IDSTART_MINUS_LETTER_INCL=IDSTART_NOA_INCL
    IDSTART_MINUS_LETTER_EXCL='Lu,Ll,Lt,Lm,Lo'
    IDSTART_MINUS_LETTER_NOA_INCL=IDSTART_NOA_INCL
    IDSTART_MINUS_LETTER_NOA_EXCL='Lu,Ll,Lt,Lm,Lo,ASCII'
    IDSTART_MINUS_LETTER_NOABMP_INCL=IDSTART_NOA_INCL
    IDSTART_MINUS_LETTER_NOABMP_EXCL='Lu,Ll,Lt,Lm,Lo,ASCII,NONBMP'

    # Identifier start - Identifier part
    # E5 Section 7.6: IdentifierPart, but remove IdentifierStart (already above)
    IDPART_MINUS_IDSTART_INCL='Lu,Ll,Lt,Lm,Lo,Nl,0024,005F,Mn,Mc,Nd,Pc,200C,200D'
    IDPART_MINUS_IDSTART_EXCL='Lu,Ll,Lt,Lm,Lo,Nl,0024,005F'
    IDPART_MINUS_IDSTART_NOA_INCL='Lu,Ll,Lt,Lm,Lo,Nl,0024,005F,Mn,Mc,Nd,Pc,200C,200D'
    IDPART_MINUS_IDSTART_NOA_EXCL='Lu,Ll,Lt,Lm,Lo,Nl,0024,005F,ASCII'
    IDPART_MINUS_IDSTART_NOABMP_INCL=IDPART_MINUS_IDSTART_NOA_INCL
    IDPART_MINUS_IDSTART_NOABMP_EXCL='Lu,Ll,Lt,Lm,Lo,Nl,0024,005F,ASCII,NONBMP'

    logger.debug('Expand UnicodeData.txt ranges')

    exec_print_stdout([
        sys.executable,
        os.path.join(script_path, 'prepare_unicode_data.py'),
        '--unicode-data', unicode_data,
        '--output', os.path.join(tempdir, 'UnicodeData-expanded.tmp')
    ] + forward_loglevel)

    def extract_chars(incl, excl, suffix):
        logger.debug('- extract_chars: %s %s %s' % (incl, excl, suffix))
        res = exec_get_stdout([
            sys.executable,
            os.path.join(script_path, 'extract_chars.py'),
            '--unicode-data', os.path.join(tempdir, 'UnicodeData-expanded.tmp'),
            '--include-categories', incl,
            '--exclude-categories', excl,
            '--out-source', os.path.join(tempdir, 'duk_unicode_%s.c.tmp' % suffix),
            '--out-header', os.path.join(tempdir, 'duk_unicode_%s.h.tmp' % suffix),
            '--table-name', 'duk_unicode_%s' % suffix
        ])
        with open(os.path.join(tempdir, suffix + '.txt'), 'wb') as f:
            f.write(res)

    def extract_caseconv():
        logger.debug('- extract_caseconv case conversion')
        res = exec_get_stdout([
            sys.executable,
            os.path.join(script_path, 'extract_caseconv.py'),
            '--command=caseconv_bitpacked',
            '--unicode-data', os.path.join(tempdir, 'UnicodeData-expanded.tmp'),
            '--special-casing', special_casing,
            '--out-source', os.path.join(tempdir, 'duk_unicode_caseconv.c.tmp'),
            '--out-header', os.path.join(tempdir, 'duk_unicode_caseconv.h.tmp'),
            '--table-name-lc', 'duk_unicode_caseconv_lc',
            '--table-name-uc', 'duk_unicode_caseconv_uc'
        ])
        with open(os.path.join(tempdir, 'caseconv.txt'), 'wb') as f:
            f.write(res)

        logger.debug('- extract_caseconv canon lookup')
        res = exec_get_stdout([
            sys.executable,
            os.path.join(script_path, 'extract_caseconv.py'),
            '--command=re_canon_lookup',
            '--unicode-data', os.path.join(tempdir, 'UnicodeData-expanded.tmp'),
            '--special-casing', special_casing,
            '--out-source', os.path.join(tempdir, 'duk_unicode_re_canon_lookup.c.tmp'),
            '--out-header', os.path.join(tempdir, 'duk_unicode_re_canon_lookup.h.tmp'),
            '--table-name-re-canon-lookup', 'duk_unicode_re_canon_lookup'
        ])
        with open(os.path.join(tempdir, 'caseconv_re_canon_lookup.txt'), 'wb') as f:
            f.write(res)

        logger.debug('- extract_caseconv canon bitmap')
        res = exec_get_stdout([
            sys.executable,
            os.path.join(script_path, 'extract_caseconv.py'),
            '--command=re_canon_bitmap',
            '--unicode-data', os.path.join(tempdir, 'UnicodeData-expanded.tmp'),
            '--special-casing', special_casing,
            '--out-source', os.path.join(tempdir, 'duk_unicode_re_canon_bitmap.c.tmp'),
            '--out-header', os.path.join(tempdir, 'duk_unicode_re_canon_bitmap.h.tmp'),
            '--table-name-re-canon-bitmap', 'duk_unicode_re_canon_bitmap'
        ])
        with open(os.path.join(tempdir, 'caseconv_re_canon_bitmap.txt'), 'wb') as f:
            f.write(res)

    # XXX: Now with configure.py part of the distributable, could generate
    # only those Unicode tables needed by desired configuration (e.g. BMP-only
    # tables if BMP-only was enabled).
    # XXX: Improve Unicode preparation performance; it consumes most of the
    # source preparation time.

    logger.debug('Create Unicode tables for codepoint classes')
    extract_chars(WHITESPACE_INCL, WHITESPACE_EXCL, 'ws')
    extract_chars(LETTER_INCL, LETTER_EXCL, 'let')
    extract_chars(LETTER_NOA_INCL, LETTER_NOA_EXCL, 'let_noa')
    extract_chars(LETTER_NOABMP_INCL, LETTER_NOABMP_EXCL, 'let_noabmp')
    extract_chars(IDSTART_INCL, IDSTART_EXCL, 'ids')
    extract_chars(IDSTART_NOA_INCL, IDSTART_NOA_EXCL, 'ids_noa')
    extract_chars(IDSTART_NOABMP_INCL, IDSTART_NOABMP_EXCL, 'ids_noabmp')
    extract_chars(IDSTART_MINUS_LETTER_INCL, IDSTART_MINUS_LETTER_EXCL, 'ids_m_let')
    extract_chars(IDSTART_MINUS_LETTER_NOA_INCL, IDSTART_MINUS_LETTER_NOA_EXCL, 'ids_m_let_noa')
    extract_chars(IDSTART_MINUS_LETTER_NOABMP_INCL, IDSTART_MINUS_LETTER_NOABMP_EXCL, 'ids_m_let_noabmp')
    extract_chars(IDPART_MINUS_IDSTART_INCL, IDPART_MINUS_IDSTART_EXCL, 'idp_m_ids')
    extract_chars(IDPART_MINUS_IDSTART_NOA_INCL, IDPART_MINUS_IDSTART_NOA_EXCL, 'idp_m_ids_noa')
    extract_chars(IDPART_MINUS_IDSTART_NOABMP_INCL, IDPART_MINUS_IDSTART_NOABMP_EXCL, 'idp_m_ids_noabmp')

    logger.debug('Create Unicode tables for case conversion')
    extract_caseconv()

    logger.debug('Combine sources and clean up')

    # Inject autogenerated files into source and header files so that they are
    # usable (for all profiles and define cases) directly.
    #
    # The injection points use a standard C preprocessor #include syntax
    # (earlier these were actual includes).

    copy_and_replace(os.path.join(tempdir, 'src', 'duk_unicode.h'), os.path.join(tempdir, 'src', 'duk_unicode.h'), {
        '#include "duk_unicode_ids_noa.h"': read_file(os.path.join(tempdir, 'duk_unicode_ids_noa.h.tmp'), strip_last_nl=True),
        '#include "duk_unicode_ids_noabmp.h"': read_file(os.path.join(tempdir, 'duk_unicode_ids_noabmp.h.tmp'), strip_last_nl=True),
        '#include "duk_unicode_ids_m_let_noa.h"': read_file(os.path.join(tempdir, 'duk_unicode_ids_m_let_noa.h.tmp'), strip_last_nl=True),
        '#include "duk_unicode_ids_m_let_noabmp.h"': read_file(os.path.join(tempdir, 'duk_unicode_ids_m_let_noabmp.h.tmp'), strip_last_nl=True),
        '#include "duk_unicode_idp_m_ids_noa.h"': read_file(os.path.join(tempdir, 'duk_unicode_idp_m_ids_noa.h.tmp'), strip_last_nl=True),
        '#include "duk_unicode_idp_m_ids_noabmp.h"': read_file(os.path.join(tempdir, 'duk_unicode_idp_m_ids_noabmp.h.tmp'), strip_last_nl=True),
        '#include "duk_unicode_caseconv.h"': read_file(os.path.join(tempdir, 'duk_unicode_caseconv.h.tmp'), strip_last_nl=True),
        '#include "duk_unicode_re_canon_lookup.h"': read_file(os.path.join(tempdir, 'duk_unicode_re_canon_lookup.h.tmp'), strip_last_nl=True),
        '#include "duk_unicode_re_canon_bitmap.h"': read_file(os.path.join(tempdir, 'duk_unicode_re_canon_bitmap.h.tmp'), strip_last_nl=True)
    })

    copy_and_replace(os.path.join(tempdir, 'src', 'duk_unicode_tables.c'), os.path.join(tempdir, 'src', 'duk_unicode_tables.c'), {
        '#include "duk_unicode_ids_noa.c"': read_file(os.path.join(tempdir, 'duk_unicode_ids_noa.c.tmp'), strip_last_nl=True),
        '#include "duk_unicode_ids_noabmp.c"': read_file(os.path.join(tempdir, 'duk_unicode_ids_noabmp.c.tmp'), strip_last_nl=True),
        '#include "duk_unicode_ids_m_let_noa.c"': read_file(os.path.join(tempdir, 'duk_unicode_ids_m_let_noa.c.tmp'), strip_last_nl=True),
        '#include "duk_unicode_ids_m_let_noabmp.c"': read_file(os.path.join(tempdir, 'duk_unicode_ids_m_let_noabmp.c.tmp'), strip_last_nl=True),
        '#include "duk_unicode_idp_m_ids_noa.c"': read_file(os.path.join(tempdir, 'duk_unicode_idp_m_ids_noa.c.tmp'), strip_last_nl=True),
        '#include "duk_unicode_idp_m_ids_noabmp.c"': read_file(os.path.join(tempdir, 'duk_unicode_idp_m_ids_noabmp.c.tmp'), strip_last_nl=True),
        '#include "duk_unicode_caseconv.c"': read_file(os.path.join(tempdir, 'duk_unicode_caseconv.c.tmp'), strip_last_nl=True),
        '#include "duk_unicode_re_canon_lookup.c"': read_file(os.path.join(tempdir, 'duk_unicode_re_canon_lookup.c.tmp'), strip_last_nl=True),
        '#include "duk_unicode_re_canon_bitmap.c"': read_file(os.path.join(tempdir, 'duk_unicode_re_canon_bitmap.c.tmp'), strip_last_nl=True)
    })

    # Create a combined source file, duktape.c, into a separate combined source
    # directory.  This allows user to just include "duktape.c", "duktape.h", and
    # "duk_config.h" into a project and maximizes inlining and size optimization
    # opportunities even with older compilers.  Because some projects include
    # these files into their repository, the result should be deterministic and
    # diffable.  Also, it must retain __FILE__/__LINE__ behavior through
    # preprocessor directives.  Whitespace and comments can be stripped as long
    # as the other requirements are met.  For some users it's preferable *not*
    # to use #line directives in the combined source, so a separate variant is
    # created for that, see: https://github.com/svaarala/duktape/pull/363.

    def create_source_prologue(license_file, authors_file):
        res = []

        # Because duktape.c/duktape.h/duk_config.h are often distributed or
        # included in project sources as is, add a license reminder and
        # Duktape version information to the duktape.c header (duktape.h
        # already contains them).

        duk_major = duk_version / 10000
        duk_minor = duk_version / 100 % 100
        duk_patch = duk_version % 100
        res.append('/*')
        res.append(' *  Single source autogenerated distributable for Duktape %d.%d.%d.' % (duk_major, duk_minor, duk_patch))
        res.append(' *')
        res.append(' *  Git commit %s (%s).' % (git_commit, git_describe))
        res.append(' *  Git branch %s.' % git_branch)
        res.append(' *')
        res.append(' *  See Duktape AUTHORS.rst and LICENSE.txt for copyright and')
        res.append(' *  licensing information.')
        res.append(' */')
        res.append('')

        # Add LICENSE.txt and AUTHORS.rst to combined source so that they're automatically
        # included and are up-to-date.

        res.append('/* LICENSE.txt */')
        with open(license_file, 'rb') as f:
            for line in f:
                res.append(line.strip())
        res.append('')
        res.append('/* AUTHORS.rst */')
        with open(authors_file, 'rb') as f:
            for line in f:
                res.append(line.strip())

        return '\n'.join(res) + '\n'

    def select_combined_sources():
        # These files must appear before the alphabetically sorted
        # ones so that static variables get defined before they're
        # used.  We can't forward declare them because that would
        # cause C++ issues (see GH-63).  When changing, verify by
        # compiling with g++.
        handpick = [
            'duk_replacements.c',
            'duk_debug_macros.c',
            'duk_builtins.c',
            'duk_error_macros.c',
            'duk_unicode_support.c',
            'duk_util_memrw.c',
            'duk_util_misc.c',
            'duk_hobject_class.c'
        ]

        files = []
        for fn in handpick:
            files.append(fn)

        for fn in sorted(os.listdir(os.path.join(tempdir, 'src'))):
            f_ext = os.path.splitext(fn)[1]
            if f_ext not in [ '.c' ]:
                continue
            if fn in files:
                continue
            files.append(fn)

        res = map(lambda x: os.path.join(tempdir, 'src', x), files)
        logger.debug(repr(files))
        logger.debug(repr(res))
        return res

    if opts.separate_sources:
        for fn in os.listdir(os.path.join(tempdir, 'src')):
            copy_file(os.path.join(tempdir, 'src', fn), os.path.join(outdir, fn))
    else:
        with open(os.path.join(tempdir, 'prologue.tmp'), 'wb') as f:
            f.write(create_source_prologue(os.path.join(tempdir, 'LICENSE.txt.tmp'), os.path.join(tempdir, 'AUTHORS.rst.tmp')))

        cmd = [
            sys.executable,
            os.path.join(script_path, 'combine_src.py'),
            '--include-path', os.path.join(tempdir, 'src'),
            '--include-exclude', 'duk_config.h',  # don't inline
            '--include-exclude', 'duktape.h',     # don't inline
            '--prologue', os.path.join(tempdir, 'prologue.tmp'),
            '--output-source', os.path.join(outdir, 'duktape.c'),
            '--output-metadata', os.path.join(tempdir, 'combine_src_metadata.json')
        ]
        if opts.line_directives:
            cmd += [ '--line-directives' ]
        cmd += select_combined_sources()
        cmd += forward_loglevel
        exec_print_stdout(cmd)

    # Merge metadata files.

    doc = {
        'type': 'duk_source_meta',
        'comment': 'Metadata for prepared Duktape sources and configuration',
        'git_commit': git_commit,
        'git_branch': git_branch,
        'git_describe': git_describe,
        'duk_version': duk_version,
        'duk_version_string': duk_version_formatted
    }
    with open(os.path.join(tempdir, 'genbuiltins_metadata.json'), 'rb') as f:
        tmp = json.loads(f.read())
        for k in tmp.keys():
            doc[k] = tmp[k]
    if opts.separate_sources:
        pass
    else:
        with open(os.path.join(tempdir, 'combine_src_metadata.json'), 'rb') as f:
            tmp = json.loads(f.read())
            for k in tmp.keys():
                doc[k] = tmp[k]

    with open(os.path.join(outdir, 'duk_source_meta.json'), 'wb') as f:
        f.write(json.dumps(doc, indent=4))

    logger.debug('Configure finished successfully')

if __name__ == '__main__':
    main()
