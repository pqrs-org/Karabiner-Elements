/*
 *  Performance.now() polyfill
 *
 *  http://www.w3.org/TR/hr-time/#sec-high-resolution-time
 *
 *  Dummy implementation which uses the Date built-in and has no higher
 *  resolution.  If/when Duktape has a built-in high resolution timer
 *  interface, reimplement this.
 */

var _perfNowZeroTime = Date.now();

if (typeof Performance === 'undefined') {
    Object.defineProperty(this, 'Performance', {
        value: {},
        writable: true, enumerable: false, configurable: true
    });
}
if (typeof Performance.now === 'undefined') {
    Object.defineProperty(Performance, 'now', {
        value: function () {
            return Date.now() - _perfNowZeroTime;
        }, writable: true, enumerable: false, configurable: true
    });
}
