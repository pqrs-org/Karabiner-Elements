# Editing translations

Settings, ConsoleUserServer, EventViewer, and MultitouchExtension share UTF-8 JSON translation files under:

```text
/Library/Application Support/org.pqrs/Karabiner-Elements/localizations/
```

Sources are grouped by feature under `src/apps/localization/Resources/`:

```text
Resources/
  shared.json
  menu_bar_extra.json
  event_viewer.json
  multitouch_extension.json
  settings/
    changed_settings.json
    general.json
    devices.json
    key_picker.json
    complex_modifications.json
    setup.json
```

All JSON files are loaded recursively into one catalog. A translation key must
appear in only one file, even when adding a language. Hidden files and symbolic
links are ignored. Each top-level key maps language tags directly to translated
strings:

<!-- prettier-ignore -->
```json
{
    "settings.toolbar.language": {
        "en": "Language"
        ,"ja": "言語"
    }
}
```

There is no separate language list. Available languages are collected from all
translation entries and sorted by language tag. Every key must have an `en` value
for fallback. Other languages may have translations for only some keys. Use language
tags such as `ja`, `fr`, `pt-BR`, or `zh-Hant` (lowercase primary language, hyphens
for subtags). `auto` is reserved for the system language selection.

Some strings contain placeholders such as `{count}` or `{name}`. Keep these names
unchanged in translations; you may move them to fit the sentence.

## Change or add a translation

Run these commands from the repository root:

1.  Install an application version that supports this resource directory.
2.  Edit the relevant JSON file under `src/apps/localization/Resources/`. Keep
    existing keys; update the strings or add a language under those keys.
3.  Run `make -C src/apps/localization install`. This validates the entire
    directory, rejects duplicate keys, and formats JSON with sorted keys and
    four-space indentation. It then synchronizes the files using `sudo`, removing
    installed translation files that no longer exist in the source directory.
    Each language's string stays on one line; languages after `en` have a leading
    comma. Embedded newlines use JSON escapes.
    No Xcode, resource compilation, application build, code signing, or application
    restart is needed to update translations.
4.  All four applications automatically reload the installed translations and
    update their translations and language lists. Select the added language and
    check the result, then submit the JSON changes in your pull request.

`make -C src/apps/localization` validates all files without installing them.
`make format` includes localization formatting. The scripts require Python 3.
An application/package installation may replace local translation edits; keep your
source changes and run `make -C src/apps/localization install` again afterward.

## Language preferences

Settings stores the selected language in `karabiner.json`. EventViewer and
MultitouchExtension each store `uiLanguage` using SwiftUI `AppStorage` in their
own application's UserDefaults domain. Neither application reads or writes
`karabiner.json` to select a language. The initial selection is `auto`, which
follows the system's preferred languages and falls back to English.

All three language selectors use the shared AppKit `LanguagePicker`. Settings
and EventViewer place it in the toolbar; MultitouchExtension places it at the
top right of each tab’s content to avoid interfering with the toolbar tabs. Changes
apply immediately, including catalog updates. If a saved language is no longer
available in a successfully loaded catalog, the selection returns to `auto`.

## Alert previews

In Settings or EventViewer, hold Option to reveal the Debug sidebar item.
Select Debug, then release Option to use the preview buttons. The sidebar item
remains visible while Debug is selected. It is hidden when another page is
selected unless Option is held.

Each preview uses the application's normal alert presentation with sample data
where needed, including borderless and full-page variants. The current app
language is used. Click anywhere in the preview or press Escape to close it.
Mouse and keyboard actions are intercepted without changing the alert's enabled
appearance, and automatic navigation is suppressed. Settings includes the
connection retry and advanced driver guidance variants.

Setup previews use the same inline Setup layout and show instructions even when
permissions have already been granted. Driver previews include advanced guidance
and the macOS 13/14 variant. Long instructions can be scrolled; clicking or Escape
returns to the Debug list without changing the actual setup state.

Setup previews also cover prerequisite instructions, partially enabled services,
and macOS 15 screenshots. The Notifications tab previews both external-change
editing cancellation toasts in their normal position with the normal four-second
auto-dismiss timer. Setup completion messages are intentionally omitted.

## Joining translated text

Use an array when independent translated labels should flow as one text, such as
a setting name followed by its default value:

```swift
AppLocalizedText([
  "setting.enable_cgeventtap_fallback",
  " ",
  .init("settings.defaults.value", arguments: ["value": localized("value.off")]),
])
```

Segments are translated with the current locale and joined without an automatic
separator. Put `" "` or `"\n"` directly in the array wherever spacing is needed:

```swift
AppLocalizedText([
  "setting.enable_cgeventtap_fallback",
  "\n",
  .init("settings.defaults.value", arguments: ["value": localized("value.off")]),
])
```

As with a single key, text that is not in the catalog is displayed verbatim. The
result is one verbatim `Text`, so it wraps as a single text block. Existing
single-key calls are unchanged. For sentences whose word order varies by language,
use one translation with placeholders instead of joining fragments.
