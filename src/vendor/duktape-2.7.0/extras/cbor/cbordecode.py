#!/usr/bin/env python2

def main():
    import sys
    import cbor
    import json

    data = sys.stdin.read()
    print('LEN: %d' % len(data))
    sys.stdout.flush()
    print('HEX: ' + data.encode('hex'))
    sys.stdout.flush()
    doc = cbor.loads(data)
    print('REPR: ' + repr(doc))
    sys.stdout.flush()
    try:
        print('JSON: ' + json.dumps(doc))
        sys.stdout.flush()
    except:
        print('JSON: cannot encode')
        sys.stdout.flush()

if __name__ == '__main__':
    main()
