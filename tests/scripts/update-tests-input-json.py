#!/usr/bin/python3

import sys
import json

with open(sys.argv[1], 'r') as f:
    dicts = json.load(f)
    for dict in dicts:
      if 'event_origin' not in dict:
        dict['event_origin'] = 'grabbed_device'

    print(json.dumps(dicts, sort_keys=True, indent=4))
