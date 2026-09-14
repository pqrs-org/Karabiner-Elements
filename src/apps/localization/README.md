# Editing translations

Settings and ConsoleUserServer share a plain UTF-8 JSON file at:

```
/Library/Application Support/org.pqrs/Karabiner-Elements/localizations.json
```

The source is `src/apps/localization/Resources/localizations.json`. Each top-level
key maps language tags directly to translated strings:

```json
{
    "settings.toolbar.language": {
        "en": "Language",
        "ja": "言語"
    }
}
```

There is no separate language list. Available languages are collected from all
translation entries and sorted by language tag. Every key must have an `en` value
for fallback. Other languages may have translations for only some keys. Use language
tags such as `ja`, `fr`, `pt-BR`, or `zh-Hant` (lowercase primary language, hyphens
for subtags). `auto` is reserved for the system language selection.

## Change or add a translation

Run these commands from the repository root:

1.  Install an application version that supports this JSON format.
2.  Edit `src/apps/localization/Resources/localizations.json`. Keep existing keys;
    update the strings or add a language under the keys you want to translate.
3.  Run `make -C src/apps/localization install`. This validates and formats the
    JSON with sorted keys and four-space indentation, then copies it using `sudo`.
    Each language's string stays on one line; embedded newlines use JSON escapes.
    No Xcode, resource compilation, application build, code signing, or application
    restart is needed to update translations.
4.  Settings and ConsoleUserServer automatically reload the installed JSON and
    update their translations and language lists. Select the added language and
    check the result, then submit the JSON changes in your pull request.

`make -C src/apps/localization` validates the file without installing it.
`make format` includes localization formatting. The scripts require Python 3.
An application/package installation may replace local translation edits; keep your
source changes and run `make -C src/apps/localization install` again afterward.

## Language selection and reload

`Auto` uses the system's preferred languages; other choices show each language's
native name. `global.ui_language` stores the selection, including region/script variants.
A saved language is retained if it disappears from a replacement JSON file.
Unsupported languages and missing translations fall back to English. Unknown keys
are displayed as-is. Values are plain strings, with no automatic plural or format
specifier processing.

Each application monitors the installed file with `DispatchSource`. Consecutive
file events are coalesced for 100 ms before reloading. In-place writes and atomic
file replacements are supported. Reload parses and validates the entire file
before publishing it; invalid JSON leaves the previous translations intact, and
a later valid save updates them. Automatic reload failures are logged.

## Using a translation in code

SwiftUI views use `AppLocalizedText(key)` or `AppLocalizedLabel(key, systemImage: ...)`.
These observe localization reloads and read the selected locale from the environment.
Strings for reports and menu items use `AppLanguage.text(key, locale: ...)`; views
using this directly also observe `AppLocalization.shared` to refresh on reload.
Write localization keys as complete string literals so their usages are searchable;
do not construct them from prefixes or configuration keys. Changed Settings uses
explicit mappings in `ChangedSettings.swift`.
User-provided names and messages are displayed verbatim. System-provided panels
continue to use the system's language.
