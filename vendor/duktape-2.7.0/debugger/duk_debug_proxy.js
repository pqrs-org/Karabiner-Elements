/*
 *  JSON debug proxy written in DukLuv
 *
 *  This single file JSON debug proxy implementation is an alternative to the
 *  Node.js-based proxy in duk_debug.js.  DukLuv is a much smaller dependency
 *  than Node.js so embedding DukLuv in a debug client is easier.
 */

'use strict';

// XXX: Code assumes uv.write() will write fully.  This is not necessarily
// true; should add support for partial writes (or at least failing when
// a partial write occurs).

var log = new Duktape.Logger('Proxy');  // default logger
//log.l = 0;  // enable debug and trace logging

/*
 *  Config
 */

var serverHost = '0.0.0.0';
var serverPort = 9093;
var targetHost = '127.0.0.1';
var targetPort = 9091;
var singleConnection = false;
var readableNumberValue = false;
var lenientJsonParse = false;
var jxParse = false;
var metadataFile = null;
var metadata = {};
var TORTURE = false;  // for manual testing of binary/json parsing robustness

/*
 *  Duktape 1.x and 2.x buffer harmonization
 */

var allocPlain = (typeof Uint8Array.allocPlain === 'function' ?
                  Uint8Array.allocPlain : Duktape.Buffer);
var plainOf = (typeof Uint8Array.plainOf === 'function' ?
               Uint8Array.plainOf : Duktape.Buffer);
var bufferToString = (typeof String.fromBuffer === 'function' ?
                      String.fromBuffer : String);

/*
 *  Detect missing 'var' declarations
 */

// Prevent new bindings on global object.  This detects missing 'var'
// declarations, e.g. "x = 123;" in a function without declaring it.
var global = new Function('return this;')();
log.debug('Preventing extensions on global object');
log.debug('Global is extensible:', Object.isExtensible(global));
Object.preventExtensions(global);
log.debug('Global is extensible:', Object.isExtensible(global));

/*
 *  Misc helpers
 */

function jxEncode(v) {
    return Duktape.enc('jx', v);
}

function plainBufferCopy(typedarray) {
    // This is still pretty awkward in Duktape 1.4.x.
    // Argument may be a "slice" and we want a copy of the slice
    // (not the full underlying buffer).

    var u8 = new Uint8Array(typedarray.length);
    u8.set(typedarray);  // make a copy, ensuring there's no slice offset
    return plainOf(u8);  // get underlying plain buffer
}

function isObject(x) {
    // Note that typeof null === 'object'.
    return (typeof x === 'object' && x !== null);
}

function readFully(filename, cb) {
    uv.fs_open(metadataFile, 'r', 0, function (handle, err) {
        var fileOff = 0;
        var data = new Uint8Array(256);
        var dataOff = 0;

        if (err) {
            return cb(null, err);
        }
        function readCb(buf, err) {
            var res;
            var newData;

            log.debug('Read callback:', buf.length, err);
            if (err) {
                uv.fs_close(handle);
                return cb(null, err);
            }
            if (buf.length == 0) {
                uv.fs_close(handle);
                res = new Uint8Array(dataOff);
                res.set(data.subarray(0, dataOff));
                res = plainOf(res);  // plain buffer
                log.debug('Read', res.length, 'bytes from', filename);
                return cb(res, null);
            }
            while (data.length - dataOff < buf.length) {
                log.debug('Resize file read buffer:', data.length, '->', data.length * 2);
                newData = new Uint8Array(data.length * 2);
                newData.set(data);
                data = newData;
            }
            data.set(new Uint8Array(buf), dataOff);
            dataOff += buf.length;
            fileOff += buf.length;
            uv.fs_read(handle, 4096, fileOff, readCb);
        }
        uv.fs_read(handle, 4096, fileOff, readCb);
    });
}

/*
 *  JSON proxy server
 *
 *  Accepts an incoming JSON proxy client and connects to a debug target,
 *  tying the two connections together.  Supports both a single connection
 *  and a persistent mode.
 */

function JsonProxyServer(host, port) {
    this.name = 'JsonProxyServer';
    this.handle = uv.new_tcp();
    uv.tcp_bind(this.handle, host, port);
    uv.listen(this.handle, 128, this.onConnection.bind(this));
}

JsonProxyServer.prototype.onConnection = function onConnection(err) {
    if (err) {
        log.error('JSON proxy onConnection error:', err);
        return;
    }
    log.info('JSON proxy client connected');  // XXX: it'd be nice to log remote peer host:port

    var jsonSock = new JsonConnHandler(this);
    var targSock = new TargetConnHandler(this);
    jsonSock.targetHandler = targSock;
    targSock.jsonHandler = jsonSock;
    uv.accept(this.handle, jsonSock.handle);

    log.info('Connecting to debug target at', targetHost + ':' + targetPort);
    jsonSock.writeJson({ notify: '_TargetConnecting', args: [ targetHost, targetPort ] });
    uv.tcp_connect(targSock.handle, targetHost, targetPort, targSock.onConnect.bind(targSock));

    if (singleConnection) {
        log.info('Single connection mode, stop listening for more connections');
        uv.shutdown(this.handle);
        uv.read_stop(this.handle);  // unnecessary but just in case
        uv.close(this.handle);
        this.handle = null;
    }
};

JsonProxyServer.prototype.onProxyClientDisconnected = function onProxyClientDisconnected() {
    // When this is invoked the proxy connection and the target connection
    // have both been closed.
    if (singleConnection) {
        log.info('Proxy connection finished (single connection mode: we should be exiting now)');
    } else {
        log.info('Proxy connection finished (persistent mode: wait for more connections)');
    }
};

/*
 *  JSON connection handler
 */

function JsonConnHandler(server) {
    var i, n;

    this.name = 'JsonConnHandler';
    this.server = server;
    this.handle = uv.new_tcp();
    this.incoming = new Uint8Array(4096);
    this.incomingOffset = 0;
    this.targetHandler = null;

    this.commandNumberLookup = {};
    if (metadata && metadata.target_commands) {
        for (i = 0, n = metadata.target_commands.length; i < n; i++) {
            this.commandNumberLookup[metadata.target_commands[i]] = i;
        }
    }
}

JsonConnHandler.prototype.finish = function finish(msg) {
    var args;

    if (!this.handle) {
        log.info('JsonConnHandler already disconnected, ignore finish()');
        return;
    }
    log.info('JsonConnHandler finished:', msg);
    try {
        args = msg ? [ msg ] : void 0;
        this.writeJson({ notify: '_Disconnecting', args: args });
    } catch (e) {
        log.info('Failed to write _Disconnecting notify, ignoring:', e);
    }
    uv.shutdown(this.handle);
    uv.read_stop(this.handle);
    uv.close(this.handle);
    this.handle = null;

    this.targetHandler.finish(msg);  // disconnect target too (if not already disconnected)

    this.server.onProxyClientDisconnected();
};

JsonConnHandler.prototype.onRead = function onRead(err, data) {
    var newIncoming;
    var msg;
    var errmsg;
    var tmpBuf;

    log.trace('Received data from JSON socket, err:', err, 'data length:', data ? data.length : 'null');

    if (err) {
        errmsg = 'Error reading data from JSON debug client: ' + err;
        this.finish(errmsg);
        return;
    }
    if (data) {
        // Feed the data one byte at a time when torture testing.
        if (TORTURE && data.length > 1) {
            for (var i = 0; i < data.length; i++) {
                tmpBuf = allocPlain(1);
                tmpBuf[0] = data[i];
                this.onRead(null, tmpBuf);
            }
            return;
        }

        // Receive data into 'incoming', resizing as necessary.
        while (data.length > this.incoming.length - this.incomingOffset) {
            newIncoming = new Uint8Array(this.incoming.length * 1.3 + 16);
            newIncoming.set(this.incoming);
            this.incoming = newIncoming;
            log.debug('Resize incoming JSON buffer to ' + this.incoming.length);
        }
        this.incoming.set(new Uint8Array(data), this.incomingOffset);
        this.incomingOffset += data.length;

        // Trial parse JSON message(s).
        while (true) {
            msg = this.trialParseJsonMessage();
            if (!msg) {
                break;
            }
            try {
                this.dispatchJsonMessage(msg);
            } catch (e) {
                errmsg = 'JSON message dispatch failed: ' + e;
                this.writeJson({ notify: '_Error', args: [ errmsg ] });
                if (lenientJsonParse) {
                    log.warn('JSON message dispatch failed (lenient mode, ignoring):', e);
                } else {
                    log.warn('JSON message dispatch failed (dropping connection):', e);
                    this.finish(errmsg);
                }
            }
        }
    } else {
        this.finish('JSON proxy client disconnected');
    }
};

JsonConnHandler.prototype.writeJson = function writeJson(msg) {
    log.info('PROXY --> CLIENT:', JSON.stringify(msg));
    if (this.handle) {
        uv.write(this.handle, JSON.stringify(msg) + '\n');
    }
};

JsonConnHandler.prototype.handleDebugMessage = function handleDebugMessage(dvalues) {
    var msg = {};
    var idx = 0;
    var cmd;

    if (dvalues.length <= 0) {
        throw new Error('invalid dvalues list: length <= 0');
    }
    var x = dvalues[idx++];
    if (!isObject(x)) {
        throw new Error('invalid initial dvalue: ' + Duktape.enc('jx', dvalues));
    }
    if (x.type === 'req') {
        cmd = dvalues[idx++];
        if (typeof cmd !== 'number') {
            throw new Error('invalid command: ' + Duktape.enc('jx', cmd));
        }
        msg.request = this.determineCommandName(cmd) || true;
        msg.command = cmd;
    } else if (x.type === 'rep') {
        msg.reply = true;
    } else if (x.type === 'err') {
        msg.error = true;
    } else if (x.type === 'nfy') {
        cmd = dvalues[idx++];
        if (typeof cmd !== 'number') {
            throw new Error('invalid command: ' + Duktape.enc('jx', cmd));
        }
        msg.notify = this.determineCommandName(cmd) || true;
        msg.command = cmd;
    } else {
        throw new Error('invalid initial dvalue: ' + Duktape.enc('jx', dvalues));
    }

    for (; idx < dvalues.length - 1; idx++) {
        if (!msg.args) {
            msg.args = [];
        }
        msg.args.push(dvalues[idx]);
    }

    if (!isObject(dvalues[idx]) || dvalues[idx].type !== 'eom') {
        throw new Error('invalid final dvalue: ' + Duktape.enc('jx', dvalues));
    }

    this.writeJson(msg);
};

JsonConnHandler.prototype.determineCommandName = function determineCommandName(cmd) {
    if (!(metadata && metadata.client_commands)) {
        return;
    }
    return metadata.client_commands[cmd];
};

JsonConnHandler.prototype.trialParseJsonMessage = function trialParseJsonMessage() {
    var buf = this.incoming;
    var avail = this.incomingOffset;
    var i;
    var msg, str, errmsg;

    for (i = 0; i < avail; i++) {
        if (buf[i] == 0x0a) {
            str = bufferToString(plainBufferCopy(buf.subarray(0, i)));
            try {
                if (jxParse) {
                    msg = Duktape.dec('jx', str);
                } else {
                    msg = JSON.parse(str);
                }
            } catch (e) {
                // In lenient mode if JSON parse fails just send back an _Error
                // and ignore the line (useful for initial development).
                //
                // In non-lenient mode drop the connection here; if the failed line
                // was a request the client is expecting a reply/error message back
                // (otherwise it may go out of sync) but we can't send a synthetic
                // one (as we can't parse the request).
                errmsg = 'JSON parse failed for: ' + jxEncode(str) + ': ' + e;
                this.writeJson({ notify: '_Error', args: [ errmsg ] });
                if (lenientJsonParse) {
                    log.warn('JSON parse failed (lenient mode, ignoring):', e);
                } else {
                    log.warn('JSON parse failed (dropping connection):', e);
                    this.finish(errmsg);
                }
            }

            this.incoming.set(this.incoming.subarray(i + 1));
            this.incomingOffset -= i + 1;
            return msg;
        }
    }
};

JsonConnHandler.prototype.dispatchJsonMessage = function dispatchJsonMessage(msg) {
    var cmd;
    var dvalues = [];
    var i, n;

    log.info('PROXY <-- CLIENT:', JSON.stringify(msg));

    // Parse message type, determine initial marker for binary message.
    if (msg.request) {
        cmd = this.determineCommandNumber(msg.request, msg.command);
        dvalues.push(new Uint8Array([ 0x01 ]));
        dvalues.push(this.encodeJsonDvalue(cmd));
    } else if (msg.reply) {
        dvalues.push(new Uint8Array([ 0x02 ]));
    } else if (msg.notify) {
        cmd = this.determineCommandNumber(msg.notify, msg.command);
        dvalues.push(new Uint8Array([ 0x04 ]));
        dvalues.push(this.encodeJsonDvalue(cmd));
    } else if (msg.error) {
        dvalues.push(new Uint8Array([ 0x03 ]));
    } else {
        throw new Error('invalid input JSON message: ' + jxEncode(msg));
    }

    // Encode arguments into dvalues.
    for (i = 0, n = (msg.args ? msg.args.length : 0); i < n; i++) {
        dvalues.push(this.encodeJsonDvalue(msg.args[i]));
    }

    // Add an EOM, and write out the dvalues to the debug target.
    dvalues.push(new Uint8Array([ 0x00 ]));
    for (i = 0, n = dvalues.length; i < n; i++) {
        this.targetHandler.writeBinary(dvalues[i]);
    }
};

JsonConnHandler.prototype.determineCommandNumber = function determineCommandNumber(name, val) {
    var res;

    if (typeof name === 'string') {
        res = this.commandNumberLookup[name];
        if (!res) {
            log.info('Unknown command name: ' + name + ', command number: ' + val);
        }
    } else if (typeof name === 'number') {
        res = name;
    } else if (name !== true) {
        throw new Error('invalid command name (must be string, number, or "true"): ' + name);
    }
    if (typeof res === 'undefined' && typeof val === 'undefined') {
        throw new Error('cannot determine command number from name: ' + name);
    }
    if (typeof val !== 'number' && typeof val !== 'undefined') {
        throw new Error('invalid command number: ' + val);
    }
    res = res || val;
    return res;
};

JsonConnHandler.prototype.writeDebugStringToBuffer = function writeDebugStringToBuffer(v, buf, off) {
    var i, n;

    for (i = 0, n = v.length; i < n; i++) {
        buf[off + i] = v.charCodeAt(i) & 0xff;  // truncate higher bits
    }
};

JsonConnHandler.prototype.encodeJsonDvalue = function encodeJsonDvalue(v) {
    var buf, dec, len, dv;

    if (isObject(v)) {
        if (v.type === 'eom') {
            return new Uint8Array([ 0x00 ]);
        } else if (v.type === 'req') {
            return new Uint8Array([ 0x01 ]);
        } else if (v.type === 'rep') {
            return new Uint8Array([ 0x02 ]);
        } else if (v.type === 'err') {
            return new Uint8Array([ 0x03 ]);
        } else if (v.type === 'nfy') {
            return new Uint8Array([ 0x04 ]);
        } else if (v.type === 'unused') {
            return new Uint8Array([ 0x15 ]);
        } else if (v.type === 'undefined') {
            return new Uint8Array([ 0x16 ]);
        } else if (v.type === 'number') {
            dec = Duktape.dec('hex', v.data);
            len = dec.length;
            if (len !== 8) {
                throw new TypeError('value cannot be converted to dvalue: ' + jxEncode(v));
            }
            buf = new Uint8Array(1 + len);
            buf[0] = 0x1a;
            buf.set(new Uint8Array(dec), 1);
            return buf;
        } else if (v.type === 'buffer') {
            dec = Duktape.dec('hex', v.data);
            len = dec.length;
            if (len <= 0xffff) {
                buf = new Uint8Array(3 + len);
                buf[0] = 0x14;
                buf[1] = (len >> 8) & 0xff;
                buf[2] = (len >> 0) & 0xff;
                buf.set(new Uint8Arrau(dec), 3);
                return buf;
            } else {
                buf = new Uint8Array(5 + len);
                buf[0] = 0x13;
                buf[1] = (len >> 24) & 0xff;
                buf[2] = (len >> 16) & 0xff;
                buf[3] = (len >> 8) & 0xff;
                buf[4] = (len >> 0) & 0xff;
                buf.set(new Uint8Array(dec), 5);
                return buf;
            }
        } else if (v.type === 'object') {
            dec = Duktape.dec('hex', v.pointer);
            len = dec.length;
            buf = new Uint8Array(3 + len);
            buf[0] = 0x1b;
            buf[1] = v.class;
            buf[2] = len;
            buf.set(new Uint8Array(dec), 3);
            return buf;
        } else if (v.type === 'pointer') {
            dec = Duktape.dec('hex', v.pointer);
            len = dec.length;
            buf = new Uint8Array(2 + len);
            buf[0] = 0x1c;
            buf[1] = len;
            buf.set(new Uint8Array(dec), 2);
            return buf;
        } else if (v.type === 'lightfunc') {
            dec = Duktape.dec('hex', v.pointer);
            len = dec.length;
            buf = new Uint8Array(4 + len);
            buf[0] = 0x1d;
            buf[1] = (v.flags >> 8) & 0xff;
            buf[2] = v.flags & 0xff;
            buf[3] = len;
            buf.set(new Uint8Array(dec), 4);
            return buf;
        } else if (v.type === 'heapptr') {
            dec = Duktape.dec('hex', v.pointer);
            len = dec.length;
            buf = new Uint8Array(2 + len);
            buf[0] = 0x1e;
            buf[1] = len;
            buf.set(new Uint8Array(dec), 2);
            return buf;
        }
    } else if (v === null) {
        return new Uint8Array([ 0x17 ]);
    } else if (typeof v === 'boolean') {
        return new Uint8Array([ v ? 0x18 : 0x19 ]);
    } else if (typeof v === 'number') {
        if (Math.floor(v) === v &&     /* whole */
            (v !== 0 || 1 / v > 0) &&  /* not negative zero */
            v >= -0x80000000 && v <= 0x7fffffff) {
            // Represented signed 32-bit integers as plain integers.
            // Debugger code expects this for all fields that are not
            // duk_tval representations (e.g. command numbers and such).
            if (v >= 0x00 && v <= 0x3f) {
                return new Uint8Array([ 0x80 + v ]);
            } else if (v >= 0x0000 && v <= 0x3fff) {
                return new Uint8Array([ 0xc0 + (v >> 8), v & 0xff ]);
            } else if (v >= -0x80000000 && v <= 0x7fffffff) {
                return new Uint8Array([ 0x10,
                                    (v >> 24) & 0xff,
                                    (v >> 16) & 0xff,
                                    (v >> 8) & 0xff,
                                    (v >> 0) & 0xff ]);
            } else {
                throw new Error('internal error when encoding integer to dvalue: ' + v);
            }
        } else {
            // Represent non-integers as IEEE double dvalues.
            buf = new Uint8Array(1 + 8);
            buf[0] = 0x1a;
            new DataView(buf).setFloat64(1, v, false);
            return buf;
        }
    } else if (typeof v === 'string') {
        if (v.length < 0 || v.length > 0xffffffff) {
            // Not possible in practice.
            throw new TypeError('cannot convert to dvalue, invalid string length: ' + v.length);
        }
        if (v.length <= 0x1f) {
            buf = new Uint8Array(1 + v.length);
            buf[0] = 0x60 + v.length;
            this.writeDebugStringToBuffer(v, buf, 1);
            return buf;
        } else if (v.length <= 0xffff) {
            buf = new Uint8Array(3 + v.length);
            buf[0] = 0x12;
            buf[1] = (v.length >> 8) & 0xff;
            buf[2] = (v.length >> 0) & 0xff;
            this.writeDebugStringToBuffer(v, buf, 3);
            return buf;
        } else {
            buf = new Uint8Array(5 + v.length);
            buf[0] = 0x11;
            buf[1] = (v.length >> 24) & 0xff;
            buf[2] = (v.length >> 16) & 0xff;
            buf[3] = (v.length >> 8) & 0xff;
            buf[4] = (v.length >> 0) & 0xff;
            this.writeDebugStringToBuffer(v, buf, 5);
            return buf;
        }
    }

    throw new TypeError('value cannot be converted to dvalue: ' + jxEncode(v));
};

/*
 *  Target binary connection handler
 */

function TargetConnHandler(server) {
    this.name = 'TargetConnHandler';
    this.server = server;
    this.handle = uv.new_tcp();
    this.jsonHandler = null;
    this.incoming = new Uint8Array(4096);
    this.incomingOffset = 0;
    this.dvalues = [];
}

TargetConnHandler.prototype.finish = function finish(msg) {
    if (!this.handle) {
        log.info('TargetConnHandler already disconnected, ignore finish()');
        return;
    }
    log.info('TargetConnHandler finished:', msg);

    this.jsonHandler.writeJson({ notify: '_TargetDisconnected' });

    // XXX: write a notify to target?

    uv.shutdown(this.handle);
    uv.read_stop(this.handle);
    uv.close(this.handle);
    this.handle = null;

    this.jsonHandler.finish(msg);  // disconnect JSON client too (if not already disconnected)
};

TargetConnHandler.prototype.onConnect = function onConnect(err) {
    var errmsg;

    if (err) {
        errmsg = 'Failed to connect to target: ' + err;
        log.warn(errmsg);
        this.jsonHandler.writeJson({ notify: '_Error', args: [ String(err) ] });
        this.finish(errmsg);
        return;
    }

    // Once we're connected to the target, start read both binary and JSON
    // input.  We don't want to read JSON input before this so that we can
    // always translate incoming messages to dvalues and write them out
    // without queueing.  Any pending JSON messages will be queued by the
    // OS instead.

    log.info('Connected to debug target at', targetHost + ':' + targetPort);
    uv.read_start(this.jsonHandler.handle, this.jsonHandler.onRead.bind(this.jsonHandler));
    uv.read_start(this.handle, this.onRead.bind(this));
};

TargetConnHandler.prototype.writeBinary = function writeBinary(buf) {
    var plain = plainBufferCopy(buf);
    log.info('PROXY --> TARGET:', Duktape.enc('jx', plain));
    if (this.handle) {
        uv.write(this.handle, plain);
    }
};

TargetConnHandler.prototype.onRead = function onRead(err, data) {
    var res;
    var errmsg;
    var tmpBuf;
    var newIncoming;

    log.trace('Received data from target socket, err:', err, 'data length:', data ? data.length : 'null');

    if (err) {
        errmsg = 'Error reading data from debug target: ' + err;
        this.finish(errmsg);
        return;
    }

    if (data) {
        // Feed the data one byte at a time when torture testing.
        if (TORTURE && data.length > 1) {
            for (var i = 0; i < data.length; i++) {
                tmpBuf = allocPlain(1);
                tmpBuf[0] = data[i];
                this.onRead(null, tmpBuf);
            }
            return;
        }

        // Receive data into 'incoming', resizing as necessary.
        while (data.length > this.incoming.length - this.incomingOffset) {
            newIncoming = new Uint8Array(this.incoming.length * 1.3 + 16);
            newIncoming.set(this.incoming);
            this.incoming = newIncoming;
            log.debug('Resize incoming binary buffer to ' + this.incoming.length);
        }
        this.incoming.set(new Uint8Array(data), this.incomingOffset);
        this.incomingOffset += data.length;

        // Trial parse handshake unless done.
        if (!this.handshake) {
            this.trialParseHandshake();
        }

        // Trial parse dvalue(s) and debug messages.
        if (this.handshake) {
            for (;;) {
                res = this.trialParseDvalue();
                if (!res) {
                    break;
                }
                log.trace('Got dvalue:', Duktape.enc('jx', res.dvalue));
                this.dvalues.push(res.dvalue);
                if (isObject(res.dvalue) && res.dvalue.type === 'eom') {
                    try {
                        this.jsonHandler.handleDebugMessage(this.dvalues);
                        this.dvalues = [];
                    } catch (e) {
                        errmsg = 'JSON message handling failed: ' + e;
                        this.jsonHandler.writeJson({ notify: '_Error', args: [ errmsg ] });
                        if (lenientJsonParse) {
                            log.warn('JSON message handling failed (lenient mode, ignoring):', e);
                        } else {
                            log.warn('JSON message handling failed (dropping connection):', e);
                            this.finish(errmsg);
                        }
                    }
                }
            }
        }
    } else {
        log.info('Target disconnected');
        this.finish('Target disconnected');
    }
};

TargetConnHandler.prototype.trialParseHandshake = function trialParseHandshake() {
    var buf = this.incoming;
    var avail = this.incomingOffset;
    var i;
    var msg;
    var m;
    var protocolVersion;

    for (i = 0; i < avail; i++) {
        if (buf[i] == 0x0a) {
            msg = bufferToString(plainBufferCopy(buf.subarray(0, i)));
            this.incoming.set(this.incoming.subarray(i + 1));
            this.incomingOffset -= i + 1;

            // Generic handshake format: only relies on initial version field.
            m = /^(\d+) (.*)$/.exec(msg) || {};
            protocolVersion = +m[1];
            this.handshake = {
                line: msg,
                protocolVersion: protocolVersion,
                text: m[2]
            };

            // More detailed v1 handshake line.
            if (protocolVersion === 1) {
                m = /^(\d+) (\d+) (.*?) (.*?) (.*)$/.exec(msg) || {};
                this.handshake.dukVersion = m[1];
                this.handshake.dukGitDescribe = m[2];
                this.handshake.targetString = m[3];
            }

            this.jsonHandler.writeJson({ notify: '_TargetConnected', args: [ msg ] });

            log.info('Target handshake: ' + JSON.stringify(this.handshake));
            return;
        }
    }
};

TargetConnHandler.prototype.bufferToDebugString = function bufferToDebugString(buf) {
    return String.fromCharCode.apply(null, buf);
};

TargetConnHandler.prototype.trialParseDvalue = function trialParseDvalue() {
    var _this = this;
    var buf = this.incoming;
    var avail = this.incomingOffset;
    var v;
    var gotValue = false;  // explicit flag for e.g. v === undefined
    var dv = new DataView(buf);
    var tmp;
    var x;
    var len;

    function consume(n) {
        log.info('PROXY <-- TARGET:', Duktape.enc('jx', _this.incoming.subarray(0, n)));
        _this.incoming.set(_this.incoming.subarray(n));
        _this.incomingOffset -= n;
    }

    x = buf[0];
    if (avail <= 0) {
        ;
    } else if (x >= 0xc0) {
        // 0xc0...0xff: integers 0-16383
        if (avail >= 2) {
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
        if (avail >= 1 + len) {
            v = new Uint8Array(len);
            v.set(buf.subarray(1, 1 + len));
            v = this.bufferToDebugString(v);
            consume(1 + len);
        }
    } else {
        switch (x) {
        case 0x00: consume(1); v = { type: 'eom' }; break;
        case 0x01: consume(1); v = { type: 'req' }; break;
        case 0x02: consume(1); v = { type: 'rep' }; break;
        case 0x03: consume(1); v = { type: 'err' }; break;
        case 0x04: consume(1); v = { type: 'nfy' }; break;
        case 0x10:  // 4-byte signed integer
            if (avail >= 5) {
                v = dv.getInt32(1, false);
                consume(5);
            }
            break;
        case 0x11:  // 4-byte string
            if (avail >= 5) {
                len = dv.getUint32(1, false);
                if (avail >= 5 + len) {
                    v = new Uint8Array(len);
                    v.set(buf.subarray(5, 5 + len));
                    v = this.bufferToDebugString(v);
                    consume(5 + len);
                }
            }
            break;
        case 0x12:  // 2-byte string
            if (avail >= 3) {
                len = dv.getUint16(1, false);
                if (avail >= 3 + len) {
                    v = new Uint8Array(len);
                    v.set(buf.subarray(3, 3 + len));
                    v = this.bufferToDebugString(v);
                    consume(3 + len);
                }
            }
            break;
        case 0x13:  // 4-byte buffer
            if (avail >= 5) {
                len = dv.getUint32(1, false);
                if (avail >= 5 + len) {
                    v = new Uint8Array(len);
                    v.set(buf.subarray(5, 5 + len));
                    v = { type: 'buffer', data: Duktape.enc('hex', plainOf(v)) };
                    consume(5 + len);
                }
            }
            break;
        case 0x14:  // 2-byte buffer
            if (avail >= 3) {
                len = dv.getUint16(1, false);
                if (avail >= 3 + len) {
                    v = new Uint8Array(len);
                    v.set(buf.subarray(3, 3 + len));
                    v = { type: 'buffer', data: Duktape.enc('hex', plainOf(v)) };
                    consume(3 + len);
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
            if (avail >= 9) {
                tmp = new Uint8Array(8);
                tmp.set(buf.subarray(1, 9));
                v = { type: 'number', data: Duktape.enc('hex', plainOf(tmp)) };
                if (readableNumberValue) {
                    // The value key should not be used programmatically,
                    // it is just there to make the dumps more readable.
                    v.value = new DataView(tmp.buffer).getFloat64(0, false);
                }
                consume(9);
            }
            break;
        case 0x1b:  // object
            if (avail >= 3) {
                len = buf[2];
                if (avail >= 3 + len) {
                    v = new Uint8Array(len);
                    v.set(buf.subarray(3, 3 + len));
                    v = { type: 'object', 'class': buf[1], pointer: Duktape.enc('hex', plainOf(v)) };
                    consume(3 + len);
                }
            }
            break;
        case 0x1c:  // pointer
            if (avail >= 2) {
                len = buf[1];
                if (avail >= 2 + len) {
                    v = new Uint8Array(len);
                    v.set(buf.subarray(2, 2 + len));
                    v = { type: 'pointer', pointer: Duktape.enc('hex', plainOf(v)) };
                    consume(2 + len);
                }
            }
            break;
        case 0x1d:  // lightfunc
            if (avail >= 4) {
                len = buf[3];
                if (avail >= 4 + len) {
                    v = new Uint8Array(len);
                    v.set(buf.subarray(4, 4 + len));
                    v = { type: 'lightfunc', flags: dv.getUint16(1, false), pointer: Duktape.enc('hex', plainOf(v)) };
                    consume(4 + len);
                }
            }
            break;
        case 0x1e:  // heapptr
            if (avail >= 2) {
                len = buf[1];
                if (avail >= 2 + len) {
                    v = new Uint8Array(len);
                    v.set(buf.subarray(2, 2 + len));
                    v = { type: 'heapptr', pointer: Duktape.enc('hex', plainOf(v)) };
                    consume(2 + len);
                }
            }
            break;
        default:
            throw new Error('failed parse initial byte: ' + buf[0]);
        }
    }

    if (typeof v !== 'undefined' || gotValue) {
        return { dvalue: v };
    }
};

/*
 *  Main
 */

function main() {
    var argv = typeof uv.argv === 'function' ? uv.argv() : [];
    var i;
    for (i = 2; i < argv.length; i++) {  // skip dukluv and script name
        if (argv[i] == '--help') {
            print('Usage: dukluv ' + argv[1] + ' [option]+');
            print('');
            print('    --server-host HOST    JSON proxy server listen address');
            print('    --server-port PORT    JSON proxy server listen port');
            print('    --target-host HOST    Debug target address');
            print('    --target-port PORT    Debug target port');
            print('    --metadata FILE       Proxy metadata file (usually named duk_debug_meta.json)');
            print('    --log-level LEVEL     Set log level, default is 2; 0=trace, 1=debug, 2=info, 3=warn, etc');
            print('    --single              Run a single proxy connection and exit (default: persist for multiple connections)');
            print('    --readable-numbers    Add a non-programmatic "value" key for IEEE doubles help readability');
            print('    --lenient             Ignore (with warning) invalid JSON without dropping connection');
            print('    --jx-parse            Parse JSON proxy input with JX, useful when testing manually');
            print('');
            return;  // don't register any sockets/timers etc to exit
        } else if (argv[i] == '--single') {
            singleConnection = true;
            continue;
        } else if (argv[i] == '--readable-numbers') {
            readableNumberValue = true;
            continue;
        } else if (argv[i] == '--lenient') {
            lenientJsonParse = true;
            continue;
        } else if (argv[i] == '--jx-parse') {
            jxParse = true;
            continue;
        }
        if (i >= argv.length - 1) {
            throw new Error('missing option value for ' + argv[i]);
        }
        if (argv[i] == '--server-host') {
            serverHost = argv[i + 1];
            i++;
        } else if (argv[i] == '--server-port') {
            serverPort = Math.floor(+argv[i + 1]);
            i++;
        } else if (argv[i] == '--target-host') {
            targetHost = argv[i + 1];
            i++;
        } else if (argv[i] == '--target-port') {
            targetPort = Math.floor(+argv[i + 1]);
            i++;
        } else if (argv[i] == '--metadata') {
            metadataFile = argv[i + 1];
            i++;
        } else if (argv[i] == '--log-level') {
            log.l = Math.floor(+argv[i + 1]);
            i++;
        } else {
            throw new Error('invalid option ' + argv[i]);
        }
    }

    function runServer() {
        var serverSocket = new JsonProxyServer(serverHost, serverPort);
        var connMode = singleConnection ? 'single connection mode' : 'persistent connection mode';
        log.info('Listening for incoming JSON debug connection on ' + serverHost + ':' + serverPort +
                 ', target is ' + targetHost + ':' + targetPort + ', ' + connMode);
    }

    if (metadataFile) {
        log.info('Read proxy metadata from', metadataFile);
        readFully(metadataFile, function (data, err) {
            if (err) {
                log.error('Failed to load metadata:', err);
                throw err;
            }
            try {
                metadata = JSON.parse(bufferToString(data));
            } catch (e) {
                log.error('Failed to parse JSON metadata from ' + metadataFile + ': ' + e);
                throw e;
            }
            runServer();
        });
    } else {
        runServer();
    }
}

main();
