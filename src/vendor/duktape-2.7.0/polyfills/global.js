/*
 *  Duktape 2.5.0 adds a 'globalThis' binding.  Polyfill for earlier versions.
 */

if (typeof globalThis === 'undefined') {
    (function () {
        var global = new Function('return this;')();
        Object.defineProperty(global, 'globalThis', {
            value: global,
            writable: true,
            enumerable: false,
            configurable: true
        });
    })();
}
