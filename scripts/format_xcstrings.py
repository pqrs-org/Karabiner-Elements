import json
from pathlib import Path


# Sort object keys and keep each translation on one line for easy comparison.
# Scope this convention to the shared catalog; other JSON files are unaffected.
def format_value(value, path=(), depth=0):
    indent = "  " * depth
    child_indent = "  " * (depth + 1)
    if isinstance(value, dict) and value:
        compact = len(path) == 3 and path[0] == "strings" and path[2] == "localizations"
        entries = []
        for key, child in sorted(value.items()):
            rendered = (
                json.dumps(child, ensure_ascii=False, sort_keys=True)
                if compact
                else format_value(child, path + (key,), depth + 1)
            )
            entries.append(
                child_indent + json.dumps(key, ensure_ascii=False) + ": " + rendered
            )
        return "{\n" + ",\n".join(entries) + "\n" + indent + "}"
    if isinstance(value, list) and value:
        entries = [
            child_indent + format_value(child, path + (index,), depth + 1)
            for index, child in enumerate(value)
        ]
        return "[\n" + ",\n".join(entries) + "\n" + indent + "]"
    return json.dumps(value, ensure_ascii=False)


if __name__ == "__main__":
    catalog = (
        Path(__file__).resolve().parents[1]
        / "src/apps/share/Resources/Localizable.xcstrings"
    )
    catalog.write_text(
        format_value(json.loads(catalog.read_text(encoding="utf-8"))) + "\n",
        encoding="utf-8",
    )
