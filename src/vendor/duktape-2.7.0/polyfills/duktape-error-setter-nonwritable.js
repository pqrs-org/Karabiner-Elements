/*
 *  Ensure Error .fileName, .lineNumber, and .stack are not directly writable,
 *  but can be written using Object.defineProperty().  This matches Duktape
 *  1.3.0 and prior.
 *
 *  See: https://github.com/svaarala/duktape/pull/390.
 */

(function () {
    var err = new Error('test');
    err.fileName = 999;
    if (err.fileName !== 999) { return; }  // already non-writable

    var fn = new Function('');  // nop
    Object.defineProperties(Error.prototype, {
        fileName: { set: fn },
        lineNumber: { set: fn },
        stack: { set: fn }
    });
})();
