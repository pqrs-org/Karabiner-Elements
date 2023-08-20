import os, sys, json, yaml

if __name__ == '__main__':
    print(json.dumps(yaml.load(sys.stdin)))
