#pragma once

#include <mirante/ItemEditor.h>
#include <mirante/ModuleSettings.h>

#include <QDialog>
#include <QJsonObject>
#include <QStringList>

#include <functional>
#include <memory>

namespace mirante {

class ApiClient;
class ModuleSettingsPage;
class ServerChoiceDialog;

// Edits the values of one module settings screen in one profile, alone, for
// EntityActions::editModuleSettings(). The app opens the Profiles window
// instead (decision 0147). It shows a ModuleSettingsPage: the screen's own
// editor, or else a FieldForm built from its fields. Keys the editor does
// not show are kept. OK is refused while the editor reports problems. A list
// the module's server can fill has an "Add from Server…" button. See
// docs/ui.md#modules and decisions 0044, 0050, 0054 and 0138 in docs/decisions/.
// Implemented in ModuleSettingsDialog.cpp.
class ModuleSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    // The title names the profile, the module and the screen, such as
    // "Default: GitLab General Configuration". values are the screen's stored
    // values in that profile. A field the values lack shows its default. The
    // screen's editor gets context, which names the profile.
    ModuleSettingsDialog(const QString &profileName, const QString &moduleName, const SettingsScreen &screen,
                         const QJsonObject &values, const EditorContext &context = EditorContext(),
                         QWidget *parent = nullptr);
    ~ModuleSettingsDialog() override;

    // The edited values; keys the editor does not show as given.
    QJsonObject values() const;

    // The secrets typed into the editor, such as a token, by field key. They
    // are never part of values(); the caller keeps them in the keychain. See
    // decision 0054.
    QJsonObject secrets() const;

    // Makes the client with which "Add from Server…" asks the module's
    // server, from the values, typed tokens and removed tokens of the open
    // form, or returns null. Without it the buttons are disabled. The screen's
    // serverChoiceFields() get the buttons. See decision 0138.
    using ServerFactory = std::function<std::unique_ptr<ApiClient>(
        const QJsonObject &values, const QJsonObject &typedSecrets, const QStringList &removedSecrets)>;
    void setServerFactory(ServerFactory factory);
    // Runs the list of the server's values and says whether it was accepted.
    // The default calls exec(); tests work the dialog instead.
    void setChoiceExec(std::function<bool(ServerChoiceDialog &dialog)> exec);
    // Opens the list of the server's values for a field, as its button does.
    // Nothing happens for a field without the button.
    void addFromServer(const QString &key);

    // Makes the dialog edit another account of the module's server, not the
    // default: the title and the note name it, a Name row above the editor
    // holds its name, and a Remove Account button closes the dialog as
    // accepted with removeChosen() true. OK is refused without a name. See
    // decision 0144.
    void setAccount(const QString &name);
    QString accountName() const;
    bool removeChosen() const;
    void chooseRemove();   // as the Remove Account button does

    ItemEditor *editor() const;   // for tests
    ModuleSettingsPage *page() const;

    void accept() override;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante
