#include "ui/ProfilesDialog.h"

#include "core/Accounts.h"
#include "core/Edits.h"
#include "core/Model.h"
#include "core/Profiles.h"
#include "items/ItemTypeRegistry.h"

#include <mirante/ApiClient.h>
#include <mirante/ModuleSettings.h>

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QSplitter>
#include <QStyle>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

namespace mirante {

namespace {

constexpr int kNodeRole = Qt::UserRole;          // the node's four ids, as a QStringList
constexpr int kScreenNodeRole = Qt::UserRole + 1; // true: the item shows a screen's page

// "1 group", "3 items". Spelled out so the text reads well without a translation.
QString countText(int count, const QString &one, const QString &many)
{
    return count == 1 ? one : many.arg(count);
}

QStringList nodeData(const ProfilesDialog::Node &node)
{
    return {node.profileId, node.moduleId, node.screenId, node.accountId};
}

ProfilesDialog::Node nodeOf(const QTreeWidgetItem *item)
{
    if (!item)
        return {};
    const QStringList ids = item->data(0, kNodeRole).toStringList();
    if (ids.size() != 4)
        return {};
    return ProfilesDialog::Node{ids.at(0), ids.at(1), ids.at(2), ids.at(3)};
}

QString keyOf(const ProfilesDialog::Node &node) { return nodeData(node).join(QLatin1Char('\n')); }

QLineEdit *readOnlyField(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *field = new QLineEdit(text, parent);
    field->setObjectName(objectName);
    field->setReadOnly(true);
    field->setCursorPosition(0);
    return field;
}

// A folder's path with an Open button. A folder not made yet, because no page
// of the profile has loaded, cannot be opened.
QWidget *folderRow(const QString &path, const QString &objectName, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(readOnlyField(QDir::toNativeSeparators(path), objectName, row), 1);
    auto *open = new QPushButton(ProfilesDialog::tr("Open"), row);
    open->setObjectName(objectName + QStringLiteral("Open"));
    const bool exists = !path.isEmpty() && QFileInfo(path).isDir();
    open->setEnabled(exists);
    open->setToolTip(exists ? ProfilesDialog::tr("Show the folder in the file manager")
                            : ProfilesDialog::tr("Made when a page of this profile first loads"));
    QObject::connect(open, &QPushButton::clicked, row, [path] { QDesktopServices::openUrl(QUrl::fromLocalFile(path)); });
    layout->addWidget(open);
    return row;
}

QLabel *heading(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
    return label;
}

QLabel *greyNote(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setForegroundRole(QPalette::PlaceholderText);
    return label;
}

} // namespace

struct ProfilesDialog::Private
{
    ProfilesDialog *q = nullptr;
    Model *model = nullptr;
    Services services;
    std::function<QString(const QString &, const QString &)> askName;
    std::function<bool(const QString &)> confirm;
    std::function<bool(const QString &)> confirmClear;
    std::function<Unsaved(const QString &)> askUnsaved;

    QTreeWidget *tree = nullptr;
    QLabel *title = nullptr;
    QScrollArea *scroll = nullptr;
    QPointer<QWidget> page;                     // the page shown
    QPointer<ModuleSettingsPage> settingsPage;  // the same, if it is a settings page
    QPointer<QLineEdit> nameEdit;               // on a General page
    QJsonObject pageValues;                     // the stored values the page was made from
    QString pageAccountName;
    Node pageNode;
    QPushButton *removeAccountButton = nullptr;
    QPushButton *revertButton = nullptr;
    QPushButton *applyButton = nullptr;
    QLabel *message = nullptr;
    QSet<QString> expanded;                     // keys of expanded nodes, kept across rebuilds
    bool filling = false;                       // the tree is being built

    // The profile of the selected node.
    QString selectedId() const { return nodeOf(tree->currentItem()).profileId; }

    QString serverScreenOf(const QString &moduleId) const
    {
        const ServerType *server = services.types ? services.types->serverType(moduleId) : nullptr;
        return server ? server->screenId() : QString();
    }

    // The stored values a settings node shows, and its account's name.
    std::pair<QJsonObject, QString> storedOf(const Node &node) const
    {
        if (node.screenId.isEmpty())
            return {};
        if (!node.accountId.isEmpty()) {
            const std::optional<Account> account =
                accounts::find(*model, node.profileId, node.moduleId, node.accountId);
            return account ? std::make_pair(account->values, account->name) : std::make_pair(QJsonObject(), QString());
        }
        return {model->moduleSettings(node.profileId, node.moduleId, node.screenId), QString()};
    }

    QTreeWidgetItem *addItem(QTreeWidgetItem *parent, const QString &text, const Node &node, bool screen)
    {
        auto *item = parent ? new QTreeWidgetItem(parent, QStringList{text}) : new QTreeWidgetItem(tree, QStringList{text});
        item->setData(0, kNodeRole, nodeData(node));
        item->setData(0, kScreenNodeRole, screen);
        return item;
    }

    // The modules of a profile, under its item, as described in the header.
    void addModules(QTreeWidgetItem *profileItem, const QString &profileId)
    {
        const ItemTypeRegistry *types = services.types;
        if (!types)
            return;
        QStringList moduleIds = types->moduleIds();
        std::stable_sort(moduleIds.begin(), moduleIds.end(), [types](const QString &a, const QString &b) {
            return types->moduleName(a).compare(types->moduleName(b), Qt::CaseInsensitive) < 0;
        });
        for (const QString &moduleId : moduleIds) {
            const std::vector<const SettingsScreen *> screens = types->settingsScreens(moduleId);
            if (screens.empty())
                continue;
            const QString serverScreen = serverScreenOf(moduleId);
            if (serverScreen.isEmpty() && screens.size() == 1) {
                addItem(profileItem, types->moduleName(moduleId),
                        Node{profileId, moduleId, screens.front()->id(), QString()}, true);
                continue;
            }
            QTreeWidgetItem *module =
                addItem(profileItem, types->moduleName(moduleId), Node{profileId, moduleId, QString(), QString()}, false);
            if (!serverScreen.isEmpty()) {
                const auto addAccountItem = [&](const QString &label, const QString &accountId) {
                    const Node node{profileId, moduleId, serverScreen, accountId};
                    QTreeWidgetItem *item = addItem(module, label, node, true);
                    if (services.failing
                        && services.failing(ModuleSettingsPage::Target{profileId, moduleId, serverScreen, accountId})) {
                        item->setIcon(0, q->style()->standardIcon(QStyle::SP_MessageBoxWarning));
                        item->setToolTip(0, ProfilesDialog::tr("The server refused this account's token."));
                    }
                };
                addAccountItem(accounts::defaultName(), QString());
                for (const Account &account : accounts::others(*model, profileId, moduleId))
                    addAccountItem(account.name, account.id);
            }
            for (const SettingsScreen *screen : screens) {
                if (screen->id() != serverScreen)
                    addItem(module, screen->name(), Node{profileId, moduleId, screen->id(), QString()}, true);
            }
        }
    }

    QTreeWidgetItem *find(const Node &node) const
    {
        const QString key = keyOf(node);
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if (keyOf(nodeOf(*it)) == key)
                return *it;
        }
        return nullptr;
    }

    // Builds the tree again, keeping the selection, or selecting keep.
    void refresh(std::optional<Node> keep = std::nullopt)
    {
        const Node selected = keep ? *keep : nodeOf(tree->currentItem());
        filling = true;
        tree->clear();
        for (const Profile &profile : profiles::all(*model)) {
            QTreeWidgetItem *item = addItem(nullptr, profiles::listName(*model, profile), Node{profile.id, {}, {}, {}}, false);
            addModules(item, profile.id);
        }
        for (QTreeWidgetItemIterator it(tree); *it; ++it)
            (*it)->setExpanded(expanded.contains(keyOf(nodeOf(*it))));
        QTreeWidgetItem *current = find(selected);
        if (!current)
            current = find(Node{selected.profileId, {}, {}, {}});
        if (!current)
            current = tree->topLevelItem(0);
        // Its parents are open, so the selection shows.
        for (QTreeWidgetItem *parent = current ? current->parent() : nullptr; parent; parent = parent->parent())
            parent->setExpanded(true);
        tree->setCurrentItem(current);
        filling = false;
        showNode(nodeOf(current));
    }

    bool isModified() const
    {
        if (settingsPage)
            return settingsPage->isModified();
        if (nameEdit) {
            const Profile *profile = model->profile(pageNode.profileId);
            return profile && nameEdit->text().trimmed() != profile->name;
        }
        return false;
    }

    void updateButtons()
    {
        const bool editable = settingsPage || nameEdit;
        const bool modified = isModified();
        applyButton->setVisible(editable);
        revertButton->setVisible(editable);
        applyButton->setEnabled(modified);
        revertButton->setEnabled(modified);
        removeAccountButton->setVisible(settingsPage && !pageNode.accountId.isEmpty());
    }

    // Shows the page of node. The page shown already stays while it has
    // changes, or, for a settings page, while its stored values are the
    // same; force makes it again anyway.
    void showNode(const Node &node, bool force = false)
    {
        const auto [values, accountName] = storedOf(node);
        if (!force && page && node == pageNode
            && (isModified() || (settingsPage && values == pageValues && accountName == pageAccountName))) {
            updateButtons();
            return;
        }
        // Later: the page may be in the signal of one of its own buttons.
        if (QWidget *shown = scroll->takeWidget()) {
            shown->hide();
            shown->deleteLater();
        }
        page = nullptr;
        settingsPage = nullptr;
        nameEdit = nullptr;
        pageNode = node;
        pageValues = values;
        pageAccountName = accountName;
        message->hide();

        QWidget *made = nullptr;
        if (node.profileId.isEmpty()) {
            title->clear();
        } else if (node.moduleId.isEmpty()) {
            title->setText(profiles::nameOf(*model, node.profileId));
            made = makeGeneralPage(node.profileId);
        } else if (node.screenId.isEmpty()) {
            title->setText(services.types ? services.types->moduleName(node.moduleId) : node.moduleId);
            made = makeModulePage(node);
        } else if (services.makePage) {
            ModuleSettingsPage *settings = services.makePage(
                ModuleSettingsPage::Target{node.profileId, node.moduleId, node.screenId, node.accountId}, nullptr);
            if (settings) {
                QObject::connect(settings, &ModuleSettingsPage::modified, q, [this] { updateButtons(); });
                settingsPage = settings;
                made = settings;
            }
            title->setText(titleOf(node));
        }
        if (!made)
            made = new QWidget;
        made->setObjectName(made->objectName().isEmpty() ? QStringLiteral("page") : made->objectName());
        page = made;
        scroll->setWidget(made);
        updateButtons();
    }

    // "GitHub · Bot", "GitHub · Default", "GitLab General Configuration".
    QString titleOf(const Node &node) const
    {
        const QString module = services.types ? services.types->moduleName(node.moduleId) : node.moduleId;
        if (node.screenId == serverScreenOf(node.moduleId))
            return ProfilesDialog::tr("%1 · %2").arg(module, accounts::nameOf(*model, node.profileId, node.moduleId,
                                                                             node.accountId));
        const SettingsScreen *screen =
            services.types ? services.types->settingsScreen(node.moduleId, node.screenId) : nullptr;
        return screen ? ProfilesDialog::tr("%1 %2").arg(module, screen->name()) : module;
    }

    QWidget *makeGeneralPage(const QString &profileId)
    {
        const Profile *profile = model->profile(profileId);
        if (!profile)
            return nullptr;
        auto *widget = new QWidget;
        widget->setObjectName(QStringLiteral("generalPage"));
        auto *layout = new QVBoxLayout(widget);
        auto *form = new QFormLayout;
        nameEdit = new QLineEdit(profile->name, widget);
        nameEdit->setObjectName(QStringLiteral("profileName"));
        QObject::connect(nameEdit, &QLineEdit::textEdited, q, [this] { updateButtons(); });
        form->addRow(ProfilesDialog::tr("Name:"), nameEdit.data());

        auto *defaultRow = new QWidget(widget);
        auto *defaultLayout = new QHBoxLayout(defaultRow);
        defaultLayout->setContentsMargins(0, 0, 0, 0);
        if (profileId == model->defaultProfile()) {
            defaultLayout->addWidget(new QLabel(ProfilesDialog::tr("This is the default profile."), defaultRow));
        } else {
            auto *makeDefault = new QPushButton(ProfilesDialog::tr("Set as Default"), defaultRow);
            makeDefault->setObjectName(QStringLiteral("defaultProfile"));
            makeDefault->setToolTip(ProfilesDialog::tr("Use this profile for groups and items that choose none"));
            QObject::connect(makeDefault, &QPushButton::clicked, q, [this] { q->makeSelectedDefault(); });
            defaultLayout->addWidget(makeDefault);
        }
        defaultLayout->addStretch(1);
        form->addRow(ProfilesDialog::tr("Default:"), defaultRow);

        const auto [groups, items] = users(profileId);
        QString used = ProfilesDialog::tr("No group or item chooses it.");
        if (groups > 0 || items > 0)
            used = ProfilesDialog::tr("Chosen by %1.").arg(usersText(groups, items));
        auto *usedLabel = new QLabel(used, widget);
        usedLabel->setObjectName(QStringLiteral("usage"));
        usedLabel->setWordWrap(true);
        form->addRow(ProfilesDialog::tr("Used by:"), usedLabel);
        layout->addLayout(form);

        if (services.storage) {
            if (const std::optional<ProfileStorage> storage = services.storage(profileId)) {
                layout->addSpacing(8);
                layout->addWidget(heading(ProfilesDialog::tr("Storage"), widget));
                layout->addWidget(greyNote(
                    ProfilesDialog::tr("Kept on this device only, for this workspace. Other devices keep their own."),
                    widget));
                auto *storageForm = new QFormLayout;
                storageForm->addRow(ProfilesDialog::tr("Web data"),
                                    folderRow(storage->webData, QStringLiteral("webDataPath"), widget));
                storageForm->addRow(ProfilesDialog::tr("Cache"),
                                    folderRow(storage->cache, QStringLiteral("cachePath"), widget));
                auto *tokens =
                    readOnlyField(storage->tokenPrefix + QStringLiteral("…"), QStringLiteral("tokenPrefix"), widget);
                tokens->setToolTip(
                    ProfilesDialog::tr("Tokens are in the system keychain, in entries whose names start with this."));
                storageForm->addRow(ProfilesDialog::tr("Tokens"), tokens);
                layout->addLayout(storageForm);
            }
        }

        layout->addSpacing(8);
        auto *actions = new QHBoxLayout;
        auto *clear = new QPushButton(ProfilesDialog::tr("Clear Data…"), widget);
        clear->setObjectName(QStringLiteral("clearProfile"));
        clear->setToolTip(ProfilesDialog::tr("Sign out of every site in this profile, on this device"));
        QObject::connect(clear, &QPushButton::clicked, q, [this] { q->clearSelected(); });
        auto *remove = new QPushButton(ProfilesDialog::tr("Delete Profile…"), widget);
        remove->setObjectName(QStringLiteral("deleteProfile"));
        // The workspace always keeps one profile.
        remove->setEnabled(model->profiles().size() > 1);
        QObject::connect(remove, &QPushButton::clicked, q, [this] { q->deleteSelected(); });
        actions->addWidget(clear);
        actions->addWidget(remove);
        actions->addStretch(1);
        layout->addLayout(actions);
        layout->addStretch(1);
        return widget;
    }

    QWidget *makeModulePage(const Node &node)
    {
        auto *widget = new QWidget;
        widget->setObjectName(QStringLiteral("modulePage"));
        auto *layout = new QVBoxLayout(widget);
        const QString description = services.types ? services.types->moduleDescription(node.moduleId) : QString();
        if (!description.isEmpty()) {
            auto *text = new QLabel(description, widget);
            text->setWordWrap(true);
            layout->addWidget(text);
        }
        if (!serverScreenOf(node.moduleId).isEmpty()) {
            layout->addWidget(greyNote(
                ProfilesDialog::tr("Each account has its own server settings and token. Items use the default "
                                   "account unless they choose another in their dialog."),
                widget));
            auto *row = new QHBoxLayout;
            auto *add = new QPushButton(ProfilesDialog::tr("Add Account"), widget);
            add->setObjectName(QStringLiteral("addAccount"));
            QObject::connect(add, &QPushButton::clicked, q, [this] { q->addAccountToSelected(); });
            row->addWidget(add);
            row->addStretch(1);
            layout->addLayout(row);
        }
        layout->addStretch(1);
        return widget;
    }

    // Whether node's page may be left: it has no changes, or they were
    // applied or discarded.
    bool mayLeave()
    {
        if (!page || !isModified())
            return true;
        switch (askUnsaved(title->text())) {
        case Unsaved::Apply:
            return q->applyCurrent();
        case Unsaved::Discard:
            if (settingsPage)
                settingsPage->setModified(false);
            if (nameEdit)
                nameEdit->setText(model->profile(pageNode.profileId) ? model->profile(pageNode.profileId)->name
                                                                       : QString());
            return true;
        case Unsaved::Stay:
            break;
        }
        return false;
    }

    // Why name cannot be used for the profile with id, or an empty string.
    QString problemWith(const QString &name, const QString &id) const
    {
        if (name.isEmpty())
            return ProfilesDialog::tr("Enter a name.");
        for (const Profile &profile : profiles::all(*model)) {
            if (profile.id != id && profile.name.compare(name, Qt::CaseInsensitive) == 0)
                return ProfilesDialog::tr("A profile called \"%1\" already exists.").arg(profile.name);
        }
        return {};
    }

    // Shows why a name was refused, or hides the message.
    bool check(const QString &name, const QString &id)
    {
        const QString problem = problemWith(name, id);
        message->setText(problem);
        message->setVisible(!problem.isEmpty());
        return problem.isEmpty();
    }

    bool rename(const QString &id, const QString &name)
    {
        const Profile *current = model->profile(id);
        if (!current || name.isEmpty() || !check(name, id))
            return false;
        if (name == current->name)
            return true;
        Profile renamed = *current;
        renamed.name = name;
        model->setProfile(renamed);
        return true;
    }

    // How many groups and items name the profile.
    std::pair<int, int> users(const QString &id) const
    {
        int groups = 0;
        int items = 0;
        for (const Group *group : model->groups()) {
            if (group->profile == id)
                ++groups;
            for (const Project *project : model->projectsOfGroup(group->id)) {
                for (const Task *task : model->tasksOfProject(project->id)) {
                    for (const Item &item : task->items) {
                        if (item.profile == id)
                            ++items;
                    }
                }
            }
        }
        return {groups, items};
    }

    static QString usersText(int groups, int items)
    {
        return groups > 0 && items > 0
                   ? ProfilesDialog::tr("%1 and %2").arg(countText(groups, ProfilesDialog::tr("1 group"),
                                                                   ProfilesDialog::tr("%1 groups")),
                                                         countText(items, ProfilesDialog::tr("1 item"),
                                                                   ProfilesDialog::tr("%1 items")))
               : groups > 0 ? countText(groups, ProfilesDialog::tr("1 group"), ProfilesDialog::tr("%1 groups"))
                            : countText(items, ProfilesDialog::tr("1 item"), ProfilesDialog::tr("%1 items"));
    }

    void showContextMenu(const QPoint &position)
    {
        QTreeWidgetItem *item = tree->itemAt(position);
        if (!item)
            return;
        const Node node = nodeOf(item);
        QMenu menu(tree);
        if (node.moduleId.isEmpty()) {
            menu.addAction(ProfilesDialog::tr("Rename…"), q, [this] { q->renameSelected(); });
            QAction *makeDefault = menu.addAction(ProfilesDialog::tr("Set as Default"), q, [this] { q->makeSelectedDefault(); });
            makeDefault->setEnabled(node.profileId != model->defaultProfile());
            menu.addAction(ProfilesDialog::tr("Clear Data…"), q, [this] { q->clearSelected(); });
            QAction *remove = menu.addAction(ProfilesDialog::tr("Delete Profile…"), q, [this] { q->deleteSelected(); });
            remove->setEnabled(model->profiles().size() > 1);
        } else if (!serverScreenOf(node.moduleId).isEmpty()) {
            menu.addAction(ProfilesDialog::tr("Add Account"), q, [this] { q->addAccountToSelected(); });
            if (!node.accountId.isEmpty())
                menu.addAction(ProfilesDialog::tr("Remove Account…"), q, [this] { q->removeSelectedAccount(); });
        }
        if (!menu.isEmpty())
            menu.exec(tree->viewport()->mapToGlobal(position));
    }
};

ProfilesDialog::ProfilesDialog(Model &model, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    d->q = this;
    d->model = &model;
    d->askName = [this](const QString &title, const QString &text) {
        bool ok = false;
        const QString name = QInputDialog::getText(this, title, tr("Name:"), QLineEdit::Normal, text, &ok);
        return ok ? name : QString();
    };
    d->confirm = [this](const QString &question) {
        return QMessageBox::question(this, tr("Profiles"), question) == QMessageBox::Yes;
    };
    d->confirmClear = [this](const QString &name) {
        QMessageBox box(QMessageBox::Warning, tr("Clear profile data"),
                        tr("Clear all web data of the profile \"%1\"?").arg(name), QMessageBox::NoButton, this);
        // The warning icon itself: the native alert on macOS would show the
        // app's icon instead.
        box.setIconPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, this).pixmap(64));
        box.setInformativeText(tr("Its logins, cookies, cache and site data on this device are deleted, so every "
                                  "site in it signs out. Pages in this profile reload. Its module "
                                  "settings stay. This cannot be undone."));
        QPushButton *clear = box.addButton(tr("Clear Data"), QMessageBox::DestructiveRole);
        QPushButton *cancel = box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(cancel);
        box.exec();
        return box.clickedButton() == clear;
    };
    d->askUnsaved = [this](const QString &title) {
        QMessageBox box(QMessageBox::Question, tr("Profiles"), tr("Apply the changes to \"%1\"?").arg(title),
                        QMessageBox::Apply | QMessageBox::Discard | QMessageBox::Cancel, this);
        box.setDefaultButton(QMessageBox::Apply);
        switch (box.exec()) {
        case QMessageBox::Apply:
            return Unsaved::Apply;
        case QMessageBox::Discard:
            return Unsaved::Discard;
        default:
            return Unsaved::Stay;
        }
    };

    setWindowTitle(tr("Profiles"));
    resize(900, 600);

    auto *help = new QLabel(tr("A profile holds web logins and module settings, such as the GitLab or Jira server "
                               "and its token. Each group chooses a profile, and each item can choose its own. "
                               "Those that choose none use the default profile. The list and the settings are "
                               "shared with the workspace; logins and tokens stay on this device."),
                            this);
    help->setObjectName(QStringLiteral("help"));
    help->setWordWrap(true);

    // The warning, in a tinted box with the warning icon. It also says what
    // a profile keeps apart, so the help above does not repeat it.
    auto *warningBox = new QFrame(this);
    warningBox->setObjectName(QStringLiteral("warningBox"));
    warningBox->setStyleSheet(
        QStringLiteral("QFrame#warningBox { background: palette(alternate-base); border-radius: 6px; }"));
    auto *icon = new QLabel(warningBox);
    const int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this) * 2;
    icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, this).pixmap(iconSize));
    icon->setAlignment(Qt::AlignTop);
    auto *warning = new QLabel(tr("Items in the same profile share its logins, servers and tokens. Every item "
                                  "that uses a profile can read the logins and data of all sites in that profile, "
                                  "and call its servers with its tokens. Use separate profiles for things you do "
                                  "not trust together."),
                               warningBox);
    warning->setObjectName(QStringLiteral("warning"));
    warning->setWordWrap(true);
    auto *warningLayout = new QHBoxLayout(warningBox);
    warningLayout->setContentsMargins(10, 8, 10, 8);
    warningLayout->setSpacing(10);
    warningLayout->addWidget(icon, 0, Qt::AlignTop);
    warningLayout->addWidget(warning, 1);

    // The tree, with New Profile under it.
    auto *left = new QWidget(this);
    d->tree = new QTreeWidget(left);
    d->tree->setObjectName(QStringLiteral("profileTree"));
    d->tree->setHeaderHidden(true);
    d->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    d->tree->setContextMenuPolicy(Qt::CustomContextMenu);
    auto *addButton = new QPushButton(tr("New Profile…"), left);
    addButton->setObjectName(QStringLiteral("addProfile"));
    auto *leftButtons = new QHBoxLayout;
    leftButtons->addWidget(addButton);
    leftButtons->addStretch(1);
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(d->tree, 1);
    leftLayout->addLayout(leftButtons);

    // The page, with its title above and its buttons under it.
    auto *right = new QWidget(this);
    d->title = new QLabel(right);
    d->title->setObjectName(QStringLiteral("pageTitle"));
    QFont titleFont = d->title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.2);
    d->title->setFont(titleFont);
    d->scroll = new QScrollArea(right);
    d->scroll->setWidgetResizable(true);
    d->scroll->setFrameShape(QFrame::NoFrame);
    d->message = new QLabel(right);
    d->message->setObjectName(QStringLiteral("message"));
    d->message->setWordWrap(true);
    d->message->hide();
    d->removeAccountButton = new QPushButton(tr("Remove Account…"), right);
    d->removeAccountButton->setObjectName(QStringLiteral("removeAccount"));
    d->revertButton = new QPushButton(tr("Revert"), right);
    d->revertButton->setObjectName(QStringLiteral("revertPage"));
    d->applyButton = new QPushButton(tr("Apply"), right);
    d->applyButton->setObjectName(QStringLiteral("applyPage"));
    auto *pageButtons = new QHBoxLayout;
    pageButtons->addWidget(d->removeAccountButton);
    pageButtons->addStretch(1);
    pageButtons->addWidget(d->revertButton);
    pageButtons->addWidget(d->applyButton);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(d->title);
    rightLayout->addWidget(d->scroll, 1);
    rightLayout->addWidget(d->message);
    rightLayout->addLayout(pageButtons);

    auto *splitter = new QSplitter(this);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({280, 620});

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(help);
    layout->addWidget(warningBox);
    layout->addWidget(splitter, 1);
    layout->addWidget(buttons);

    connect(addButton, &QPushButton::clicked, this, &ProfilesDialog::addProfile);
    connect(d->applyButton, &QPushButton::clicked, this, [this] { applyCurrent(); });
    connect(d->revertButton, &QPushButton::clicked, this, &ProfilesDialog::revertCurrent);
    connect(d->removeAccountButton, &QPushButton::clicked, this, &ProfilesDialog::removeSelectedAccount);
    connect(d->tree, &QTreeWidget::customContextMenuRequested, this,
            [this](const QPoint &position) { d->showContextMenu(position); });
    connect(d->tree, &QTreeWidget::itemExpanded, this,
            [this](QTreeWidgetItem *item) { d->expanded.insert(keyOf(nodeOf(item))); });
    connect(d->tree, &QTreeWidget::itemCollapsed, this,
            [this](QTreeWidgetItem *item) { d->expanded.remove(keyOf(nodeOf(item))); });
    connect(d->tree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
                if (d->filling)
                    return;
                if (previous && !(nodeOf(current) == d->pageNode) && !d->mayLeave()) {
                    // Back to the page with the changes, after this signal.
                    const Node stay = d->pageNode;
                    QMetaObject::invokeMethod(
                        this,
                        [this, stay] {
                            const QSignalBlocker blocker(d->tree);
                            if (QTreeWidgetItem *item = d->find(stay))
                                d->tree->setCurrentItem(item);
                        },
                        Qt::QueuedConnection);
                    return;
                }
                d->showNode(nodeOf(current));
            });
    // Queued: the rename fills the tree again, which deletes the entry the
    // tree is still handling the double-click on.
    connect(
        d->tree, &QTreeWidget::itemDoubleClicked, this,
        [this](QTreeWidgetItem *item) {
            if (nodeOf(item).moduleId.isEmpty())
                renameSelected();
        },
        Qt::QueuedConnection);
    // A reload or another device may change the profiles while the window is
    // open.
    connect(&model, &Model::profilesChanged, this, [this] { d->refresh(); });
    connect(&model, &Model::modelReset, this, [this] { d->refresh(); });

    d->refresh();
    // The profiles start open, so their modules show.
    for (int i = 0; i < d->tree->topLevelItemCount(); ++i)
        d->tree->topLevelItem(i)->setExpanded(true);
}

ProfilesDialog::~ProfilesDialog()
{
    // The tree outlives d, so its signals must not reach it.
    d->tree->disconnect(this);
}

void ProfilesDialog::setServices(Services services)
{
    d->services = std::move(services);
    d->refresh();
    for (int i = 0; i < d->tree->topLevelItemCount(); ++i)
        d->tree->topLevelItem(i)->setExpanded(true);
}

void ProfilesDialog::setAskName(std::function<QString(const QString &, const QString &)> ask)
{
    d->askName = std::move(ask);
}

void ProfilesDialog::setConfirm(std::function<bool(const QString &)> confirm) { d->confirm = std::move(confirm); }

void ProfilesDialog::setConfirmClear(std::function<bool(const QString &)> confirm)
{
    d->confirmClear = std::move(confirm);
}

void ProfilesDialog::setAskUnsaved(std::function<Unsaved(const QString &)> ask) { d->askUnsaved = std::move(ask); }

QStringList ProfilesDialog::ids() const
{
    QStringList result;
    for (int i = 0; i < d->tree->topLevelItemCount(); ++i)
        result << nodeOf(d->tree->topLevelItem(i)).profileId;
    return result;
}

void ProfilesDialog::select(const QString &id) { selectNode(Node{id, {}, {}, {}}); }

void ProfilesDialog::selectNode(const Node &node)
{
    QTreeWidgetItem *item = d->find(node);
    if (!item)
        item = d->find(Node{node.profileId, {}, {}, {}});
    if (!item)
        item = d->find(Node{d->model->defaultProfile(), {}, {}, {}});
    if (!item)
        return;
    for (QTreeWidgetItem *parent = item->parent(); parent; parent = parent->parent())
        parent->setExpanded(true);
    d->tree->setCurrentItem(item);
}

ProfilesDialog::Node ProfilesDialog::currentNode() const { return d->pageNode; }

QWidget *ProfilesDialog::currentPage() const { return d->page; }

ModuleSettingsPage *ProfilesDialog::currentSettingsPage() const { return d->settingsPage; }

bool ProfilesDialog::applyCurrent()
{
    if (d->settingsPage) {
        if (!d->services.applyPage || !d->services.applyPage(*d->settingsPage))
            return false;
        // The tree's names follow, and the page is made again from what was stored.
        d->refresh();
        return true;
    }
    if (d->nameEdit)
        return d->rename(d->pageNode.profileId, d->nameEdit->text().trimmed());
    return true;
}

void ProfilesDialog::revertCurrent()
{
    // Made again from the stored values.
    if (d->settingsPage)
        d->settingsPage->setModified(false);
    d->showNode(d->pageNode, true);
}

void ProfilesDialog::addProfile()
{
    if (!d->mayLeave())
        return;
    const QString name = d->askName(tr("New profile"), QString()).trimmed();
    if (name.isEmpty() || !d->check(name, QString()))
        return;
    Profile profile;
    profile.id = edits::newId();
    profile.name = name;
    d->model->setProfile(profile);
    d->refresh(Node{profile.id, {}, {}, {}});
    if (QTreeWidgetItem *item = d->find(Node{profile.id, {}, {}, {}}))
        item->setExpanded(true);
}

void ProfilesDialog::renameSelected()
{
    const QString id = d->selectedId();
    const Profile *current = d->model->profile(id);
    if (!current)
        return;
    const QString name = d->askName(tr("Rename profile"), current->name).trimmed();
    if (name.isEmpty() || name == current->name)
        return;
    d->rename(id, name);
}

void ProfilesDialog::deleteSelected()
{
    const QString id = d->selectedId();
    const Profile *current = d->model->profile(id);
    if (!current || d->model->profiles().size() < 2)
        return;
    const auto [groups, items] = d->users(id);
    QString question = tr("Delete the profile \"%1\"? Its module settings are deleted, and its logins, "
                          "site data and tokens on this device.")
                           .arg(current->name);
    if (groups > 0 || items > 0) {
        question += QLatin1Char(' ')
            + tr("It is used by %1. A group then uses the default profile, and an item its group's profile.")
                  .arg(Private::usersText(groups, items));
    }
    if (id == d->model->defaultProfile()) {
        // The one the model falls back on without it: the first of the others by name.
        const std::vector<Profile> listed = profiles::all(*d->model);
        const auto next = std::min_element(listed.begin() + 1, listed.end(), [](const Profile &a, const Profile &b) {
            const int byName = a.name.compare(b.name, Qt::CaseInsensitive);
            return byName != 0 ? byName < 0 : a.id < b.id;
        });
        question += QLatin1Char(' ') + tr("It is the default profile; \"%1\" becomes the default.").arg(next->name);
    }
    if (!d->confirm(question))
        return;
    d->message->hide();
    // Its pages go with it, changed or not.
    if (d->settingsPage)
        d->settingsPage->setModified(false);
    if (d->nameEdit)
        d->nameEdit->setText(current->name);
    if (profiles::remove(*d->model, id))
        emit profileDeleted(id);
}

void ProfilesDialog::makeSelectedDefault()
{
    const QString id = d->selectedId();
    if (d->model->profile(id))
        d->model->setDefaultProfile(id);
}

void ProfilesDialog::clearSelected()
{
    const QString id = d->selectedId();
    if (id.isEmpty() || !d->confirmClear(profiles::nameOf(*d->model, id)))
        return;
    emit profileClearRequested(id);
}

void ProfilesDialog::addAccountToSelected()
{
    const Node node = nodeOf(d->tree->currentItem());
    if (node.moduleId.isEmpty() || !d->services.addAccount || d->serverScreenOf(node.moduleId).isEmpty())
        return;
    if (!d->mayLeave())
        return;
    const QString id = d->services.addAccount(node.profileId, node.moduleId);
    if (id.isEmpty())
        return;
    d->refresh(Node{node.profileId, node.moduleId, d->serverScreenOf(node.moduleId), id});
    // The new account's name is the first thing to change.
    if (d->settingsPage) {
        if (auto *name = d->settingsPage->findChild<QLineEdit *>(QStringLiteral("accountName"))) {
            name->setFocus();
            name->selectAll();
        }
    }
}

void ProfilesDialog::removeSelectedAccount()
{
    const Node node = nodeOf(d->tree->currentItem());
    if (node.accountId.isEmpty() || !d->services.removeAccount)
        return;
    // Its changes go with it.
    const bool wasModified = d->settingsPage && d->settingsPage->isModified();
    if (d->settingsPage)
        d->settingsPage->setModified(false);
    if (!d->services.removeAccount(node.profileId, node.moduleId, node.accountId)) {
        if (wasModified && d->settingsPage)
            d->settingsPage->setModified(true);
        d->updateButtons();
        return;
    }
    d->refresh(Node{node.profileId, node.moduleId, {}, {}});
}

void ProfilesDialog::done(int result)
{
    if (!d->mayLeave())
        return;
    QDialog::done(result);
}

} // namespace mirante
