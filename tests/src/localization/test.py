import importlib.util
import json
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

SCRIPT = (
    Path(__file__).resolve().parents[3]
    / "src/apps/localization/scripts/localizations.py"
)
spec = importlib.util.spec_from_file_location("localizations", SCRIPT)
localizations = importlib.util.module_from_spec(spec)
spec.loader.exec_module(localizations)


class LocalizationResourcesTests(unittest.TestCase):
    def test_application_translation_references(self):
        apps = SCRIPT.parents[2]
        resources = localizations.load_resources(apps / "localization" / "Resources")
        catalog = {
            key: value
            for entries in resources.values()
            for key, value in entries.items()
        }
        for app in (
            "EventViewer",
            "MultitouchExtension",
            "SettingsWindow",
            "localization",
        ):
            for source in (apps / app).rglob("*.swift"):
                if "build" in source.parts:
                    continue
                for key in re.findall(
                    r'"((?:event_viewer|multitouch|shared\.language|shared\.debug|settings\.toolbar)\.[A-Za-z0-9_.]+)"',
                    source.read_text(),
                ):
                    if not key.endswith("."):
                        self.assertIn(key, catalog, str(source))
        for key, translations in catalog.items():
            if key.startswith(("event_viewer.", "multitouch.", "shared.debug.")):
                self.assertIn("ja", translations, key)
                self.assertEqual(
                    set(re.findall(r"\{\w+\}", translations["en"])),
                    set(re.findall(r"\{\w+\}", translations["ja"])),
                    key,
                )

    def test_nested_resources_and_format(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / "settings").mkdir()
            first = directory / "shared.json"
            second = directory / "settings" / "general.json"
            first.write_text(
                '{"shared": {"ja": "共有", "en": "Shared", "fr": "Partagé"}}'
            )
            second.write_text('{"setting": {"en": "Setting"}}')
            (directory / ".temporary.json").write_text("invalid")
            resources = localizations.load_resources(directory)
            self.assertEqual(set(resources), {first, second})
            formatted = localizations.format_strings(resources[first])
            self.assertEqual(json.loads(formatted), resources[first])
            self.assertLess(formatted.index('"en"'), formatted.index('"fr"'))
            self.assertLess(formatted.index('"fr"'), formatted.index('"ja"'))
            self.assertIn('\n        ,"fr":', formatted)
            self.assertEqual(
                localizations.format_strings(json.loads(formatted)), formatted
            )

    def test_duplicates_prevent_any_formatting(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            first = directory / "a.json"
            second = directory / "b.json"
            original = '{"key":{"en":"First"}}'
            first.write_text(original)
            second.write_text('{"key":{"en":"Second"}}')
            result = subprocess.run(
                ["/usr/bin/python3", str(SCRIPT), "--directory", temporary, "--format"],
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Duplicate translation key", result.stderr)
            self.assertEqual(first.read_text(), original)

    def test_duplicate_keys_inside_one_file(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            path = directory / "shared.json"
            for content in [
                '{"key":{"en":"One"},"key":{"en":"Two"}}',
                '{"key":{"en":"One","en":"Two"}}',
            ]:
                path.write_text(content)
                with self.assertRaisesRegex(ValueError, "Duplicate JSON key"):
                    localizations.load_resources(directory)

    def test_empty_directory_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(ValueError, "No translation JSON"):
                localizations.load_resources(Path(temporary))


if __name__ == "__main__":
    unittest.main()
