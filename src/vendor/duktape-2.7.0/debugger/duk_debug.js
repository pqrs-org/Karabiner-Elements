/*
 *  Minimal debug web console for Duktape command line tool
 *
 *  See debugger/README.rst.
 *
 *  The web UI socket.io communication can easily become a bottleneck and
 *  it's important to ensure that the web UI remains responsive.  Basic rate
 *  limiting mechanisms (token buckets, suppressing identical messages, etc)
 *  are used here now.  Ideally the web UI would pull data on its own terms
 *  which would provide natural rate limiting.
 *
 *  Promises are used to structure callback chains.
 *
 *  https://github.com/petkaantonov/bluebird
 *  https://github.com/petkaantonov/bluebird/blob/master/API.md
 *  https://github.com/petkaantonov/bluebird/wiki/Promise-anti-patterns
 */

var Promise = require('bluebird');
var events = require('events');
var stream = require('stream');
var path = require('path');
var fs = require('fs');
var net = require('net');
var byline = require('byline');
var util = require('util');
var readline = require('readline');
var sprintf = require('sprintf').sprintf;
var utf8 = require('utf8');
var yaml = require('yamljs');
var recursiveReadSync = require('recursive-readdir-sync');

// Command line options (defaults here, overwritten if necessary)
var optTargetHost = '127.0.0.1';
var optTargetPort = 9091;
var optHttpPort = 9092;
var optJsonProxyPort = 9093;
var optJsonProxy = false;
var optSourceSearchDirs = [ '../tests/ecmascript' ];
var optDumpDebugRead = null;
var optDumpDebugWrite = null;
var optDumpDebugPretty = null;
var optLogMessages = false;

// Constants
var UI_MESSAGE_CLIPLEN = 128;
var LOCALS_CLIPLEN = 64;
var EVAL_CLIPLEN = 4096;
var GETVAR_CLIPLEN = 4096;
var SUPPORTED_DEBUG_PROTOCOL_VERSION = 2;

// Commands initiated by Duktape
var CMD_STATUS = 0x01;
var CMD_UNUSED_2 = 0x02;  // Duktape 1.x: print notify
var CMD_UNUSED_3 = 0x03;  // Duktape 1.x: alert notify
var CMD_UNUSED_4 = 0x04;  // Duktape 1.x: log notify
var CMD_THROW = 0x05;
var CMD_DETACHING = 0x06;

// Commands initiated by the debug client (= us)
var CMD_BASICINFO = 0x10;
var CMD_TRIGGERSTATUS = 0x11;
var CMD_PAUSE = 0x12;
var CMD_RESUME = 0x13;
var CMD_STEPINTO = 0x14;
var CMD_STEPOVER = 0x15;
var CMD_STEPOUT = 0x16;
var CMD_LISTBREAK = 0x17;
var CMD_ADDBREAK = 0x18;
var CMD_DELBREAK = 0x19;
var CMD_GETVAR = 0x1a;
var CMD_PUTVAR = 0x1b;
var CMD_GETCALLSTACK = 0x1c;
var CMD_GETLOCALS = 0x1d;
var CMD_EVAL = 0x1e;
var CMD_DETACH = 0x1f;
var CMD_DUMPHEAP = 0x20;
var CMD_GETBYTECODE = 0x21;

// Errors
var ERR_UNKNOWN = 0x00;
var ERR_UNSUPPORTED = 0x01;
var ERR_TOOMANY = 0x02;
var ERR_NOTFOUND = 0x03;

// Marker objects for special protocol values
var DVAL_EOM = { type: 'eom' };
var DVAL_REQ = { type: 'req' };
var DVAL_REP = { type: 'rep' };
var DVAL_ERR = { type: 'err' };
var DVAL_NFY = { type: 'nfy' };

// String map for commands (debug dumping).  A single map works (instead of
// separate maps for each direction) because command numbers don't currently
// overlap.  So merge the YAML metadata.
var debugCommandMeta = yaml.load('duk_debugcommands.yaml');
var debugCommandNames = [];  // list of command names, merged client/target
debugCommandMeta.target_commands.forEach(function (k, i) {
    debugCommandNames[i] = k;
});
debugCommandMeta.client_commands.forEach(function (k, i) {  // override
    debugCommandNames[i] = k;
});
var debugCommandNumbers = {};  // map from (merged) command name to number
debugCommandNames.forEach(function (k, i) {
    debugCommandNumbers[k] = i;
});

// Duktape heaphdr type constants, must match C headers
var DUK_HTYPE_STRING = 0;
var DUK_HTYPE_OBJECT = 1;
var DUK_HTYPE_BUFFER = 2;

// Duktape internal class numbers, must match C headers
var dukClassNameMeta = yaml.load('duk_classnames.yaml');
var dukClassNames = dukClassNameMeta.class_names;

// Bytecode opcode/extraop metadata
var dukOpcodes = yaml.load('duk_opcodes.yaml');
if (dukOpcodes.opcodes.length != 256) {
    throw new Error('opcode metadata length incorrect');
}

/*
 *  Miscellaneous helpers
 */

var nybbles = '0123456789abcdef';

/* Convert a buffer into a string using Unicode codepoints U+0000...U+00FF.
 * This is the NodeJS 'binary' encoding, but since it's being deprecated,
 * reimplement it here.  We need to avoid parsing strings as e.g. UTF-8:
 * although Duktape strings are usually UTF-8/CESU-8 that's not always the
 * case, e.g. for internal strings.  Buffer values are also represented as
 * strings in the debug protocol, so we must deal accurately with arbitrary
 * byte arrays.
 */
function bufferToDebugString(buf) {
    var cp = [];
    var i, n;

/*
    // This fails with "RangeError: Maximum call stack size exceeded" for some
    // reason, so use a much slower variant.

    for (i = 0, n = buf.length; i < n; i++) {
        cp[i] = buf[i];
    }

    return String.fromCharCode.apply(String, cp);
*/

    for (i = 0, n = buf.length; i < n; i++) {
        cp[i] = String.fromCharCode(buf[i]);
    }

    return cp.join('');
}

/* Write a string into a buffer interpreting codepoints U+0000...U+00FF
 * as bytes.  Drop higher bits.
 */
function writeDebugStringToBuffer(str, buf, off) {
    var i, n;

    for (i = 0, n = str.length; i < n; i++) {
        buf[off + i] = str.charCodeAt(i) & 0xff;  // truncate higher bits
    }
}

/* Encode an ordinary Unicode string into a dvalue compatible format, i.e.
 * into a byte array represented as codepoints U+0000...U+00FF.  Concretely,
 * encode with UTF-8 and then represent the bytes with U+0000...U+00FF.
 */
function stringToDebugString(str) {
    return utf8.encode(str);
}

/* Pretty print a dvalue.  Useful for dumping etc. */
function prettyDebugValue(x) {
    if (typeof x === 'object' && x !== null) {
        if (x.type === 'eom') {
            return 'EOM';
        } else if (x.type === 'req') {
            return 'REQ';
        } else if (x.type === 'rep') {
            return 'REP';
        } else if (x.type === 'err') {
            return 'ERR';
        } else if (x.type === 'nfy') {
            return 'NFY';
        }
    }
    return JSON.stringify(x);
}

/* Pretty print a number for UI usage.  Types and values should be easy to
 * read and typing should be obvious.  For numbers, support Infinity, NaN,
 * and signed zeroes properly.
 */
function prettyUiNumber(x) {
    if (x === 1 / 0) { return 'Infinity'; }
    if (x === -1 / 0) { return '-Infinity'; }
    if (Number.isNaN(x)) { return 'NaN'; }
    if (x === 0 && 1 / x > 0) { return '0'; }
    if (x === 0 && 1 / x < 0) { return '-0'; }
    return x.toString();
}

/* Pretty print a dvalue string (bytes represented as U+0000...U+00FF)
 * for UI usage.  Try UTF-8 decoding to get a nice Unicode string (JSON
 * encoded) but if that fails, ensure that bytes are encoded transparently.
 * The result is a quoted string with a special quote marker for a "raw"
 * string when UTF-8 decoding fails.  Very long strings are optionally
 * clipped.
 */
function prettyUiString(x, cliplen) {
    var ret;

    if (typeof x !== 'string') {
        throw new Error('invalid input to prettyUiString: ' + typeof x);
    }
    try {
        // Here utf8.decode() is better than decoding using NodeJS buffer
        // operations because we want strict UTF-8 interpretation.
        ret = JSON.stringify(utf8.decode(x));
    } catch (e) {
        // When we fall back to representing bytes, indicate that the string
        // is "raw" with a 'r"' prefix (a somewhat arbitrary convention).
        // U+0022 = ", U+0027 = '
        ret = 'r"' + x.replace(/[\u0022\u0027\u0000-\u001f\u0080-\uffff]/g, function (match) {
            var cp = match.charCodeAt(0);
            return '\\x' + nybbles[(cp >> 4) & 0x0f] + nybbles[cp & 0x0f];
        }) + '"';
    }

    if (cliplen && ret.length > cliplen) {
        ret = ret.substring(0, cliplen) + '...';  // trailing '"' intentionally missing
    }
    return ret;
}

/* Pretty print a dvalue string (bytes represented as U+0000...U+00FF)
 * for UI usage without quotes.
 */
function prettyUiStringUnquoted(x, cliplen) {
    var ret;

    if (typeof x !== 'string') {
        throw new Error('invalid input to prettyUiStringUnquoted: ' + typeof x);
    }

    try {
        // Here utf8.decode() is better than decoding using NodeJS buffer
        // operations because we want strict UTF-8 interpretation.

        // XXX: unprintable characters etc?  In some UI cases we'd want to
        // e.g. escape newlines and in others not.
        ret = utf8.decode(x);
    } catch (e) {
        // For the unquoted version we don't need to escape single or double
        // quotes.
        ret = x.replace(/[\u0000-\u001f\u0080-\uffff]/g, function (match) {
            var cp = match.charCodeAt(0);
            return '\\x' + nybbles[(cp >> 4) & 0x0f] + nybbles[cp & 0x0f];
        });
    }

    if (cliplen && ret.length > cliplen) {
        ret = ret.substring(0, cliplen) + '...';
    }
    return ret;
}

/* Pretty print a dvalue for UI usage.  Everything comes out as a ready-to-use
 * string.
 *
 * XXX: Currently the debug client formats all values for UI use.  A better
 * solution would be to pass values in typed form and let the UI format them,
 * so that styling etc. could take typing into account.
 */
function prettyUiDebugValue(x, cliplen) {
    if (typeof x === 'object' && x !== null) {
        // Note: typeof null === 'object', so null special case explicitly
        if (x.type === 'eom') {
            return 'EOM';
        } else if (x.type === 'req') {
            return 'REQ';
        } else if (x.type === 'rep') {
            return 'REP';
        } else if (x.type === 'err') {
            return 'ERR';
        } else if (x.type === 'nfy') {
            return 'NFY';
        } else if (x.type === 'unused') {
            return 'unused';
        } else if (x.type === 'undefined') {
            return 'undefined';
        } else if (x.type === 'buffer') {
            return '|' + x.data + '|';
        } else if (x.type === 'object') {
            return '[object ' + (dukClassNames[x.class] || ('class ' + x.class)) + ' ' + x.pointer + ']';
        } else if (x.type === 'pointer') {
            return '<pointer ' + x.pointer + '>';
        } else if (x.type === 'lightfunc') {
            return '<lightfunc 0x' + x.flags.toString(16) + ' ' + x.pointer + '>';
        } else if (x.type === 'number') {
            // duk_tval number, any IEEE double
            var tmp = new Buffer(x.data, 'hex');  // decode into hex
            var val = tmp.readDoubleBE(0);        // big endian ieee double
            return prettyUiNumber(val);
        }
    } else if (x === null) {
        return 'null';
    } else if (typeof x === 'boolean') {
        return x ? 'true' : 'false';
    } else if (typeof x === 'string') {
        return prettyUiString(x, cliplen);
    } else if (typeof x === 'number') {
        // Debug protocol integer
        return prettyUiNumber(x);
    }

    // We shouldn't come here, but if we do, JSON is a reasonable default.
    return JSON.stringify(x);
}

/* Pretty print a debugger message given as an array of parsed dvalues.
 * Result should be a pure ASCII one-liner.
 */
function prettyDebugMessage(msg) {
    return msg.map(prettyDebugValue).join(' ');
}

/* Pretty print a debugger command. */
function prettyDebugCommand(cmd) {
    return debugCommandNames[cmd] || String(cmd);
}

/* Decode and normalize source file contents: UTF-8, tabs to 8,
 * CR LF to LF.
 */
function decodeAndNormalizeSource(data) {
    var tmp;
    var lines, line, repl;
    var i, n;
    var j, m;

    try {
        tmp = data.toString('utf8');
    } catch (e) {
        console.log('Failed to UTF-8 decode source file, ignoring: ' + e);
        tmp = String(data);
    }

    lines = tmp.split(/\r?\n/);
    for (i = 0, n = lines.length; i < n; i++) {
        line = lines[i];
        if (/\t/.test(line)) {
            repl = '';
            for (j = 0, m = line.length; j < m; j++) {
                if (line.charAt(j) === '\t') {
                    repl += ' ';
                    while ((repl.length % 8) != 0) {
                        repl += ' ';
                    }
                } else {
                    repl += line.charAt(j);
                }
            }
            lines[i] = repl;
        }
    }

    // XXX: normalize last newline (i.e. force a newline if contents don't
    // end with a newline)?

    return lines.join('\n');
}

/* Token bucket rate limiter for a given callback.  Calling code calls
 * trigger() to request 'cb' to be called, and the rate limiter ensures
 * that 'cb' is not called too often.
 */
function RateLimited(tokens, rate, cb) {
    var _this = this;
    this.maxTokens = tokens;
    this.tokens = this.maxTokens;
    this.rate = rate;
    this.cb = cb;
    this.delayedCb = false;

    // Right now the implementation is setInterval-based, but could also be
    // made timerless.  There are so few rate limited resources that this
    // doesn't matter in practice.

    this.tokenAdder = setInterval(function () {
        if (_this.tokens < _this.maxTokens) {
            _this.tokens++;
        }
        if (_this.delayedCb) {
            _this.delayedCb = false;
            _this.tokens--;
            _this.cb();
        }
    }, this.rate);
}
RateLimited.prototype.trigger = function () {
    if (this.tokens > 0) {
        this.tokens--;
        this.cb();
    } else {
        this.delayedCb = true;
    }
};

/*
 *  Source file manager
 *
 *  Scan the list of search directories for ECMAScript source files and
 *  build an index of them.  Provides a mechanism to find a source file
 *  based on a raw 'fileName' property provided by the debug target, and
 *  to provide a file list for the web UI.
 *
 *  NOTE: it's tempting to do loose matching for filenames, but this does
 *  not work in practice.  Filenames must match 1:1 with the debug target
 *  so that e.g. breakpoints assigned based on filenames found from the
 *  search paths will match 1:1 on the debug target.  If this is not the
 *  case, breakpoints won't work as expected.
 */

function SourceFileManager(directories) {
    this.directories = directories;
    this.extensions = { '.js': true, '.jsm': true };
    this.fileMap = {};  // filename as seen by debug target -> file path
    this.files = [];    // filenames as seen by debug target
}

SourceFileManager.prototype.scan = function () {
    var _this = this;
    var fileMap = {};   // relative path -> file path
    var files;

    this.directories.forEach(function (dir) {
        console.log('Scanning source files: ' + dir);
        try {
            recursiveReadSync(dir).forEach(function (fn) {
                // Example: dir     ../../tests
                //          normFn  ../../tests/foo/bar.js
                //          relFn   foo/bar.js
                var normDir = path.normalize(dir);
                var normFn = path.normalize(fn);
                var relFn = path.relative(normDir, normFn);

                if (fs.existsSync(normFn) && fs.lstatSync(normFn).isFile() &&
                    _this.extensions[path.extname(normFn)]) {
                    // We want the fileMap to contain the filename relative to
                    // the search dir root.  First directory containing a
                    // certail relFn wins.
                    if (relFn in fileMap) {
                        console.log('Found', relFn, 'multiple times, first match wins');
                    } else {
                        fileMap[relFn] = normFn;
                    }
                }
            });
        } catch (e) {
            console.log('Failed to scan ' + dir + ': ' + e);
        }
    });

    files = Object.keys(fileMap);
    files.sort();
    this.files = files;

    this.fileMap = fileMap;

    console.log('Found ' + files.length + ' source files in ' + this.directories.length + ' search directories');
};

SourceFileManager.prototype.getFiles = function () {
    return this.files;
};

SourceFileManager.prototype.search = function (fileName) {
    var _this = this;

    // Loose matching is tempting but counterproductive: filenames must
    // match 1:1 between the debug client and the debug target for e.g.
    // breakpoints to work as expected.  Note that a breakpoint may be
    // assigned by selecting a file from a dropdown populated by scanning
    // the filesystem for available sources and there's no way of knowing
    // if the debug target uses the exact same name.
    //
    // We intentionally allow any files from the search paths, not just
    // those scanned to this.fileMap.

    function tryLookup() {
        var i, fn, data;

        for (i = 0; i < _this.directories.length; i++) {
            fn = path.join(_this.directories[i], fileName);
            if (fs.existsSync(fn) && fs.lstatSync(fn).isFile()) {
                data = fs.readFileSync(fn);             // Raw bytes
                return decodeAndNormalizeSource(data);  // Unicode string
            }
        }
        return null;
    }

    return tryLookup(fileName);
};

/*
 *  Debug protocol parser
 *
 *  The debug protocol parser is an EventEmitter which parses debug messages
 *  from an input stream and emits 'debug-message' events for completed
 *  messages ending in an EOM.  The parser also provides debug dumping, stream
 *  logging functionality, and statistics gathering functionality.
 *
 *  This parser is used to parse both incoming and outgoing messages.  For
 *  outgoing messages the only function is to validate and debug dump the
 *  messages we're about to send.  The downside of dumping at this low level
 *  is that we can't match request and reply/error messages here.
 *
 *  http://www.sitepoint.com/nodejs-events-and-eventemitter/
 */

function DebugProtocolParser(inputStream,
                             protocolVersion,
                             rawDumpFileName,
                             textDumpFileName,
                             textDumpFilePrefix,
                             hexDumpConsolePrefix,
                             textDumpConsolePrefix) {
    var _this = this;
    this.inputStream = inputStream;
    this.closed = false;       // stream is closed/broken, don't parse anymore
    this.bytes = 0;
    this.dvalues = 0;
    this.messages = 0;
    this.requests = 0;
    this.prevBytes = 0;
    this.bytesPerSec = 0;
    this.statsTimer = null;
    this.readableNumberValue = true;

    events.EventEmitter.call(this);

    var buf = new Buffer(0);    // accumulate data
    var msg = [];               // accumulated message until EOM
    var versionIdentification;

    var statsInterval = 2000;
    var statsIntervalSec = statsInterval / 1000;
    this.statsTimer = setInterval(function () {
        _this.bytesPerSec = (_this.bytes - _this.prevBytes) / statsIntervalSec;
        _this.prevBytes = _this.bytes;
        _this.emit('stats-update');
    }, statsInterval);

    function consume(n) {
        var tmp = new Buffer(buf.length - n);
        buf.copy(tmp, 0, n);
        buf = tmp;
    }

    inputStream.on('data', function (data) {
        var i, n, x, v, gotValue, len, t, tmpbuf, verstr;
        var prettyMsg;

        if (_this.closed || !_this.inputStream) {
            console.log('Ignoring incoming data from closed input stream, len ' + data.length);
            return;
        }

        _this.bytes += data.length;
        if (rawDumpFileName) {
            fs.appendFileSync(rawDumpFileName, data);
        }
        if (hexDumpConsolePrefix) {
            console.log(hexDumpConsolePrefix + data.toString('hex'));
        }

        buf = Buffer.concat([ buf, data ]);

        // Protocol version handling.  When dumping an output stream, the
        // caller gives a non-null protocolVersion so we don't read one here.
        if (protocolVersion == null) {
            if (buf.length > 1024) {
                _this.emit('transport-error', 'Parse error (version identification too long), dropping connection');
                _this.close();
                return;
            }

            for (i = 0, n = buf.length; i < n; i++) {
                if (buf[i] == 0x0a) {
                    tmpbuf = new Buffer(i);
                    buf.copy(tmpbuf, 0, 0, i);
                    consume(i + 1);
                    verstr = tmpbuf.toString('utf-8');
                    t = verstr.split(' ');
                    protocolVersion = Number(t[0]);
                    versionIdentification = verstr;

                    _this.emit('protocol-version', {
                        protocolVersion: protocolVersion,
                        versionIdentification: versionIdentification
                    });
                    break;
                }
            }

            if (protocolVersion == null) {
                // Still waiting for version identification to complete.
                return;
            }
        }

        // Parse complete dvalues (quite inefficient now) by trial parsing.
        // Consume a value only when it's fully present in 'buf'.
        // See doc/debugger.rst for format description.

        while (buf.length > 0) {
            x = buf[0];
            v = undefined;
            gotValue = false;  // used to flag special values like undefined

            if (x >= 0xc0) {
                // 0xc0...0xff: integers 0-16383
                if (buf.length >= 2) {
                    v = ((x - 0xc0) << 8) + buf[1];
                    consume(2);
                }
            } else if (x >= 0x80) {
                // 0x80...0xbf: integers 0-63
                v = x - 0x80;
                consume(1);
            } else if (x >= 0x60) {
                // 0x60...0x7f: strings with length 0-31
                len = x - 0x60;
                if (buf.length >= 1 + len) {
                    v = new Buffer(len);
                    buf.copy(v, 0, 1, 1 + len);
                    v = bufferToDebugString(v);
                    consume(1 + len);
                }
            } else {
                switch (x) {
                case 0x00: v = DVAL_EOM; consume(1); break;
                case 0x01: v = DVAL_REQ; consume(1); break;
                case 0x02: v = DVAL_REP; consume(1); break;
                case 0x03: v = DVAL_ERR; consume(1); break;
                case 0x04: v = DVAL_NFY; consume(1); break;
                case 0x10:  // 4-byte signed integer
                    if (buf.length >= 5) {
                        v = buf.readInt32BE(1);
                        consume(5);
                    }
                    break;
                case 0x11:  // 4-byte string
                    if (buf.length >= 5) {
                        len = buf.readUInt32BE(1);
                        if (buf.length >= 5 + len) {
                            v = new Buffer(len);
                            buf.copy(v, 0, 5, 5 + len);
                            v = bufferToDebugString(v);
                            consume(5 + len);
                        }
                    }
                    break;
                case 0x12:  // 2-byte string
                    if (buf.length >= 3) {
                        len = buf.readUInt16BE(1);
                        if (buf.length >= 3 + len) {
                            v = new Buffer(len);
                            buf.copy(v, 0, 3, 3 + len);
                            v = bufferToDebugString(v);
                            consume(3 + len);
                        }
                    }
                    break;
                case 0x13:  // 4-byte buffer
                    if (buf.length >= 5) {
                        len = buf.readUInt32BE(1);
                        if (buf.length >= 5 + len) {
                            v = new Buffer(len);
                            buf.copy(v, 0, 5, 5 + len);
                            v = { type: 'buffer', data: v.toString('hex') };
                            consume(5 + len);
                            // Value could be a Node.js buffer directly, but
                            // we prefer all dvalues to be JSON compatible
                        }
                    }
                    break;
                case 0x14:  // 2-byte buffer
                    if (buf.length >= 3) {
                        len = buf.readUInt16BE(1);
                        if (buf.length >= 3 + len) {
                            v = new Buffer(len);
                            buf.copy(v, 0, 3, 3 + len);
                            v = { type: 'buffer', data: v.toString('hex') };
                            consume(3 + len);
                            // Value could be a Node.js buffer directly, but
                            // we prefer all dvalues to be JSON compatible
                        }
                    }
                    break;
                case 0x15:  // unused/none
                    v = { type: 'unused' };
                    consume(1);
                    break;
                case 0x16:  // undefined
                    v = { type: 'undefined' };
                    gotValue = true;  // indicate 'v' is actually set
                    consume(1);
                    break;
                case 0x17:  // null
                    v = null;
                    gotValue = true;  // indicate 'v' is actually set
                    consume(1);
                    break;
                case 0x18:  // true
                    v = true;
                    consume(1);
                    break;
                case 0x19:  // false
                    v = false;
                    consume(1);
                    break;
                case 0x1a:  // number (IEEE double), big endian
                    if (buf.length >= 9) {
                        v = new Buffer(8);
                        buf.copy(v, 0, 1, 9);
                        v = { type: 'number', data: v.toString('hex') };

                        if (_this.readableNumberValue) {
                            // The value key should not be used programmatically,
                            // it is just there to make the dumps more readable.
                            v.value = buf.readDoubleBE(1);
                        }
                        consume(9);
                    }
                    break;
                case 0x1b:  // object
                    if (buf.length >= 3) {
                        len = buf[2];
                        if (buf.length >= 3 + len) {
                            v = new Buffer(len);
                            buf.copy(v, 0, 3, 3 + len);
                            v = { type: 'object', 'class': buf[1], pointer: v.toString('hex') };
                            consume(3 + len);
                        }
                    }
                    break;
                case 0x1c:  // pointer
                    if (buf.length >= 2) {
                        len = buf[1];
                        if (buf.length >= 2 + len) {
                            v = new Buffer(len);
                            buf.copy(v, 0, 2, 2 + len);
                            v = { type: 'pointer', pointer: v.toString('hex') };
                            consume(2 + len);
                        }
                    }
                    break;
                case 0x1d:  // lightfunc
                    if (buf.length >= 4) {
                        len = buf[3];
                        if (buf.length >= 4 + len) {
                            v = new Buffer(len);
                            buf.copy(v, 0, 4, 4 + len);
                            v = { type: 'lightfunc', flags: buf.readUInt16BE(1), pointer: v.toString('hex') };
                            consume(4 + len);
                        }
                    }
                    break;
                case 0x1e:  // heapptr
                    if (buf.length >= 2) {
                        len = buf[1];
                        if (buf.length >= 2 + len) {
                            v = new Buffer(len);
                            buf.copy(v, 0, 2, 2 + len);
                            v = { type: 'heapptr', pointer: v.toString('hex') };
                            consume(2 + len);
                        }
                    }
                    break;
                default:
                    _this.emit('transport-error', 'Parse error, dropping connection');
                    _this.close();
                }
            }

            if (typeof v === 'undefined' && !gotValue) {
                break;
            }
            msg.push(v);
            _this.dvalues++;

            // Could emit a 'debug-value' event here, but that's not necessary
            // because the receiver will just collect statistics which can also
            // be done using the finished message.

            if (v === DVAL_EOM) {
                _this.messages++;

                if (textDumpFileName || textDumpConsolePrefix) {
                    prettyMsg = prettyDebugMessage(msg);
                    if (textDumpFileName) {
                        fs.appendFileSync(textDumpFileName, (textDumpFilePrefix || '') + prettyMsg + '\n');
                    }
                    if (textDumpConsolePrefix) {
                        console.log(textDumpConsolePrefix + prettyMsg);
                    }
                }

                _this.emit('debug-message', msg);
                msg = [];  // new object, old may be in circulation for a while
            }
        }
    });

    // Not all streams will emit this.
    inputStream.on('error', function (err) {
        _this.emit('transport-error', err);
        _this.close();
    });

    // Not all streams will emit this.
    inputStream.on('close', function () {
        _this.close();
    });
}
DebugProtocolParser.prototype = Object.create(events.EventEmitter.prototype);

DebugProtocolParser.prototype.close = function () {
    // Although the underlying transport may not have a close() or destroy()
    // method or even a 'close' event, this method is always available and
    // will generate a 'transport-close'.
    //
    // The caller is responsible for closing the underlying stream if that
    // is necessary.

    if (this.closed) { return; }

    this.closed = true;
    if (this.statsTimer) {
        clearInterval(this.statsTimer);
        this.statsTimer = null;
    }
    this.emit('transport-close');
};

/*
 *  Debugger output formatting
 */

function formatDebugValue(v) {
    var buf, dec, len;

    // See doc/debugger.rst for format description.

    if (typeof v === 'object' && v !== null) {
        // Note: typeof null === 'object', so null special case explicitly
        if (v.type === 'eom') {
            return new Buffer([ 0x00 ]);
        } else if (v.type === 'req') {
            return new Buffer([ 0x01 ]);
        } else if (v.type === 'rep') {
            return new Buffer([ 0x02 ]);
        } else if (v.type === 'err') {
            return new Buffer([ 0x03 ]);
        } else if (v.type === 'nfy') {
            return new Buffer([ 0x04 ]);
        } else if (v.type === 'unused') {
            return new Buffer([ 0x15 ]);
        } else if (v.type === 'undefined') {
            return new Buffer([ 0x16 ]);
        } else if (v.type === 'number') {
            dec = new Buffer(v.data, 'hex');
            len = dec.length;
            if (len !== 8) {
                throw new TypeError('value cannot be converted to dvalue: ' + JSON.stringify(v));
            }
            buf = new Buffer(1 + len);
            buf[0] = 0x1a;
            dec.copy(buf, 1);
            return buf;
        } else if (v.type === 'buffer') {
            dec = new Buffer(v.data, 'hex');
            len = dec.length;
            if (len <= 0xffff) {
                buf = new Buffer(3 + len);
                buf[0] = 0x14;
                buf[1] = (len >> 8) & 0xff;
                buf[2] = (len >> 0) & 0xff;
                dec.copy(buf, 3);
                return buf;
            } else {
                buf = new Buffer(5 + len);
                buf[0] = 0x13;
                buf[1] = (len >> 24) & 0xff;
                buf[2] = (len >> 16) & 0xff;
                buf[3] = (len >> 8) & 0xff;
                buf[4] = (len >> 0) & 0xff;
                dec.copy(buf, 5);
                return buf;
            }
        } else if (v.type === 'object') {
            dec = new Buffer(v.pointer, 'hex');
            len = dec.length;
            buf = new Buffer(3 + len);
            buf[0] = 0x1b;
            buf[1] = v.class;
            buf[2] = len;
            dec.copy(buf, 3);
            return buf;
        } else if (v.type === 'pointer') {
            dec = new Buffer(v.pointer, 'hex');
            len = dec.length;
            buf = new Buffer(2 + len);
            buf[0] = 0x1c;
            buf[1] = len;
            dec.copy(buf, 2);
            return buf;
        } else if (v.type === 'lightfunc') {
            dec = new Buffer(v.pointer, 'hex');
            len = dec.length;
            buf = new Buffer(4 + len);
            buf[0] = 0x1d;
            buf[1] = (v.flags >> 8) & 0xff;
            buf[2] = v.flags & 0xff;
            buf[3] = len;
            dec.copy(buf, 4);
            return buf;
        } else if (v.type === 'heapptr') {
            dec = new Buffer(v.pointer, 'hex');
            len = dec.length;
            buf = new Buffer(2 + len);
            buf[0] = 0x1e;
            buf[1] = len;
            dec.copy(buf, 2);
            return buf;
        }
    } else if (v === null) {
        return new Buffer([ 0x17 ]);
    } else if (typeof v === 'boolean') {
        return new Buffer([ v ? 0x18 : 0x19 ]);
    } else if (typeof v === 'number') {
        if (Math.floor(v) === v &&     /* whole */
            (v !== 0 || 1 / v > 0) &&  /* not negative zero */
            v >= -0x80000000 && v <= 0x7fffffff) {
            // Represented signed 32-bit integers as plain integers.
            // Debugger code expects this for all fields that are not
            // duk_tval representations (e.g. command numbers and such).
            if (v >= 0x00 && v <= 0x3f) {
                return new Buffer([ 0x80 + v ]);
            } else if (v >= 0x0000 && v <= 0x3fff) {
                return new Buffer([ 0xc0 + (v >> 8), v & 0xff ]);
            } else if (v >= -0x80000000 && v <= 0x7fffffff) {
                return new Buffer([ 0x10,
                                    (v >> 24) & 0xff,
                                    (v >> 16) & 0xff,
                                    (v >> 8) & 0xff,
                                    (v >> 0) & 0xff ]);
            } else {
                throw new Error('internal error when encoding integer to dvalue: ' + v);
            }
        } else {
            // Represent non-integers as IEEE double dvalues
            buf = new Buffer(1 + 8);
            buf[0] = 0x1a;
            buf.writeDoubleBE(v, 1);
            return buf;
        }
    } else if (typeof v === 'string') {
        if (v.length < 0 || v.length > 0xffffffff) {
            // Not possible in practice.
            throw new TypeError('cannot convert to dvalue, invalid string length: ' + v.length);
        }
        if (v.length <= 0x1f) {
            buf = new Buffer(1 + v.length);
            buf[0] = 0x60 + v.length;
            writeDebugStringToBuffer(v, buf, 1);
            return buf;
        } else if (v.length <= 0xffff) {
            buf = new Buffer(3 + v.length);
            buf[0] = 0x12;
            buf[1] = (v.length >> 8) & 0xff;
            buf[2] = (v.length >> 0) & 0xff;
            writeDebugStringToBuffer(v, buf, 3);
            return buf;
        } else {
            buf = new Buffer(5 + v.length);
            buf[0] = 0x11;
            buf[1] = (v.length >> 24) & 0xff;
            buf[2] = (v.length >> 16) & 0xff;
            buf[3] = (v.length >> 8) & 0xff;
            buf[4] = (v.length >> 0) & 0xff;
            writeDebugStringToBuffer(v, buf, 5);
            return buf;
        }
    }

    // Shouldn't come here.
    throw new TypeError('value cannot be converted to dvalue: ' + JSON.stringify(v));
}

/*
 *  Debugger implementation
 *
 *  A debugger instance communicates with the debug target and maintains
 *  persistent debug state so that the current state can be resent to the
 *  socket.io client (web UI) if it reconnects.  Whenever the debugger state
 *  is changed an event is generated.  The socket.io handler will listen to
 *  state change events and push the necessary updates to the web UI, often
 *  in a rate limited fashion or using a client pull to ensure the web UI
 *  is not overloaded.
 *
 *  The debugger instance assumes that if the debug protocol connection is
 *  re-established, it is always to the same target.  There is no separate
 *  abstraction for a debugger session.
 */

function Debugger() {
    events.EventEmitter.call(this);

    this.web = null;                      // web UI singleton
    this.targetStream = null;             // transport connection to target
    this.outputPassThroughStream = null;  // dummy passthrough for message dumping
    this.inputParser = null;              // parser for incoming debug messages
    this.outputParser = null;             // parser for outgoing debug messages (stats, dumping)
    this.protocolVersion = null;
    this.dukVersion = null;
    this.dukGitDescribe = null;
    this.targetInfo = null;
    this.attached = false;
    this.handshook = false;
    this.reqQueue = null;
    this.stats = {                        // stats for current debug connection
        rxBytes: 0, rxDvalues: 0, rxMessages: 0, rxBytesPerSec: 0,
        txBytes: 0, txDvalues: 0, txMessages: 0, txBytesPerSec: 0
    };
    this.execStatus = {
        attached: false,
        state: 'detached',
        fileName: '',
        funcName: '',
        line: 0,
        pc: 0
    };
    this.breakpoints = [];
    this.callstack = [];
    this.locals = [];
    this.messageLines = [];
    this.messageScrollBack = 100;
}
Debugger.prototype = events.EventEmitter.prototype;

Debugger.prototype.decodeBytecodeFromBuffer = function (buf, consts, funcs) {
    var i, j, n, m, ins, pc;
    var res = [];
    var op, str, args, comments;

    // XXX: add constants inline to preformatted output (e.g. for strings,
    // add a short escaped snippet as a comment on the line after the
    // compact argument list).

    for (i = 0, n = buf.length; i < n; i += 4) {
        pc = i / 4;

        // shift forces unsigned
        if (this.endianness === 'little') {
            ins = buf.readInt32LE(i) >>> 0;
        } else {
            ins = buf.readInt32BE(i) >>> 0;
        }

        op = dukOpcodes.opcodes[ins & 0xff];

        args = [];
        comments = [];
        if (op.args) {
            for (j = 0, m = op.args.length; j < m; j++) {
                var A = (ins >>> 8) & 0xff;
                var B = (ins >>> 16) & 0xff;
                var C = (ins >>> 24) & 0xff;
                var BC = (ins >>> 16) & 0xffff;
                var ABC = (ins >>> 8) & 0xffffff;
                var Bconst = op & 0x01;
                var Cconst = op & 0x02;

                switch (op.args[j]) {
                case 'A_R':   args.push('r' + A); break;
                case 'A_RI':  args.push('r' + A + '(indirect)'); break;
                case 'A_C':   args.push('c' + A); break;
                case 'A_H':   args.push('0x' + A.toString(16)); break;
                case 'A_I':   args.push(A.toString(10)); break;
                case 'A_B':   args.push(A ? 'true' : 'false'); break;
                case 'B_RC':  args.push((Bconst ? 'c' : 'r') + B); break;
                case 'B_R':   args.push('r' + B); break;
                case 'B_RI':  args.push('r' + B + '(indirect)'); break;
                case 'B_C':   args.push('c' + B); break;
                case 'B_H':   args.push('0x' + B.toString(16)); break;
                case 'B_I':   args.push(B.toString(10)); break;
                case 'C_RC':  args.push((Cconst ? 'c' : 'r') + C); break;
                case 'C_R':   args.push('r' + C); break;
                case 'C_RI':  args.push('r' + C + '(indirect)'); break;
                case 'C_C':   args.push('c' + C); break;
                case 'C_H':   args.push('0x' + C.toString(16)); break;
                case 'C_I':   args.push(C.toString(10)); break;
                case 'BC_R':  args.push('r' + BC); break;
                case 'BC_C':  args.push('c' + BC); break;
                case 'BC_H':  args.push('0x' + BC.toString(16)); break;
                case 'BC_I':  args.push(BC.toString(10)); break;
                case 'ABC_H': args.push(ABC.toString(16)); break;
                case 'ABC_I': args.push(ABC.toString(10)); break;
                case 'BC_LDINT': args.push(BC - (1 << 15)); break;
                case 'BC_LDINTX': args.push(BC - 0); break;  // no bias in LDINTX
                case 'ABC_JUMP': {
                    var pc_add = ABC - (1 << 23) + 1;  // pc is preincremented before adding
                    var pc_dst = pc + pc_add;
                    args.push(pc_dst + ' (' + (pc_add >= 0 ? '+' : '') + pc_add + ')');
                    break;
                }
                default:      args.push('?'); break;
                }
            }
        }
        if (op.flags) {
            for (j = 0, m = op.flags.length; j < m; j++) {
                if (ins & op.flags[j].mask) {
                    comments.push(op.flags[j].name);
                }
            }
        }

        if (args.length > 0) {
            str = sprintf('%05d %08x   %-12s %s', pc, ins, op.name, args.join(', '));
        } else {
            str = sprintf('%05d %08x   %-12s', pc, ins, op.name);
        }
        if (comments.length > 0) {
            str = sprintf('%-44s ; %s', str, comments.join(', '));
        }

        res.push({
            str: str,
            ins: ins
        });
    }

    return res;
};

Debugger.prototype.uiMessage = function (type, val) {
    var msg;
    if (typeof type === 'object') {
        msg = type;
    } else if (typeof type === 'string') {
        msg = { type: type, message: val };
    } else {
        throw new TypeError('invalid ui message: ' + type);
    }
    this.messageLines.push(msg);
    while (this.messageLines.length > this.messageScrollBack) {
        this.messageLines.shift();
    }
    this.emit('ui-message-update');  // just trigger a sync, gets rate limited
};

Debugger.prototype.sendRequest = function (msg) {
    var _this = this;
    return new Promise(function (resolve, reject) {
        var dvals = [];
        var dval;
        var data;
        var i;

        if (!_this.attached || !_this.handshook || !_this.reqQueue || !_this.targetStream) {
            throw new Error('invalid state for sendRequest');
        }

        for (i = 0; i < msg.length; i++) {
            try {
                dval = formatDebugValue(msg[i]);
            } catch (e) {
                console.log('Failed to format dvalue, dropping connection: ' + e);
                console.log(e.stack || e);
                _this.targetStream.destroy();
                throw new Error('failed to format dvalue');
            }
            dvals.push(dval);
        }

        data = Buffer.concat(dvals);

        _this.targetStream.write(data);
        _this.outputPassThroughStream.write(data);  // stats and dumping

        if (optLogMessages) {
            console.log('Request ' + prettyDebugCommand(msg[1]) + ': ' + prettyDebugMessage(msg));
        }

        if (!_this.reqQueue) {
            throw new Error('no reqQueue');
        }

        _this.reqQueue.push({
            reqMsg: msg,
            reqCmd: msg[1],
            resolveCb: resolve,
            rejectCb: reject
        });
    });
};

Debugger.prototype.sendBasicInfoRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_BASICINFO, DVAL_EOM ]).then(function (msg) {
        _this.dukVersion = msg[1];
        _this.dukGitDescribe = msg[2];
        _this.targetInfo = msg[3];
        _this.endianness = { 1: 'little', 2: 'mixed', 3: 'big' }[msg[4]] || 'unknown';
        _this.emit('basic-info-update');
        return msg;
    });
};

Debugger.prototype.sendGetVarRequest = function (varname, level) {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_GETVAR, (typeof level === 'number' ? level : -1), varname, DVAL_EOM ]).then(function (msg) {
        return { found: msg[1] === 1, value: msg[2] };
    });
};

Debugger.prototype.sendPutVarRequest = function (varname, varvalue, level) {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_PUTVAR, (typeof level === 'number' ? level : -1), varname, varvalue, DVAL_EOM ]);
};

Debugger.prototype.sendInvalidCommandTestRequest = function () {
    // Intentional invalid command
    var _this = this;
    return this.sendRequest([ DVAL_REQ, 0xdeadbeef, DVAL_EOM ]);
};

Debugger.prototype.sendStatusRequest = function () {
    // Send a status request to trigger a status notify, result is ignored:
    // target sends a status notify instead of a meaningful reply
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_TRIGGERSTATUS, DVAL_EOM ]);
};

Debugger.prototype.sendBreakpointListRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_LISTBREAK, DVAL_EOM ]).then(function (msg) {
        var i, n;
        var breakpts = [];

        for (i = 1, n = msg.length - 1; i < n; i += 2) {
            breakpts.push({ fileName: msg[i], lineNumber: msg[i + 1] });
        }

        _this.breakpoints = breakpts;
        _this.emit('breakpoints-update');
        return msg;
    });
};

Debugger.prototype.sendGetLocalsRequest = function (level) {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_GETLOCALS, (typeof level === 'number' ? level : -1), DVAL_EOM ]).then(function (msg) {
        var i;
        var locals = [];

        for (i = 1; i <= msg.length - 2; i += 2) {
            // XXX: do pretty printing in debug client for now
            locals.push({ key: msg[i], value: prettyUiDebugValue(msg[i + 1], LOCALS_CLIPLEN) });
        }

        _this.locals = locals;
        _this.emit('locals-update');
        return msg;
    });
};

Debugger.prototype.sendGetCallStackRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_GETCALLSTACK, DVAL_EOM ]).then(function (msg) {
        var i;
        var stack = [];

        for (i = 1; i + 3 <= msg.length - 1; i += 4) {
            stack.push({
                fileName: msg[i],
                funcName: msg[i + 1],
                lineNumber: msg[i + 2],
                pc: msg[i + 3]
            });
        }

        _this.callstack = stack;
        _this.emit('callstack-update');
        return msg;
    });
};

Debugger.prototype.sendStepIntoRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_STEPINTO, DVAL_EOM ]);
};

Debugger.prototype.sendStepOverRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_STEPOVER, DVAL_EOM ]);
};

Debugger.prototype.sendStepOutRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_STEPOUT, DVAL_EOM ]);
};

Debugger.prototype.sendPauseRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_PAUSE, DVAL_EOM ]);
};

Debugger.prototype.sendResumeRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_RESUME, DVAL_EOM ]);
};

Debugger.prototype.sendEvalRequest = function (evalInput, level) {
    var _this = this;
    // Use explicit level if given.  If no level is given, use null if the call
    // stack is empty, -1 otherwise.  This works well when we're paused and the
    // callstack information is not liable to change before we do an Eval.
    if (typeof level !== 'number') {
        level = this.callstack && this.callstack.length > 0 ? -1 : null;
    }
    return this.sendRequest([ DVAL_REQ, CMD_EVAL, level, evalInput, DVAL_EOM ]).then(function (msg) {
        return { error: msg[1] === 1 /*error*/, value: msg[2] };
    });
};

Debugger.prototype.sendDetachRequest = function () {
    var _this = this;
    return this.sendRequest([ DVAL_REQ, CMD_DETACH, DVAL_EOM ]);
};

Debugger.prototype.sendDumpHeapRequest = function () {
    var _this = this;

    return this.sendRequest([ DVAL_REQ, CMD_DUMPHEAP, DVAL_EOM ]).then(function (msg) {
        var res = {};
        var objs = [];
        var i, j, n, m, o, prop;

        res.type = 'heapDump';
        res.heapObjects = objs;

        for (i = 1, n = msg.length - 1; i < n; /*nop*/) {
            o = {};
            o.ptr = msg[i++];
            o.type = msg[i++];
            o.flags = msg[i++] >>> 0;  /* unsigned */
            o.refc = msg[i++];

            if (o.type === DUK_HTYPE_STRING) {
                o.blen = msg[i++];
                o.clen = msg[i++];
                o.hash = msg[i++] >>> 0;  /* unsigned */
                o.data = msg[i++];
            } else if (o.type === DUK_HTYPE_BUFFER) {
                o.len = msg[i++];
                o.data = msg[i++];
            } else if (o.type === DUK_HTYPE_OBJECT) {
                o['class'] = msg[i++];
                o.proto = msg[i++];
                o.esize = msg[i++];
                o.enext = msg[i++];
                o.asize = msg[i++];
                o.hsize = msg[i++];
                o.props = [];
                for (j = 0, m = o.enext; j < m; j++) {
                    prop = {};
                    prop.flags = msg[i++];
                    prop.key = msg[i++];
                    prop.accessor = (msg[i++] == 1);
                    if (prop.accessor) {
                        prop.getter = msg[i++];
                        prop.setter = msg[i++];
                    } else {
                        prop.value = msg[i++];
                    }
                    o.props.push(prop);
                }
                o.array = [];
                for (j = 0, m = o.asize; j < m; j++) {
                    prop = {};
                    prop.value = msg[i++];
                    o.array.push(prop);
                }
            } else {
                console.log('invalid htype: ' + o.type + ', disconnect');
                _this.disconnectDebugger();
                throw new Error('invalid htype');
                return;
            }

            objs.push(o);
        }

        return res;
    });
};

Debugger.prototype.sendGetBytecodeRequest = function () {
    var _this = this;

    return this.sendRequest([ DVAL_REQ, CMD_GETBYTECODE, -1 /* level; could be other than -1 too */, DVAL_EOM ]).then(function (msg) {
        var idx = 1;
        var nconst;
        var nfunc;
        var val;
        var buf;
        var i, n;
        var consts = [];
        var funcs = [];
        var bcode;
        var preformatted;
        var ret;
        var idxPreformattedInstructions;

        //console.log(JSON.stringify(msg));

        nconst = msg[idx++];
        for (i = 0; i < nconst; i++) {
            val = msg[idx++];
            consts.push(val);
        }

        nfunc = msg[idx++];
        for (i = 0; i < nfunc; i++) {
            val = msg[idx++];
            funcs.push(val);
        }
        val = msg[idx++];

        // Right now bytecode is a string containing a direct dump of the
        // bytecode in target endianness.  Decode here so that the web UI
        // doesn't need to.

        buf = new Buffer(val.length);
        writeDebugStringToBuffer(val, buf, 0);
        bcode = _this.decodeBytecodeFromBuffer(buf, consts, funcs);

        preformatted = [];
        consts.forEach(function (v, i) {
            preformatted.push('; c' + i + ' ' + JSON.stringify(v));
        });
        preformatted.push('');
        idxPreformattedInstructions = preformatted.length;
        bcode.forEach(function (v) {
            preformatted.push(v.str);
        });
        preformatted = preformatted.join('\n') + '\n';

        ret = {
            constants: consts,
            functions: funcs,
            bytecode: bcode,
            preformatted: preformatted,
            idxPreformattedInstructions: idxPreformattedInstructions
        };

        return ret;
    });
};

Debugger.prototype.changeBreakpoint = function (fileName, lineNumber, mode) {
    var _this = this;

    return this.sendRequest([ DVAL_REQ, CMD_LISTBREAK, DVAL_EOM ]).then(function (msg) {
        var i, n;
        var breakpts = [];
        var deleted = false;

        // Up-to-date list of breakpoints on target
        for (i = 1, n = msg.length - 1; i < n; i += 2) {
            breakpts.push({ fileName: msg[i], lineNumber: msg[i + 1] });
        }

        // Delete matching breakpoints in reverse order so that indices
        // remain valid.  We do this for all operations so that duplicates
        // are eliminated if present.
        for (i = breakpts.length - 1; i >= 0; i--) {
            var bp = breakpts[i];
            if (mode === 'deleteall' || (bp.fileName === fileName && bp.lineNumber === lineNumber)) {
                deleted = true;
                _this.sendRequest([ DVAL_REQ, CMD_DELBREAK, i, DVAL_EOM ], function (msg) {
                    // nop
                }, function (err) {
                    // nop
                });
            }
        }

        // Technically we should wait for each delbreak reply but because
        // target processes the requests in order, it doesn't matter.
        if ((mode === 'add') || (mode === 'toggle' && !deleted)) {
            _this.sendRequest([ DVAL_REQ, CMD_ADDBREAK, fileName, lineNumber, DVAL_EOM ], function (msg) {
                // nop
            }, function (err) {
                _this.uiMessage('debugger-info', 'Failed to add breakpoint: ' + err);
            });
        }

        // Read final, effective breakpoints from the target
        _this.sendBreakpointListRequest();
    });
};

Debugger.prototype.disconnectDebugger = function () {
    if (this.targetStream) {
        // We require a destroy() method from the actual target stream
        this.targetStream.destroy();
        this.targetStream = null;
    }
    if (this.inputParser) {
        this.inputParser.close();
        this.inputParser = null;
    }
    if (this.outputPassThroughStream) {
        // There is no close() or destroy() for a passthrough stream, so just
        // close the outputParser which will cancel timers etc.
    }
    if (this.outputParser) {
        this.outputParser.close();
        this.outputParser = null;
    }

    this.attached = false;
    this.handshook = false;
    this.reqQueue = null;
    this.execStatus = {
        attached: false,
        state: 'detached',
        fileName: '',
        funcName: '',
        line: 0,
        pc: 0
    };
};

Debugger.prototype.connectDebugger = function () {
    var _this = this;

    this.disconnectDebugger();  // close previous target connection

    // CUSTOMTRANSPORT: to use a custom transport, change this.targetStream to
    // use your custom transport.

    console.log('Connecting to ' + optTargetHost + ':' + optTargetPort + '...');
    this.targetStream = new net.Socket();
    this.targetStream.connect(optTargetPort, optTargetHost, function () {
        console.log('Debug transport connected');
        _this.attached = true;
        _this.reqQueue = [];
        _this.uiMessage('debugger-info', 'Debug transport connected');
    });

    this.inputParser = new DebugProtocolParser(
        this.targetStream,
        null,
        optDumpDebugRead,
        optDumpDebugPretty,
        optDumpDebugPretty ? 'Recv: ' : null,
        null,
        null   // console logging is done at a higher level to match request/response
    );

    // Use a PassThrough stream to debug dump and get stats for output messages.
    // Simply write outgoing data to both the targetStream and this passthrough
    // separately.
    this.outputPassThroughStream = stream.PassThrough();
    this.outputParser = new DebugProtocolParser(
        this.outputPassThroughStream,
        1,
        optDumpDebugWrite,
        optDumpDebugPretty,
        optDumpDebugPretty ? 'Send: ' : null,
        null,
        null   // console logging is done at a higher level to match request/response
    );

    this.inputParser.on('transport-close', function () {
        _this.uiMessage('debugger-info', 'Debug transport closed');
        _this.disconnectDebugger();
        _this.emit('exec-status-update');
        _this.emit('detached');
    });

    this.inputParser.on('transport-error', function (err) {
        _this.uiMessage('debugger-info', 'Debug transport error: ' + err);
        _this.disconnectDebugger();
    });

    this.inputParser.on('protocol-version', function (msg) {
        var ver = msg.protocolVersion;
        console.log('Debug version identification:', msg.versionIdentification);
        _this.protocolVersion = ver;
        _this.uiMessage('debugger-info', 'Debug version identification: ' + msg.versionIdentification);
        if (ver !== SUPPORTED_DEBUG_PROTOCOL_VERSION) {
            console.log('Unsupported debug protocol version (got ' + ver + ', support ' + SUPPORTED_DEBUG_PROTOCOL_VERSION + ')');
            _this.uiMessage('debugger-info', 'Protocol version ' + ver + ' unsupported, dropping connection');
            _this.targetStream.destroy();
        } else {
            _this.uiMessage('debugger-info', 'Debug protocol version: ' + ver);
            _this.handshook = true;
            _this.execStatus = {
                attached: true,
                state: 'attached',
                fileName: '',
                funcName: '',
                line: 0,
                pc: 0
            };
            _this.emit('exec-status-update');
            _this.emit('attached');  // inform web UI

            // Fetch basic info right away
            _this.sendBasicInfoRequest();
        }
    });

    this.inputParser.on('debug-message', function (msg) {
        _this.processDebugMessage(msg);
    });

    this.inputParser.on('stats-update', function () {
        _this.stats.rxBytes = this.bytes;
        _this.stats.rxDvalues = this.dvalues;
        _this.stats.rxMessages = this.messages;
        _this.stats.rxBytesPerSec = this.bytesPerSec;
        _this.emit('debug-stats-update');
    });

    this.outputParser.on('stats-update', function () {
        _this.stats.txBytes = this.bytes;
        _this.stats.txDvalues = this.dvalues;
        _this.stats.txMessages = this.messages;
        _this.stats.txBytesPerSec = this.bytesPerSec;
        _this.emit('debug-stats-update');
    });
};

Debugger.prototype.processDebugMessage = function (msg) {
    var req;
    var prevState, newState;
    var err;

    if (msg[0] === DVAL_REQ) {
        // No actual requests sent by the target right now (just notifys).
        console.log('Unsolicited reply message, dropping connection: ' + prettyDebugMessage(msg));
    } else if (msg[0] === DVAL_REP) {
        if (this.reqQueue.length <= 0) {
            console.log('Unsolicited reply message, dropping connection: ' + prettyDebugMessage(msg));
            this.targetStream.destroy();
        }
        req = this.reqQueue.shift();

        if (optLogMessages) {
            console.log('Reply for ' + prettyDebugCommand(req.reqCmd) + ': ' + prettyDebugMessage(msg));
        }

        if (req.resolveCb) {
            req.resolveCb(msg);
        } else {
            // nop: no callback
        }
    } else if (msg[0] === DVAL_ERR) {
        if (this.reqQueue.length <= 0) {
            console.log('Unsolicited error message, dropping connection: ' + prettyDebugMessage(msg));
            this.targetStream.destroy();
        }
        err = new Error(String(msg[2]) + ' (code ' + String(msg[1]) + ')');
        err.errorCode = msg[1] || 0;
        req = this.reqQueue.shift();

        if (optLogMessages) {
            console.log('Error for ' + prettyDebugCommand(req.reqCmd) + ': ' + prettyDebugMessage(msg));
        }

        if (req.rejectCb) {
            req.rejectCb(err);
        } else {
            // nop: no callback
        }
    } else if (msg[0] === DVAL_NFY) {
        if (optLogMessages) {
            console.log('Notify ' + prettyDebugCommand(msg[1]) + ': ' + prettyDebugMessage(msg));
        }

        if (msg[1] === CMD_STATUS) {
            prevState = this.execStatus.state;
            newState = msg[2] === 0 ? 'running' : 'paused';
            this.execStatus = {
                attached: true,
                state: newState,
                fileName: msg[3],
                funcName: msg[4],
                line: msg[5],
                pc: msg[6]
            };

            if (prevState !== newState && newState === 'paused') {
                // update run state now that we're paused
                this.sendBreakpointListRequest();
                this.sendGetLocalsRequest();
                this.sendGetCallStackRequest();
            }

            this.emit('exec-status-update');
        } else if (msg[1] === CMD_THROW) {
            this.uiMessage({ type: 'throw', fatal: msg[2], message: (msg[2] ? 'UNCAUGHT: ' : 'THROW: ') + prettyUiStringUnquoted(msg[3], UI_MESSAGE_CLIPLEN), fileName: msg[4], lineNumber: msg[5] });
        } else if (msg[1] === CMD_DETACHING) {
            this.uiMessage({ type: 'detaching', reason: msg[2], message: 'DETACH: ' + (msg.length >= 5 ? prettyUiStringUnquoted(msg[3]) : 'detaching') });
        } else {
            // Ignore unknown notify messages
            console.log('Unknown notify, ignoring: ' + prettyDebugMessage(msg));

            //this.targetStream.destroy();
        }
    } else {
        console.log('Invalid initial dvalue, dropping connection: ' + prettyDebugMessage(msg));
        this.targetStream.destroy();
    }
};

Debugger.prototype.run = function () {
    var _this = this;

    // Initial debugger connection

    this.connectDebugger();

    // Poll various state items when running

    var sendRound = 0;
    var statusPending = false;
    var bplistPending = false;
    var localsPending = false;
    var callStackPending = false;

    setInterval(function () {
        if (_this.execStatus.state !== 'running') {
            return;
        }

        // Could also check for an empty request queue, but that's probably
        // too strict?

        // Pending flags are used to avoid requesting the same thing twice
        // while a previous request is pending.  The flag-based approach is
        // quite awkward.  Rework to use promises.

        switch (sendRound) {
        case 0:
            if (!statusPending) {
                statusPending = true;
                _this.sendStatusRequest().finally(function () { statusPending = false; });
            }
            break;
        case 1:
            if (!bplistPending) {
                bplistPending = true;
                _this.sendBreakpointListRequest().finally(function () { bplistPending = false; });
            }
            break;
        case 2:
            if (!localsPending) {
                localsPending = true;
                _this.sendGetLocalsRequest().finally(function () { localsPending = false; });
            }
            break;
        case 3:
            if (!callStackPending) {
                callStackPending = true;
                _this.sendGetCallStackRequest().finally(function () { callStackPending = false; });
            }
            break;
        }
        sendRound = (sendRound + 1) % 4;
    }, 500);
};

/*
 *  Express setup and socket.io
 */

function DebugWebServer() {
    this.dbg = null;       // debugger singleton
    this.socket = null;    // current socket (or null)
    this.keepaliveTimer = null;
    this.uiMessageLimiter = null;
    this.cachedJson = {};  // cache to avoid resending identical data
    this.sourceFileManager = new SourceFileManager(optSourceSearchDirs);
    this.sourceFileManager.scan();
}

DebugWebServer.prototype.handleSourcePost = function (req, res) {
    var fileName = req.body && req.body.fileName;
    var fileData;

    console.log('Source request: ' + fileName);

    if (typeof fileName !== 'string') {
        res.status(500).send('invalid request');
        return;
    }
    fileData = this.sourceFileManager.search(fileName, optSourceSearchDirs);
    if (typeof fileData !== 'string') {
        res.status(404).send('not found');
        return;
    }
    res.status(200).send(fileData);  // UTF-8
};

DebugWebServer.prototype.handleSourceListPost = function (req, res) {
    console.log('Source list request');

    var files = this.sourceFileManager.getFiles();
    res.header('Content-Type', 'application/json');
    res.status(200).json(files);
};

DebugWebServer.prototype.handleHeapDumpGet = function (req, res) {
    console.log('Heap dump get');

    this.dbg.sendDumpHeapRequest().then(function (val) {
        res.header('Content-Type', 'application/json');
        //res.status(200).json(val);
        res.status(200).send(JSON.stringify(val, null, 4));
    }).catch(function (err) {
        res.status(500).send('Failed to get heap dump: ' + (err.stack || err));
    });
};

DebugWebServer.prototype.run = function () {
    var _this = this;

    var express = require('express');
    var bodyParser = require('body-parser');
    var app = express();
    var http = require('http').Server(app);
    var io = require('socket.io')(http);

    app.use(bodyParser.json());
    app.post('/source', this.handleSourcePost.bind(this));
    app.post('/sourceList', this.handleSourceListPost.bind(this));
    app.get('/heapDump.json', this.handleHeapDumpGet.bind(this));
    app.use('/', express.static(__dirname + '/static'));

    http.listen(optHttpPort, function () {
        console.log('Listening on *:' + optHttpPort);
    });

    io.on('connection', this.handleNewSocketIoConnection.bind(this));

    this.dbg.on('attached', function () {
        console.log('Debugger attached');
    });

    this.dbg.on('detached', function () {
        console.log('Debugger detached');
    });

    this.dbg.on('debug-stats-update', function () {
        _this.debugStatsLimiter.trigger();
    });

    this.dbg.on('ui-message-update', function () {
        // Explicit rate limiter because this is a source of a lot of traffic.
        _this.uiMessageLimiter.trigger();
    });

    this.dbg.on('basic-info-update', function () {
        _this.emitBasicInfo();
    });

    this.dbg.on('breakpoints-update', function () {
        _this.emitBreakpoints();
    });

    this.dbg.on('exec-status-update', function () {
        // Explicit rate limiter because this is a source of a lot of traffic.
        _this.execStatusLimiter.trigger();
    });

    this.dbg.on('locals-update', function () {
        _this.emitLocals();
    });

    this.dbg.on('callstack-update', function () {
        _this.emitCallStack();
    });

    this.uiMessageLimiter = new RateLimited(10, 1000, this.uiMessageLimiterCallback.bind(this));
    this.execStatusLimiter = new RateLimited(50, 500, this.execStatusLimiterCallback.bind(this));
    this.debugStatsLimiter = new RateLimited(1, 2000, this.debugStatsLimiterCallback.bind(this));

    this.keepaliveTimer = setInterval(this.emitKeepalive.bind(this), 30000);
};

DebugWebServer.prototype.handleNewSocketIoConnection = function (socket) {
    var _this = this;

    console.log('Socket.io connected');
    if (this.socket) {
        console.log('Closing previous socket.io socket');
        this.socket.emit('replaced');
    }
    this.socket = socket;

    this.emitKeepalive();

    socket.on('disconnect', function () {
        console.log('Socket.io disconnected');
        if (_this.socket === socket) {
             _this.socket = null;
        }
    });

    socket.on('keepalive', function (msg) {
        // nop
    });

    socket.on('attach', function (msg) {
        if (_this.dbg.targetStream) {
            console.log('Attach request when debugger already has a connection, ignoring');
        } else {
            _this.dbg.connectDebugger();
        }
    });

    socket.on('detach', function (msg) {
        // Try to detach cleanly, timeout if no response
        Promise.any([
            _this.dbg.sendDetachRequest(),
            Promise.delay(3000)
        ]).finally(function () {
            _this.dbg.disconnectDebugger();
        });
    });

    socket.on('stepinto', function (msg) {
        _this.dbg.sendStepIntoRequest();
    });

    socket.on('stepover', function (msg) {
        _this.dbg.sendStepOverRequest();
    });

    socket.on('stepout', function (msg) {
        _this.dbg.sendStepOutRequest();
    });

    socket.on('pause', function (msg) {
        _this.dbg.sendPauseRequest();
    });

    socket.on('resume', function (msg) {
        _this.dbg.sendResumeRequest();
    });

    socket.on('eval', function (msg) {
        // msg.input is a proper Unicode strings here, and needs to be
        // converted into a protocol string (U+0000...U+00FF).
        var input = stringToDebugString(msg.input);
        _this.dbg.sendEvalRequest(input, msg.level).then(function (v) {
            socket.emit('eval-result', { error: v.error, result: prettyUiDebugValue(v.value, EVAL_CLIPLEN) });
        });

        // An eval call quite possibly changes the local variables so always
        // re-read locals afterwards.  We don't need to wait for Eval to
        // complete here; the requests will pipeline automatically and be
        // executed in order.

        // XXX: move this to the web UI so that the UI can control what
        // locals are listed (or perhaps show locals for all levels with
        // an expandable tree view).
        _this.dbg.sendGetLocalsRequest();
    });

    socket.on('getvar', function (msg) {
        // msg.varname is a proper Unicode strings here, and needs to be
        // converted into a protocol string (U+0000...U+00FF).
        var varname = stringToDebugString(msg.varname);
        _this.dbg.sendGetVarRequest(varname, msg.level)
        .then(function (v) {
            socket.emit('getvar-result', { found: v.found, result: prettyUiDebugValue(v.value, GETVAR_CLIPLEN) });
        });
    });

    socket.on('putvar', function (msg) {
        // msg.varname and msg.varvalue are proper Unicode strings here, they
        // need to be converted into protocol strings (U+0000...U+00FF).
        var varname = stringToDebugString(msg.varname);
        var varvalue = msg.varvalue;

        // varvalue is JSON parsed by the web UI for now, need special string
        // encoding here.
        if (typeof varvalue === 'string') {
            varvalue = stringToDebugString(msg.varvalue);
        }

        _this.dbg.sendPutVarRequest(varname, varvalue, msg.level)
        .then(function (v) {
            console.log('putvar done');  // XXX: signal success to UI?
        });

        // A PutVar call quite possibly changes the local variables so always
        // re-read locals afterwards.  We don't need to wait for PutVar to
        // complete here; the requests will pipeline automatically and be
        // executed in order.

        // XXX: make the client do this?
        _this.dbg.sendGetLocalsRequest();
    });

    socket.on('add-breakpoint', function (msg) {
        _this.dbg.changeBreakpoint(msg.fileName, msg.lineNumber, 'add');
    });

    socket.on('delete-breakpoint', function (msg) {
        _this.dbg.changeBreakpoint(msg.fileName, msg.lineNumber, 'delete');
    });

    socket.on('toggle-breakpoint', function (msg) {
        _this.dbg.changeBreakpoint(msg.fileName, msg.lineNumber, 'toggle');
    });

    socket.on('delete-all-breakpoints', function (msg) {
        _this.dbg.changeBreakpoint(null, null, 'deleteall');
    });

    socket.on('get-bytecode', function (msg) {
        _this.dbg.sendGetBytecodeRequest().then(function (res) {
            socket.emit('bytecode', res);
        });
    });

    // Resend all debugger state for new client
    this.cachedJson = {};  // clear client state cache
    this.emitBasicInfo();
    this.emitStats();
    this.emitExecStatus();
    this.emitUiMessages();
    this.emitBreakpoints();
    this.emitCallStack();
    this.emitLocals();
};

// Check if 'msg' would encode to the same JSON which was previously sent
// to the web client.  The caller then avoid resending unnecessary stuff.
DebugWebServer.prototype.cachedJsonCheck = function (cacheKey, msg) {
    var newJson = JSON.stringify(msg);
    if (this.cachedJson[cacheKey] === newJson) {
        return true;  // cached
    }
    this.cachedJson[cacheKey] = newJson;
    return false;  // not cached, send (cache already updated)
};

DebugWebServer.prototype.uiMessageLimiterCallback = function () {
    this.emitUiMessages();
};

DebugWebServer.prototype.execStatusLimiterCallback = function () {
    this.emitExecStatus();
};

DebugWebServer.prototype.debugStatsLimiterCallback = function () {
    this.emitStats();
};

DebugWebServer.prototype.emitKeepalive = function () {
    if (!this.socket) { return; }

    this.socket.emit('keepalive', { nodeVersion: process.version });
};

DebugWebServer.prototype.emitBasicInfo = function () {
    if (!this.socket) { return; }

    var newMsg = {
        duk_version: this.dbg.dukVersion,
        duk_git_describe: this.dbg.dukGitDescribe,
        target_info: this.dbg.targetInfo,
        endianness: this.dbg.endianness
    };
    if (this.cachedJsonCheck('basic-info', newMsg)) {
        return;
    }
    this.socket.emit('basic-info', newMsg);
};

DebugWebServer.prototype.emitStats = function () {
    if (!this.socket) { return; }

    this.socket.emit('debug-stats', this.dbg.stats);
};

DebugWebServer.prototype.emitExecStatus = function () {
    if (!this.socket) { return; }

    var newMsg = this.dbg.execStatus;
    if (this.cachedJsonCheck('exec-status', newMsg)) {
        return;
    }
    this.socket.emit('exec-status', newMsg);
};

DebugWebServer.prototype.emitUiMessages = function () {
    if (!this.socket) { return; }

    var newMsg = this.dbg.messageLines;
    if (this.cachedJsonCheck('output-lines', newMsg)) {
        return;
    }
    this.socket.emit('output-lines', newMsg);
};

DebugWebServer.prototype.emitBreakpoints = function () {
    if (!this.socket) { return; }

    var newMsg = { breakpoints: this.dbg.breakpoints };
    if (this.cachedJsonCheck('breakpoints', newMsg)) {
        return;
    }
    this.socket.emit('breakpoints', newMsg);
};

DebugWebServer.prototype.emitCallStack = function () {
    if (!this.socket) { return; }

    var newMsg = { callstack: this.dbg.callstack };
    if (this.cachedJsonCheck('callstack', newMsg)) {
        return;
    }
    this.socket.emit('callstack', newMsg);
};

DebugWebServer.prototype.emitLocals = function () {
    if (!this.socket) { return; }

    var newMsg = { locals: this.dbg.locals };
    if (this.cachedJsonCheck('locals', newMsg)) {
        return;
    }
    this.socket.emit('locals', newMsg);
};

/*
 *  JSON debug proxy
 */

function DebugProxy(serverPort) {
    this.serverPort = serverPort;
    this.server = null;
    this.socket = null;
    this.targetStream = null;
    this.inputParser = null;

    // preformatted dvalues
    this.dval_eom = formatDebugValue(DVAL_EOM);
    this.dval_req = formatDebugValue(DVAL_REQ);
    this.dval_rep = formatDebugValue(DVAL_REP);
    this.dval_nfy = formatDebugValue(DVAL_NFY);
    this.dval_err = formatDebugValue(DVAL_ERR);
}

DebugProxy.prototype.determineCommandNumber = function (cmdName, cmdNumber) {
    var ret;
    if (typeof cmdName === 'string') {
        ret = debugCommandNumbers[cmdName];
    } else if (typeof cmdName === 'number') {
        ret = cmdName;
    }
    ret = ret || cmdNumber;
    if (typeof ret !== 'number') {
        throw Error('cannot figure out command number for "' + cmdName + '" (' + cmdNumber + ')');
    }
    return ret;
};

DebugProxy.prototype.commandNumberToString = function (id) {
    return debugCommandNames[id] || String(id);
};

DebugProxy.prototype.formatDvalues = function (args) {
    if (!args) {
        return [];
    }
    return args.map(function (v) {
        return formatDebugValue(v);
    });
};

DebugProxy.prototype.writeJson = function (val) {
    this.socket.write(JSON.stringify(val) + '\n');
};

DebugProxy.prototype.writeJsonSafe = function (val) {
    try {
        this.writeJson(val);
    } catch (e) {
        console.log('Failed to write JSON in writeJsonSafe, ignoring: ' + e);
    }
};

DebugProxy.prototype.disconnectJsonClient = function () {
    if (this.socket) {
        this.socket.destroy();
        this.socket = null;
    }
};

DebugProxy.prototype.disconnectTarget = function () {
    if (this.inputParser) {
        this.inputParser.close();
        this.inputParser = null;
    }
    if (this.targetStream) {
        this.targetStream.destroy();
        this.targetStream = null;
    }
};

DebugProxy.prototype.run = function () {
    var _this = this;

    console.log('Waiting for client connections on port ' + this.serverPort);
    this.server = net.createServer(function (socket) {
        console.log('JSON proxy client connected');

        _this.disconnectJsonClient();
        _this.disconnectTarget();

        // A byline-parser is simple and good enough for now (assume
        // compact JSON with no newlines).
        var socketByline = byline(socket);
        _this.socket = socket;

        socketByline.on('data', function (line) {
            try {
                // console.log('Received json proxy input line: ' + line.toString('utf8'));
                var msg = JSON.parse(line.toString('utf8'));
                var first_dval;
                var args_dvalues = _this.formatDvalues(msg.args);
                var last_dval = _this.dval_eom;
                var cmd;

                if (msg.request) {
                    // "request" can be a string or "true"
                    first_dval = _this.dval_req;
                    cmd = _this.determineCommandNumber(msg.request, msg.command);
                } else if (msg.reply) {
                    first_dval = _this.dval_rep;
                } else if (msg.notify) {
                    // "notify" can be a string or "true"
                    first_dval = _this.dval_nfy;
                    cmd = _this.determineCommandNumber(msg.notify, msg.command);
                } else if (msg.error) {
                    first_dval = _this.dval_err;
                } else {
                    throw new Error('Invalid input JSON message: ' + JSON.stringify(msg));
                }

                _this.targetStream.write(first_dval);
                if (cmd) {
                    _this.targetStream.write(formatDebugValue(cmd));
                }
                args_dvalues.forEach(function (v) {
                    _this.targetStream.write(v);
                });
                _this.targetStream.write(last_dval);
            } catch (e) {
                console.log(e);

                _this.writeJsonSafe({
                    notify: '_Error',
                    args: [ 'Failed to handle input json message: ' + e ]
                });

                _this.disconnectJsonClient();
                _this.disconnectTarget();
            }
        });

        _this.connectToTarget();
    }).listen(this.serverPort);
};

DebugProxy.prototype.connectToTarget = function () {
    var _this = this;

    console.log('Connecting to ' + optTargetHost + ':' + optTargetPort + '...');
    this.targetStream = new net.Socket();
    this.targetStream.connect(optTargetPort, optTargetHost, function () {
        console.log('Debug transport connected');
    });

    this.inputParser = new DebugProtocolParser(
        this.targetStream,
        null,
        optDumpDebugRead,
        optDumpDebugPretty,
        optDumpDebugPretty ? 'Recv: ' : null,
        null,
        null   // console logging is done at a higher level to match request/response
    );

    // Don't add a 'value' key to numbers.
    this.inputParser.readableNumberValue = false;

    this.inputParser.on('transport-close', function () {
        console.log('Debug transport closed');

        _this.writeJsonSafe({
            notify: '_Disconnecting'
        });

        _this.disconnectJsonClient();
        _this.disconnectTarget();
    });

    this.inputParser.on('transport-error', function (err) {
        console.log('Debug transport error', err);

        _this.writeJsonSafe({
            notify: '_Error',
            args: [ String(err) ]
        });
    });

    this.inputParser.on('protocol-version', function (msg) {
        var ver = msg.protocolVersion;
        console.log('Debug version identification:', msg.versionIdentification);

        _this.writeJson({
            notify: '_TargetConnected',
            args: [ msg.versionIdentification ]  // raw identification string
        });

        if (ver !== 1) {
            console.log('Protocol version ' + ver + ' unsupported, dropping connection');
        }
    });

    this.inputParser.on('debug-message', function (msg) {
        var t;

        //console.log(msg);

        if (typeof msg[0] !== 'object' || msg[0] === null) {
            throw new Error('unexpected initial dvalue: ' + msg[0]);
        } else if (msg[0].type === 'eom') {
            throw new Error('unexpected initial dvalue: ' + msg[0]);
        } else if (msg[0].type === 'req') {
            if (typeof msg[1] !== 'number') {
                throw new Error('unexpected request command number: ' + msg[1]);
            }
            t = {
                request: _this.commandNumberToString(msg[1]),
                command: msg[1],
                args: msg.slice(2, msg.length - 1)
            };
            _this.writeJson(t);
        } else if (msg[0].type === 'rep') {
            t = {
                reply: true,
                args: msg.slice(1, msg.length - 1)
            };
            _this.writeJson(t);
        } else if (msg[0].type === 'err') {
            t = {
                error: true,
                args: msg.slice(1, msg.length - 1)
            };
            _this.writeJson(t);
        } else if (msg[0].type === 'nfy') {
            if (typeof msg[1] !== 'number') {
                throw new Error('unexpected notify command number: ' + msg[1]);
            }
            t = {
                notify: _this.commandNumberToString(msg[1]),
                command: msg[1],
                args: msg.slice(2, msg.length - 1)
            };
            _this.writeJson(t);
        } else {
            throw new Error('unexpected initial dvalue: ' + msg[0]);
        }
    });

    this.inputParser.on('stats-update', function () {
    });
};

/*
 *  Command line parsing and initialization
 */

function main() {
    console.log('((o) Duktape debugger');

    // Parse arguments.

    var argv = require('minimist')(process.argv.slice(2));
    //console.dir(argv);
    if (argv['target-host']) {
        optTargetHost = argv['target-host'];
    }
    if (argv['target-port']) {
        optTargetPort = argv['target-port'];
    }
    if (argv['http-port']) {
        optHttpPort = argv['http-port'];
    }
    if (argv['json-proxy-port']) {
        optJsonProxyPort = argv['json-proxy-port'];
    }
    if (argv['json-proxy']) {
        optJsonProxy = argv['json-proxy'];
    }
    if (argv['source-dirs']) {
        optSourceSearchDirs = argv['source-dirs'].split(path.delimiter);
    }
    if (argv['dump-debug-read']) {
        optDumpDebugRead = argv['dump-debug-read'];
    }
    if (argv['dump-debug-write']) {
        optDumpDebugWrite = argv['dump-debug-write'];
    }
    if (argv['dump-debug-pretty']) {
        optDumpDebugPretty = argv['dump-debug-pretty'];
    }
    if (argv['log-messages']) {
        optLogMessages = true;
    }

    // Dump effective options.  Also provides a list of option names.

    console.log('');
    console.log('Effective options:');
    console.log('  --target-host:       ' + optTargetHost);
    console.log('  --target-port:       ' + optTargetPort);
    console.log('  --http-port:         ' + optHttpPort);
    console.log('  --json-proxy-port:   ' + optJsonProxyPort);
    console.log('  --json-proxy:        ' + optJsonProxy);
    console.log('  --source-dirs:       ' + optSourceSearchDirs.join(' '));
    console.log('  --dump-debug-read:   ' + optDumpDebugRead);
    console.log('  --dump-debug-write:  ' + optDumpDebugWrite);
    console.log('  --dump-debug-pretty: ' + optDumpDebugPretty);
    console.log('  --log-messages:      ' + optLogMessages);
    console.log('');

    // Create debugger and web UI singletons, tie them together and
    // start them.

    if (optJsonProxy) {
        console.log('Starting in JSON proxy mode, JSON port: ' + optJsonProxyPort);

        var prx = new DebugProxy(optJsonProxyPort);
        prx.run();
    } else {
        var dbg = new Debugger();
        var web = new DebugWebServer();
        dbg.web = web;
        web.dbg = dbg;
        dbg.run();
        web.run();
    }
}

main();
