# Editing translations

Edit the feature-specific JSON files under `src/apps/localization/Resources/`.
Keep existing keys and add or update translations under each key:

<!-- prettier-ignore -->
```json
{
    "example.save_button": {
        "en": "Save"
        ,"de": "Speichern"
        ,"fr": "Enregistrer"
        ,"ja": "保存"
    }
}
```

- Each key must appear in only one file and include an `en` fallback.
- Keep placeholders such as `{count}` and `{name}` unchanged, but move them as
  needed to fit the sentence.
- Long translations may be arrays of strings, joined without added separators.
  Use `\n` for line breaks.

## Choosing a language tag

Use BCP 47 language tags. Start with a lowercase language code, such as `ja`
(Japanese), `fr` (French), or `de` (German). Add a region or script only when
needed: `pt-BR` (Brazilian Portuguese), `pt-PT` (European Portuguese),
`zh-Hans` (Simplified Chinese), or `zh-Hant` (Traditional Chinese).
Use hyphens, not underscores, and keep the casing shown in these examples.
For other languages, see [W3C's language tag guide](https://www.w3.org/International/articles/language-tags/).
`auto` is reserved for system language selection. Added languages appear
automatically in the language picker; partial translations are allowed.

When adding a language, also check that selecting `auto` uses it when the
corresponding language is preferred in macOS settings.

## Check your changes

Install a version of Karabiner-Elements that supports external translation files.
Then run from the repository root (Python 3 is required):

```sh
make -C src/apps/localization install
```

This validates and formats the JSON files and installs the translations using
`sudo`. Applications reload them automatically. Select the language, check the
result, and submit the source JSON changes in your pull request.

To validate without installing, run `make -C src/apps/localization check`.
After updating Karabiner-Elements, rerun the install command to reapply local edits.

## Alert previews

In Settings or EventViewer, hold the option key and select Debug in the sidebar.
You can check alerts in the selected language.

## Language-specific guidelines

- [Japanese terminology and style](STYLE_GUIDE.ja.md)
