#pragma once

#include <mirante/Fields.h>

#include <QJsonObject>
#include <QString>

#include <functional>
#include <vector>

class QObject;
class QWidget;

namespace mirante {

class ApiClient;
class ItemEditor;
struct EditorContext;

// One value that a server offers for a settings field, such as a repository
// the token can reach. See SettingsScreen::listServerChoices().
struct ServerChoice
{
    QString value;    // what goes in the field, such as "owner/repo"
    QString detail;   // shown after the value in smaller text, such as "Private"; may be empty
};

// Makes a module's own editor of its project or device settings. Include
// <mirante/ItemEditor.h> to implement it.
using EditorFactory = std::function<ItemEditor *(const EditorContext &context, QWidget *parent)>;

// One configuration screen of a module, such as GitLab's "General
// Configuration". Each profile's Modules submenu, in the Profiles menu, lists
// it by name, in a submenu named after the module. Its values are one JSON object per profile, kept in the
// workspace and synced. Secrets, such as tokens, never go there. See
// docs/decisions/0044-module-settings-in-the-workspace.md and
// docs/decisions/0050-profiles-hold-logins-connections-and-settings.md.
//
// Like an item type, a screen lists its fields, and the core builds the form,
// or returns an editor of its own.
class SettingsScreen
{
public:
    virtual ~SettingsScreen() = default;

    // Stable, because it names the stored values: lowercase letters and
    // digits, such as "general". Unique within the module.
    virtual QString id() const = 0;

    // The menu entry and the dialog title, such as "General Configuration".
    virtual QString name() const = 0;

    // The screen's data: the fields of the form the core builds. A screen with
    // its own editor still lists its Secret fields here, so the core can load
    // their state and delete them with the profile.
    virtual std::vector<Field> fields() const { return {}; }

    // An editor of the screen's own, or null to use a form built from
    // fields(). It is given the stored object with setConfig(), and its
    // config() is stored. Include <mirante/ItemEditor.h> to implement it.
    virtual ItemEditor *createEditor(const EditorContext &context, QWidget *parent) const
    {
        Q_UNUSED(context)
        Q_UNUSED(parent)
        return nullptr;
    }

    // The keys of the TextList fields whose values the module's server can
    // list, such as GitHub's repositories. In the form the core builds, each
    // gets an "Add from Server…" button under its list. It opens a list of
    // the values from listServerChoices(), to check and add. Ignored for a
    // screen with its own editor, for a field that is not a TextList, and for
    // a module without a ServerType. See decision 0138.
    virtual QStringList serverChoiceFields() const { return {}; }

    // Asks the server for the values of one of serverChoiceFields(). server
    // uses the base URL and the token in the open dialog, saved or not, so
    // the list works before the first OK. Calls done once: with the values,
    // or with an empty list and a message for the user, such as
    // ApiReply::message. Include <mirante/ApiClient.h> to implement it.
    virtual void listServerChoices(const QString &key, ApiClient &server, QObject *receiver,
                                   std::function<void(const std::vector<ServerChoice> &choices,
                                                      const QString &error)> done) const
    {
        Q_UNUSED(key)
        Q_UNUSED(server)
        Q_UNUSED(receiver)
        done({}, QString());
    }
};

// Reads the stored values of a module's own settings screens. The registrar
// gives one to each module; it lives as long as the module's item types, so
// they may keep it. See ModuleRegistrar::settings().
//
// Each profile has its own values. An item uses those of its profile:
// ItemContext::moduleSettings() reads them for a view, and
// ItemType::fieldsForProfile() is given the profile's id for a form.
class ModuleSettings
{
public:
    virtual ~ModuleSettings() = default;

    // The current values of one of the module's screens in a profile, read on
    // each call. Empty if nothing is stored, the profile is unknown, or no
    // workspace is open.
    virtual QJsonObject values(const QString &profileId, const QString &screenId) const = 0;

    // The values of a screen for one account of the module's server in a
    // profile. An empty accountId is the default account, whose values are
    // those of values(). For another account and the screen its ServerType
    // names, that account's own values; for any other screen, values(). An
    // account the profile does not have gives the default's. A view and a
    // status job get these through moduleSettings() already; this is for
    // code given ids, such as ItemType::fieldsForAccount(). See decision 0146.
    virtual QJsonObject accountValues(const QString &profileId, const QString &accountId,
                                      const QString &screenId) const
    {
        Q_UNUSED(accountId)
        return values(profileId, screenId);
    }
};

} // namespace mirante
