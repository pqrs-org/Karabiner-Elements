===========================================================
Debug transport with local debug protocol encoding/decoding
===========================================================

This example implements a debug transport which decodes/encodes the Duktape
debug protocol locally into a more easy to use C interface, which is useful
for debug clients implemented locally on the target.  The example also
demonstrates how to trial parse dvalues in C.
