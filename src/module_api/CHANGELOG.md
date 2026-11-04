# Module interface changelog

What changed in the module interface, newest first, and what a module must do about it. CI fails if a pull request changes `src/module_api` without changing this file. See [decision 0119](../../docs/decisions/0119-module-interface-versions-and-checks.md).

While the version is 0.x, any minor version may break modules, so a module must state the exact version. From 1.0, a minor version only adds things, and only a major version may break them.

## Unreleased

Changes that do not affect modules are listed here too.

- A profile can hold several accounts of a module's server (decision 0144). The core picks the item's account: `ItemContext::server()`, `StatusJob::server()` and the quick-add requests give its client, and their `moduleSettings()` give its values for the server's screen. New, with defaults that fall back to the default account: `ModuleSettings::accountValues()`, `ItemType::fieldsForAccount()`, `ItemType::fromUrlForAccount()`, `EditorContext::accountId`, `ItemEditor::setAccount()` and `QuickAddItem::accountId`, which the core fills. `TestRegistrar::setAccountReader()` lets a module's tests give an account's values. Modules need no change, but a module that reads its settings in `fieldsForProfile()` or `fromUrl()` should override the account versions, or its other accounts use the default's values there.

- A `TextList` field with `suggestions` gets an **Add from List…** button under it, which opens a checklist of the suggestions to add. `mirante::widgets::ChoiceListDialog` is that checklist, with a filter, for a module's own editors too. A field without suggestions looks as before, so modules need no change. See decision 0139.
- `SettingsScreen::serverChoiceFields()` and `listServerChoices()` let a module's server fill a `TextList` field of its settings screen, through an **Add from Server…** button under it. `ServerChoice` is a value with an optional detail. Both methods have defaults, so modules need no change. See decision 0138.
- `ItemListView::filterChanged(column, values)` says when the user changes the values chosen in a filter, so a view can keep them and give them back with `setFilterDefault()` next time. Modules need no change.
- `ItemListView::setFilterDefault()` gives a filter the values it chooses at the start, such as those of an item's field. Each is chosen once a row has it, until the user changes the filter. Modules need no change.
- `ItemListView::addFilter()` lets the user choose several values: each has a check box, and a row is shown when its cell has any of them. The drop-down is still a `QComboBox` named `filter.<column>`, and `setCurrentIndex()` still chooses one value alone, so modules and their tests need no change.
- `mirante_finish_modules()` writes each module's release, from its manifest pin, into the generated list of modules, for the About dialog. Modules need no change.
- `mirante_add_module()` takes `OPTIONS NAME=default...`, build options that the manifest's folder entry can set. Each is the variable and the compile definition `MIRANTE_OPTION_<NAME>`. A folder entry can also make a module optional. See decision 0129.
- `mirante_add_module()` takes `EXTENSIONS point=files...`, extension points with their default sources. An app's extension, named in the folder entry's `extensions`, is compiled in place of a point's defaults. See decision 0132.
- A module's own web pages, in its `RESOURCES` under `web/`, are served as `module://<module id>/<path>`, with an origin per module. `createWebView()` refuses `qrc:` addresses, which share one origin. See decision 0128.
- A module with a folder `translations/` carries its own texts, as `<id>_<language>.ts`. The core loads them for the enabled modules. A module without them keeps working, in English.

## 0.1

The first version with a number.

- `mirante_add_module()` needs `API 0.1`. Without it, configuration stops.
- `<mirante/ModuleApiVersion.h>` gives the version as C++ macros.
- `mirante_add_module()` builds modules with `-Werror=suggest-override` on GCC and Clang. A method that overrides without saying `override` stops the build.
- `QT_LIBS` may name only Qt modules that the core uses or that the manifest's `qt` list names.
