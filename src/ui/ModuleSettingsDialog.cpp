#include "ui/ModuleSettingsDialog.h"

#include "ui/ModuleSettingsPage.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace mirante {

struct ModuleSettingsDialog::Private
{
    ModuleSettingsPage *page = nullptr;
    QString profileName;
    QString moduleName;
    QDialogButtonBox *buttons = nullptr;
    bool removeChosen = false;
};

ModuleSettingsDialog::ModuleSettingsDialog(const QString &profileName, const QString &moduleName,
                                           const SettingsScreen &screen, const QJsonObject &values,
                                           const EditorContext &context, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    d->profileName = profileName;
    d->moduleName = moduleName;
    setWindowTitle(tr("%1: %2 %3").arg(profileName, moduleName, screen.name()));
    resize(480, 400);

    d->page = new ModuleSettingsPage(profileName, screen, values, context, this);

    d->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(d->buttons, &QDialogButtonBox::accepted, this, &ModuleSettingsDialog::accept);
    connect(d->buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(d->page, 1);
    layout->addWidget(d->buttons);
}

ModuleSettingsDialog::~ModuleSettingsDialog() = default;

QJsonObject ModuleSettingsDialog::values() const { return d->page->values(); }

QJsonObject ModuleSettingsDialog::secrets() const { return d->page->secrets(); }

ItemEditor *ModuleSettingsDialog::editor() const { return d->page->editor(); }

ModuleSettingsPage *ModuleSettingsDialog::page() const { return d->page; }

void ModuleSettingsDialog::setAccount(const QString &name)
{
    setWindowTitle(tr("%1: %2 %3 (%4)").arg(d->profileName, d->moduleName, d->page->screen().name(), name));
    d->page->setAccount(name);
    if (!d->buttons->findChild<QPushButton *>(QStringLiteral("removeAccount"))) {
        QPushButton *remove = d->buttons->addButton(tr("Remove Account"), QDialogButtonBox::DestructiveRole);
        remove->setObjectName(QStringLiteral("removeAccount"));
        connect(remove, &QPushButton::clicked, this, &ModuleSettingsDialog::chooseRemove);
    }
}

QString ModuleSettingsDialog::accountName() const { return d->page->accountName(); }

bool ModuleSettingsDialog::removeChosen() const { return d->removeChosen; }

void ModuleSettingsDialog::chooseRemove()
{
    if (!d->page->isAccount())
        return;
    d->removeChosen = true;
    QDialog::accept();
}

void ModuleSettingsDialog::setServerFactory(ServerFactory factory) { d->page->setServerFactory(std::move(factory)); }

void ModuleSettingsDialog::setChoiceExec(std::function<bool(ServerChoiceDialog &dialog)> exec)
{
    d->page->setChoiceExec(std::move(exec));
}

void ModuleSettingsDialog::addFromServer(const QString &key) { d->page->addFromServer(key); }

void ModuleSettingsDialog::accept()
{
    if (!d->page->check())
        return;
    QDialog::accept();
}

} // namespace mirante
