#!/usr/bin/env python2
#
#  Merge debugger YAML metadata files and output a merged JSON metadata file.
#

import logging
import sys
logging.basicConfig(level=logging.INFO, stream=sys.stdout, format='%(name)-21s %(levelname)-7s %(message)s')
logger = logging.getLogger('merge_debug_meta.py')
logger.setLevel(logging.INFO)

import os
import json
import yaml
import optparse

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('--output', dest='output', default=None, help='output JSON filename')
    parser.add_option('--class-names', dest='class_names', help='YAML metadata for class names')
    parser.add_option('--debug-commands', dest='debug_commands', help='YAML metadata for debug commands')
    parser.add_option('--debug-errors', dest='debug_errors', help='YAML metadata for debug protocol error codes')
    parser.add_option('--opcodes', dest='opcodes', help='YAML metadata for opcodes')
    parser.add_option('--quiet', dest='quiet', action='store_true', default=False, help='Suppress info messages (show warnings)')
    parser.add_option('--verbose', dest='verbose', action='store_true', default=False, help='Show verbose debug messages')
    (opts, args) = parser.parse_args()

    # Log level.
    if opts.quiet:
        logger.setLevel(logging.WARNING)
    elif opts.verbose:
        logger.setLevel(logging.DEBUG)

    res = {}
    def merge(fn):
        with open(fn, 'rb') as f:
            doc = yaml.load(f)
        for k in doc.keys():
            res[k] = doc[k]

    merge(opts.class_names)
    merge(opts.debug_commands)
    merge(opts.debug_errors)
    merge(opts.opcodes)

    with open(opts.output, 'wb') as f:
        f.write(json.dumps(res, indent=4) + '\n')
    logger.debug('Wrote merged debugger metadata to ' + str(opts.output))
