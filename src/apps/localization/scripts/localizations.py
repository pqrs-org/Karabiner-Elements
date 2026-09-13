import argparse
import json
import re
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
                or not isinstance(value, str)
            ):
                raise ValueError(f"{key!r}: invalid translation for {language!r}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Validate or format translations")
    parser.add_argument(
        "--file",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "Resources/localizations.json",
    )
    parser.add_argument("--format", action="store_true")
    args = parser.parse_args()
    strings = json.loads(args.file.read_text(encoding="utf-8"))
    validate(strings)
    if args.format:
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
        args.file.write_text(
            json.dumps(strings, ensure_ascii=False, indent=4) + "\n",
            encoding="utf-8",
        )
