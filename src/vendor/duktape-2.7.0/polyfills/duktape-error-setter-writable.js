/*
 *  Ensure Error .fileName, .lineNumber, and .stack are directly writable
 *  without having to use Object.defineProperty().  This matches Duktape
 *  1.4.0 behavior.
 *
 *  See: https://github.com/svaarala/duktape/pull/390.
 */

(function () {
    var err = new Error('test');
    err.fileName = 999;
    if (err.fileName === 999) { return; }  // already writable

    Object.defineProperties(Error.prototype, {
        fileName: { set: new Function('v', 'Object.defineProperty(this, "fileName", { value: v, writable: true, enumerable: false, configurable: true });') },
        lineNumber: { set: new Function('v', 'Object.defineProperty(this, "lineNumber", { value: v, writable: true, enumerable: false, configurable: true });') },
        stack: { set: new Function('v', 'Object.defineProperty(this, "stack", { value: v, writable: true, enumerable: false, configurable: true });') },
    });
})();
