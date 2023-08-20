/*
 *  Helper to check if a number is internally represented as a fastint:
 *
 *    if (Duktape.isFastint(x)) {
 *        print('fastint');
 *    } else {
 *        print('not a fastint (or not a number)');
 *    }
 *
 *  NOTE: This helper depends on the internal tag numbering (defined in
 *  duk_tval.h) which is both version specific and depends on whether
 *  duk_tval is packed or not.
 */

if (typeof Duktape === 'object') {
    if (typeof Duktape.fastintTag === 'undefined') {
        Object.defineProperty(Duktape, 'fastintTag', {
            /* Tag number depends on duk_tval packing. */
            value: (Duktape.info(true).itag >= 0xfff0) ?
                    0xfff1 /* tag for packed duk_tval */ :
                    1 /* tag for unpacked duk_tval */,
            writable: false,
            enumerable: false,
            configurable: true
        });
    }
    if (typeof Duktape.isFastint === 'undefined') {
        Object.defineProperty(Duktape, 'isFastint', {
            value: function (v) {
                return Duktape.info(v).type === 4 &&                 /* public type is DUK_TYPE_NUMBER */
                       Duktape.info(v).itag === Duktape.fastintTag;  /* internal tag is fastint */
            },
            writable: false,
            enumerable: false,
            configurable: true
        });
    }
}
