import argparse
import json
import re
from collections import Counter
from pathlib import Path


def validate(strings):
    if not isinstance(strings, dict) or not strings:
        raise ValueError("Translations must be a nonempty object")
    for key, translations in strings.items():
        if not key or not isinstance(translations, dict) or "en" not in translations:
            raise ValueError(f"{key!r}: each key must have an English translation")
        for language, value in translations.items():
            if (
                not re.fullmatch(r"[a-z]{2,8}(?:-[A-Za-z0-9]{1,8})*", language)
                or language == "auto"
                or not (
                    isinstance(value, str)
                    or (
                        isinstance(value, list)
                        and all(isinstance(segment, str) for segment in value)
                    )
                )
            ):
                raise ValueError(f"{key!r}: invalid translation for {language!r}")


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"Duplicate JSON key: {key!r}")
        result[key] = value
    return result


def load_resources(directory):
    resources = {}
    owners = {}
    for path in sorted(directory.rglob("*.json")):
        if any(part.startswith(".") for part in path.relative_to(directory).parts):
            continue
        if path.is_symlink() or not path.is_file():
            continue
        strings = json.loads(
            path.read_text(encoding="utf-8"), object_pairs_hook=unique_object
        )
        validate(strings)
        for key in strings:
            if key in owners:
                raise ValueError(
                    f"Duplicate translation key {key!r}: {owners[key]} and {path}"
                )
            owners[key] = path
        resources[path] = strings
    if not resources:
        raise ValueError(f"No translation JSON files in {directory}")
    return resources


def format_strings(strings):
    def format_value(value):
        if isinstance(value, list) and value:
            return (
                "[\n"
                + ",\n".join(
                    "            " + json.dumps(segment, ensure_ascii=False)
                    for segment in value
                )
                + "\n        ]"
            )
        return json.dumps(value, ensure_ascii=False)

    # Sort translation keys alphabetically, but keep English first within each key.
    strings = {
        key: {
            language: translations[language]
            for language in sorted(
                translations, key=lambda language: (language != "en", language)
            )
        }
        for key, translations in sorted(strings.items())
    }
    # Keep commas before additional languages so English-only edits do not
    # depend on whether another translation follows.
    entries = []
    for key, translations in strings.items():
        languages = [
            f"{json.dumps(language)}: {format_value(value)}"
            for language, value in translations.items()
        ]
        entries.append(
            f"    {json.dumps(key, ensure_ascii=False)}: {{\n"
            + "        "
            + "\n        ,".join(languages)
            + "\n    }"
        )
    return "{\n" + ",\n".join(entries) + "\n}\n"


def compile_resources(resources):
    return {
        key: {
            language: "".join(value) if isinstance(value, list) else value
            for language, value in translations.items()
        }
        for strings in resources.values()
        for key, translations in strings.items()
    }


def print_coverage(resources):
    total = sum(len(strings) for strings in resources.values())
    counts = Counter(
        language
        for strings in resources.values()
        for translations in strings.values()
        for language in translations
    )
    print(f"Translation keys: {total}")
    print("Language  Translated / Total  Coverage")
    for language in sorted(counts, key=lambda language: (language != "en", language)):
        count = counts[language]
        print(f"{language:<8}  {count} / {total}  {count / total:.1%}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Validate, format, or compile translation resources"
    )
    parser.add_argument(
        "--directory",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "Resources",
    )
    parser.add_argument("--format", action="store_true")
    parser.add_argument(
        "--output", type=Path, help="Write a merged translation catalog"
    )
    args = parser.parse_args()
    # Validate every file and detect duplicates before rewriting any file.
    resources = load_resources(args.directory)
    if args.format:
        for path, strings in resources.items():
            path.write_text(format_strings(strings), encoding="utf-8")
    else:
        print_coverage(resources)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            format_strings(compile_resources(resources)), encoding="utf-8"
        )
