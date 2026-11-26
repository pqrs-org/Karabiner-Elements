"""Format the Simple Modifications definitions in the Karabiner JSON style."""

import json
import sys
from pathlib import Path


def multiline(opening, closing, items, indent):
    body = ",\n".join(f"{indent}    {item}" for item in items)
    return f"{opening}\n{body}\n{indent}{closing}"


def format_value(value, indent=""):
    if isinstance(value, dict):
        # Keep empty objects and single-property objects with inline values on one line.
        # Put each property on its own line for all other objects.
        items = [
            f"{json.dumps(key, ensure_ascii=False)}: {format_value(item, indent + '    ')}"
            for key, item in value.items()
        ]
        if not items:
            return "{}"
        if len(items) == 1 and "\n" not in items[0]:
            return f"{{ {items[0]} }}"
        return multiline("{", "}", items, indent)

    if isinstance(value, list):
        # Keep arrays inline unless an item spans multiple lines or the array has
        # multiple items and at least one is an object or another array.
        items = [format_value(item, indent + "    ") for item in value]
        if (
            len(items) > 1 and any(isinstance(item, (dict, list)) for item in value)
        ) or any("\n" in item for item in items):
            return multiline("[", "]", items, indent)
        return f"[{', '.join(items)}]"

    return json.dumps(value, ensure_ascii=False)


if __name__ == "__main__":
    path = Path(sys.argv[1])
    original = path.read_text(encoding="utf-8")
    formatted = format_value(json.loads(original)) + "\n"
    if original != formatted:
        path.write_text(formatted, encoding="utf-8")
