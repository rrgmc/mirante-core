#pragma once

#include <mirante/ItemEditor.h>
#include <mirante/ModuleSettings.h>

#include <QJsonObject>
#include <QStringList>
#include <QWidget>

#include <functional>
#include <memory>

namespace mirante {

class ApiClient;
class ServerChoiceDialog;

// The values of one module settings screen in one profile, for the default
// account or another, as a widget: a note on whose settings they are, the
// account's name for another account, the screen's own editor or a FieldForm
// built from its fields, and the message of the last check. Keys the editor
// does not show are kept. A list the module's server can fill has an "Add
// from Server…" button. The Profiles window shows it as a page, and
// ModuleSettingsDialog alone. See docs/ui.md#profiles and decisions 0044,
// 0138, 0144 and 0147. Implemented in ModuleSettingsPage.cpp.
class ModuleSettingsPage : public QWidget
{
    Q_OBJECT

public:
    // What the page edits. An empty accountId is the default account.
    struct Target
    {
        QString profileId;
        QString moduleId;
        QString screenId;
        QString accountId;
    };

    // values are the screen's stored values. A field the values lack shows
    // its default. The screen's editor gets context, which names the profile.
    ModuleSettingsPage(const QString &profileName, const SettingsScreen &screen, const QJsonObject &values,
                       const EditorContext &context = EditorContext(), QWidget *parent = nullptr);
    ~ModuleSettingsPage() override;

    const SettingsScreen &screen() const;
    void setTarget(const Target &target);
    Target target() const;

    // The edited values; keys the editor does not show as given.
    QJsonObject values() const;
    // The secrets typed into the editor, such as a token, by field key. They
    // are never part of values(); the caller keeps them in the keychain.
    QJsonObject secrets() const;
    QStringList removedSecrets() const;
    ItemEditor *editor() const;

    // Makes the page edit another account of the module's server: the note
    // names it, and an Account name row above the editor holds its name,
    // which check() requires.
    void setAccount(const QString &name);
    bool isAccount() const;
    QString accountName() const;

    // Shows what is wrong, under the form, and returns whether nothing is.
    bool check();

    // Whether the user changed anything since the page was made or
    // setModified(false). modified() says when it becomes true.
    bool isModified() const;
    void setModified(bool modified);

    // See ModuleSettingsDialog::setServerFactory().
    using ServerFactory = std::function<std::unique_ptr<ApiClient>(
        const QJsonObject &values, const QJsonObject &typedSecrets, const QStringList &removedSecrets)>;
    void setServerFactory(ServerFactory factory);
    void setChoiceExec(std::function<bool(ServerChoiceDialog &dialog)> exec);
    void addFromServer(const QString &key);

signals:
    void modified();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante
