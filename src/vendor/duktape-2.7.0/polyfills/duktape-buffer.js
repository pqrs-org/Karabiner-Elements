/*
 *  Duktape.Buffer polyfill for Duktape 1.x compatibility.
 *
 *  Because plain buffer and other buffer behavior changed in Duktape 2.x
 *  quite a bit, 100% matching behavior is not possible.
 */

if (typeof Duktape === 'object' && typeof Duktape.Buffer === 'undefined') {
    (function () {
        function isBufferOrView(v) {
            return v instanceof Buffer ||
                   v instanceof ArrayBuffer ||
                   v instanceof Uint8Array ||  // also matches plain buffer in 2.x
                   v instanceof Uint8ClampedArray ||
                   v instanceof Int8Array ||
                   v instanceof Uint16Array ||
                   v instanceof Int16Array ||
                   v instanceof Uint32Array ||
                   v instanceof Int32Array ||
                   v instanceof Float32Array ||
                   v instanceof Float64Array ||
                   v instanceof DataView;
        }
        var fn = function Buffer(arg) {
            if (this instanceof fn) {
                // Constructor call (not 100% reliable check); result is an
                // ArrayBuffer (in 1.x Duktape.Buffer).  For a buffer argument
                // the ArrayBuffer shares the argument's storage with any slice
                //offset/length lost.
                if (isBufferOrView(arg)) {
                    return Object(Uint8Array.plainOf(arg));
                } else {
                    return Object(Uint8Array.allocPlain(arg));
                }
            } else {
                // Normal call; result is a plain buffer.  For a buffer argument the
                // underlying buffer is returned (shared, not copy).  Otherwise a
                // new plain buffer is created.
                if (isBufferOrView(arg)) {
                    return Uint8Array.plainOf(arg);
                } else {
                    return Uint8Array.allocPlain(arg);
                }
            }
        };
        Object.defineProperty(Duktape, 'Buffer', {
            value: fn,
            writable: true,
            enumerable: false,
            configurable: true
        });
    })();
}
