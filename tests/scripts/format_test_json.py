#!/usr/bin/python3

"""Format input JSONC and result JSON files used by tests."""

import json
import pathlib
import sys

SEPARATE_PROPERTY_KEYS = {
    "device_id",
    "event",
    "event_time_stamp",
    "event_type",
}

RESULT_ALIGNMENT_KEYS = ("keys", "modifiers", "time_stamp", "type")

TEST_JSON_PATTERNS = ("*_input.jsonc", "*_result.json")


def find_compactable_object_spans(text):
    spans = []
    square_bracket_depth = 0
    curly_bracket_depth = 0
    object_start = None
    object_has_comment = False
    state = "normal"
    escaped = False
    i = 0

    while i < len(text):
        c = text[i]
        next_c = text[i + 1] if i + 1 < len(text) else ""

        if state == "string":
            if escaped:
                escaped = False
            elif c == "\\":
                escaped = True
            elif c == '"':
                state = "normal"

        elif state == "line_comment":
            if c == "\n":
                state = "normal"

        elif state == "block_comment":
            if c == "*" and next_c == "/":
                state = "normal"
                i += 1

        else:
            if c == '"':
                state = "string"

            elif c == "/" and next_c == "/":
                if object_start is not None:
                    object_has_comment = True
                state = "line_comment"
                i += 1

            elif c == "/" and next_c == "*":
                if object_start is not None:
                    object_has_comment = True
                state = "block_comment"
                i += 1

            elif c == "[":
                square_bracket_depth += 1

            elif c == "]":
                square_bracket_depth -= 1

            elif c == "{":
                if square_bracket_depth == 1 and curly_bracket_depth == 0:
                    object_start = i
                    object_has_comment = False
                curly_bracket_depth += 1

            elif c == "}":
                curly_bracket_depth -= 1
                if object_start is not None and curly_bracket_depth == 0:
                    if not object_has_comment:
                        spans.append((object_start, i + 1))
                    object_start = None

        i += 1

    return spans


def collapse_whitespace(text):
    result = []
    state = "normal"
    escaped = False
    pending_space = False

    for c in text:
        if state == "string":
            result.append(c)
            if escaped:
                escaped = False
            elif c == "\\":
                escaped = True
            elif c == '"':
                state = "normal"

        elif c == '"':
            if pending_space:
                result.append(" ")
                pending_space = False
            result.append(c)
            state = "string"

        elif c.isspace():
            pending_space = True

        else:
            if pending_space:
                result.append(" ")
                pending_space = False
            result.append(c)

    return "".join(result)


def split_object_properties(text):
    properties = []
    property_start = 1
    curly_bracket_depth = 0
    square_bracket_depth = 0
    state = "normal"
    escaped = False

    for i in range(1, len(text) - 1):
        c = text[i]

        if state == "string":
            if escaped:
                escaped = False
            elif c == "\\":
                escaped = True
            elif c == '"':
                state = "normal"

        elif c == '"':
            state = "string"

        elif c == "{":
            curly_bracket_depth += 1

        elif c == "}":
            curly_bracket_depth -= 1

        elif c == "[":
            square_bracket_depth += 1

        elif c == "]":
            square_bracket_depth -= 1

        elif c == "," and curly_bracket_depth == 0 and square_bracket_depth == 0:
            properties.append(collapse_whitespace(text[property_start:i]).strip())
            property_start = i + 1

    last_property = collapse_whitespace(text[property_start:-1]).strip()
    if last_property:
        properties.append(last_property)

    return properties


def get_property_key(property_text):
    if not property_text.startswith('"'):
        return None

    escaped = False
    for i in range(1, len(property_text)):
        c = property_text[i]
        if escaped:
            escaped = False
        elif c == "\\":
            escaped = True
        elif c == '"':
            return property_text[1:i]

    return None


def format_top_level_object(text):
    properties = [
        (get_property_key(property_text), property_text)
        for property_text in split_object_properties(text)
    ]

    if not any(key in SEPARATE_PROPERTY_KEYS for key, _ in properties):
        return collapse_whitespace(text)

    lines = []
    other_properties = []

    for key, property_text in properties:
        if key in SEPARATE_PROPERTY_KEYS:
            lines.append(property_text)
        else:
            other_properties.append(property_text)

    if other_properties:
        lines.append(", ".join(other_properties))

    return "{\n" + ",\n".join(f"        {line}" for line in lines) + "\n    }"


def format_input_jsonc(text):
    for start, end in reversed(find_compactable_object_spans(text)):
        text = text[:start] + format_top_level_object(text[start:end]) + text[end:]
    return text


def needs_inner_spaces(value):
    if isinstance(value, dict):
        if not value:
            return False
        if len(value) == 1:
            return needs_inner_spaces(next(iter(value.values())))
        return True

    if isinstance(value, list):
        if not value:
            return False
        if len(value) == 1:
            return needs_inner_spaces(value[0])
        return any(isinstance(v, (dict, list)) for v in value)

    return False


def dump_single_line(value):
    if isinstance(value, dict):
        if not value:
            return "{}"
        items = [
            f"{json.dumps(k, ensure_ascii=False)}: {dump_single_line(v)}"
            for k, v in value.items()
        ]
        return "{ " + ", ".join(items) + " }"

    if isinstance(value, list):
        values = ", ".join(dump_single_line(v) for v in value)
        if needs_inner_spaces(value):
            return "[ " + values + " ]"
        return "[" + values + "]"

    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def align_key(lines, key):
    marker = f'"{key}":'
    positions = [line.rfind(marker) for line in lines]
    maximum_position = max((p for p in positions if p >= 0), default=0)

    for i, position in enumerate(positions):
        if position >= 0:
            lines[i] = (
                lines[i][:position]
                + " " * (maximum_position - position)
                + lines[i][position:]
            )


def format_result_json(value):
    lines = [dump_single_line(v) for v in value]

    if not lines:
        return "[\n]\n"

    for key in RESULT_ALIGNMENT_KEYS:
        align_key(lines, key)

    body = ",\n".join(f"    {line}" for line in lines)
    return "[\n" + body + "\n]\n"


def is_event_queue(value):
    if not value:
        return False

    first = value[0]
    return isinstance(first, dict) and SEPARATE_PROPERTY_KEYS.issubset(first)


def format_event_queue_json(value):
    text = json.dumps(value, ensure_ascii=False, indent=4) + "\n"
    return format_input_jsonc(text)


def format_path(path):
    text = path.read_text(encoding="utf-8")

    if path.name.endswith("_input.jsonc"):
        formatted = format_input_jsonc(text)
    elif path.name.endswith("_result.json"):
        value = json.loads(text)
        if is_event_queue(value):
            formatted = format_event_queue_json(value)
        else:
            formatted = format_result_json(value)
    else:
        raise ValueError(f"Unsupported test JSON file: {path}")

    if formatted != text:
        path.write_text(formatted, encoding="utf-8")


def find_test_json_paths(path):
    if not path.is_dir():
        return [path]

    return sorted(
        test_json_path
        for pattern in TEST_JSON_PATTERNS
        for test_json_path in path.rglob(pattern)
    )


def main(paths):
    for path_string in paths:
        for path in find_test_json_paths(pathlib.Path(path_string)):
            format_path(path)


if __name__ == "__main__":
    main(sys.argv[1:])
