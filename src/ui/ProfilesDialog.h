#pragma once

#include "ui/ModuleSettingsPage.h"

#include <QDialog>

#include <functional>
#include <memory>
#include <optional>

namespace mirante {

class ItemTypeRegistry;
class Model;

// Where a profile keeps its data on this device. EntityActions fills it from
// the ProfileManager and the workspace id.
struct ProfileStorage
{
    QString webData;        // the profile's folder: logins, cookies and site data
    QString cache;          // the web cache, inside the profile's folder
    QString tokenPrefix;    // the start of its keychain entries' names
};

// The Profiles window, opened with Profiles > Manage Profiles…, and the one
// place where profiles and their settings are edited (decision 0147).
//
// A tree on the left lists the profiles, the default first. Under each, each
// module with settings screens; under a module with a server, its accounts,
// "Default" first, then its other screens. A module without a server and
// with one screen is that screen. The page of the selected node is on the
// right: a profile's General page (name, default, who uses it, storage, Clear
// Data…, Delete Profile…), a module's page (Add Account), or a screen's form,
// for the default account or another (with its name and Remove Account).
//
// A page that edits values has Apply and Revert. Leaving a changed page, or
// closing the window, asks first. Adding and deleting profiles and accounts,
// the default and clearing data happen at once. The last profile cannot be
// deleted. A warning says that items in one profile share its logins,
// servers and tokens. See docs/ui.md#profiles. Implemented in
// ProfilesDialog.cpp.
class ProfilesDialog : public QDialog
{
    Q_OBJECT

public:
    // A node of the tree: a profile, with only profileId; a module, with
    // moduleId too; or a screen, with screenId too, for the account accountId
    // (empty for the default).
    struct Node
    {
        QString profileId;
        QString moduleId;
        QString screenId;
        QString accountId;

        bool operator==(const Node &other) const
        {
            return profileId == other.profileId && moduleId == other.moduleId && screenId == other.screenId
                && accountId == other.accountId;
        }
    };

    // What the window needs beyond the model to show modules and their
    // settings. Without types, the tree lists the profiles only. A missing
    // function leaves its part out, such as the storage without web profiles.
    struct Services
    {
        const ItemTypeRegistry *types = nullptr;
        std::function<ModuleSettingsPage *(const ModuleSettingsPage::Target &target, QWidget *parent)> makePage;
        std::function<bool(ModuleSettingsPage &page)> applyPage;
        std::function<QString(const QString &profileId, const QString &moduleId)> addAccount;
        std::function<bool(const QString &profileId, const QString &moduleId, const QString &accountId)>
            removeAccount;
        std::function<std::optional<ProfileStorage>(const QString &profileId)> storage;
        // Whether the account's token is refused, marked in the tree.
        std::function<bool(const ModuleSettingsPage::Target &target)> failing;
    };

    explicit ProfilesDialog(Model &model, QWidget *parent = nullptr);
    ~ProfilesDialog() override;

    void setServices(Services services);

    // Asks for a name, with a title and a starting text. Returns an empty
    // string when cancelled. The default shows an input dialog; tests replace it.
    void setAskName(std::function<QString(const QString &title, const QString &text)> ask);
    // Asks the user to confirm a deletion. The default shows a message box.
    void setConfirm(std::function<bool(const QString &question)> confirm);
    // Asks the user to confirm clearing the data of the named profile. The
    // default shows a warning with the warning icon; tests replace it.
    void setConfirmClear(std::function<bool(const QString &profileName)> confirm);
    // Asks what to do with the changes of the page named title before leaving
    // it. The default shows a message box.
    enum class Unsaved { Apply, Discard, Stay };
    void setAskUnsaved(std::function<Unsaved(const QString &title)> ask);

    // The profile ids in the tree, in its order: the default first, then by name.
    QStringList ids() const;
    // Selects a profile's General page.
    void select(const QString &id);
    // Selects a node. One that is not in the tree selects its profile, or
    // the default profile.
    void selectNode(const Node &node);
    Node currentNode() const;
    // The page shown, and the settings page if it is one.
    QWidget *currentPage() const;
    ModuleSettingsPage *currentSettingsPage() const;
    // Applies or reverts the shown page, as its buttons do. applyCurrent()
    // returns false when the page reports a problem.
    bool applyCurrent();
    void revertCurrent();

    // What the buttons and the context menu do, on the selected node's
    // profile or account. Each asks first where it should.
    void addProfile();
    void renameSelected();
    void deleteSelected();
    void clearSelected();
    // Makes the selected profile the workspace's default. Does not ask.
    void makeSelectedDefault();
    // Adds another account to the selected module and selects it.
    void addAccountToSelected();
    void removeSelectedAccount();

    void done(int result) override;

signals:
    // A profile was deleted from the list. Its web logins on this device go next.
    void profileDeleted(const QString &id);
    // The user chose to clear a profile's web logins and site data on this device.
    void profileClearRequested(const QString &id);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante
