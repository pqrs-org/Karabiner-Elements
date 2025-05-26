var testvectors = JSON.parse(new TextDecoder().decode(readFile('appendix_a.json')));

// Very rudimentary for now, just dump useful information about decode
// results and (simple, unstructured) comparison to expected.  This is
// only useful for manually inspecting the results right now.

testvectors.forEach(function (test, idx) {
    print('===', idx, '->', Duktape.enc('jx', test));

    var cbor = Duktape.dec('base64', test.cbor);
    try {
        var dec = CBOR.decode(cbor);
    } catch (e) {
        print('decode failed: ' + e);
        return;
    }

    print('dec (jx): ' + Duktape.enc('jx', dec));

    if (dec !== test.decoded) {
        print('decoded compare failed');
    }
    if (test.roundtrip) {
        var enc = CBOR.encode(dec);
        print('re-enc: ' + Duktape.enc('hex', enc));
        if (Duktape.enc('base64', cbor) !== Duktape.enc('base64', enc)) {
            print('roundtrip failed');
        }
    }
});
