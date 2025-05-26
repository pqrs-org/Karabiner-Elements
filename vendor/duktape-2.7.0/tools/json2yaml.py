import os, sys, json, yaml

if __name__ == '__main__':
    # Use safe_dump() instead of dump() to avoid tags like "!!python/unicode"
    print(yaml.safe_dump(json.load(sys.stdin), default_flow_style=False))
