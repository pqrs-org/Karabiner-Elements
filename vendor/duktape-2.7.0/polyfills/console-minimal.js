/*
 *  Minimal console.log() polyfill
 */

if (typeof console === 'undefined') {
    Object.defineProperty(this, 'console', {
        value: {}, writable: true, enumerable: false, configurable: true
    });
}
if (typeof console.log === 'undefined') {
    (function () {
        var origPrint = print;  // capture in closure in case changed later
        Object.defineProperty(this.console, 'log', {
            value: function () {
                var strArgs = Array.prototype.map.call(arguments, function (v) { return String(v); });
                origPrint(Array.prototype.join.call(strArgs, ' '));
            }, writable: true, enumerable: false, configurable: true
        });
    })();
}
