#include "ui/ModuleSettingsPage.h"

#include "ui/ServerChoiceDialog.h"

#include <mirante/ApiClient.h>
#include <mirante/widgets/FieldEditor.h>
#include <mirante/widgets/FieldForm.h>

#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include <map>

namespace mirante {

using widgets::FieldEditor;
using widgets::FieldForm;

namespace {

// A text list, as the form shows one, with an "Add from Server…" button
// under it. See SettingsScreen::serverChoiceFields().
class ServerListEditor : public FieldEditor
{
public:
    ServerListEditor(const Field &field, QWidget *parent)
        : FieldEditor(parent)
    {
        m_text = new QPlainTextEdit(this);
        m_text->setObjectName(QStringLiteral("text"));
        m_text->setPlaceholderText(field.placeholder);
        m_text->setTabChangesFocus(true);
        m_text->setToolTip(ModuleSettingsPage::tr("One entry per line."));
        connect(m_text, &QPlainTextEdit::textChanged, this, &FieldEditor::changed);

        button = new QPushButton(ModuleSettingsPage::tr("Add from Server…"), this);
        button->setObjectName(QStringLiteral("addFromServer_") + field.key);
        button->setToolTip(ModuleSettingsPage::tr(
            "List what the server offers with this token, to check and add."));

        auto *buttons = new QHBoxLayout;
        buttons->addWidget(button);
        buttons->addStretch();
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_text, 1);
        layout->addLayout(buttons);
    }

    // One entry per line, trimmed, without blank lines.
    QStringList entries() const
    {
        QStringList list;
        for (const QString &line : m_text->toPlainText().split(QLatin1Char('\n'))) {
            const QString entry = line.trimmed();
            if (!entry.isEmpty())
                list << entry;
        }
        return list;
    }

    // Adds entries at the end, one per line.
    void append(const QStringList &values)
    {
        if (values.isEmpty())
            return;
        setLines(entries() + values);
    }

    void setValue(const QJsonValue &value) override
    {
        QStringList lines;
        for (const QJsonValue &entry : value.toArray())
            lines << entry.toString();
        setLines(lines);
    }

    QJsonValue value() const override
    {
        const QStringList list = entries();
        return list.isEmpty() ? QJsonValue(QJsonValue::Undefined) : QJsonValue(QJsonArray::fromStringList(list));
    }

    QPushButton *button = nullptr;

private:
    void setLines(const QStringList &lines) { m_text->setPlainText(lines.join(QLatin1Char('\n'))); }

    QPlainTextEdit *m_text = nullptr;
};

} // namespace

struct ModuleSettingsPage::Private
{
    const SettingsScreen *screen = nullptr;
    Target target;
    QString profileName;
    ItemEditor *editor = nullptr;
    QLabel *note = nullptr;
    QLabel *message = nullptr;          // what check() found
    QWidget *accountRow = nullptr;      // another account's name, hidden for the default
    QLineEdit *accountName = nullptr;
    bool modified = false;
    // The fields with an "Add from Server…" button, by key, with their labels.
    std::map<QString, std::pair<ServerListEditor *, QString>> serverLists;
    ServerFactory serverFactory;
    std::function<bool(ServerChoiceDialog &dialog)> choiceExec = [](ServerChoiceDialog &dialog) {
        return dialog.exec() == QDialog::Accepted;
    };
};

ModuleSettingsPage::ModuleSettingsPage(const QString &profileName, const SettingsScreen &screen,
                                       const QJsonObject &values, const EditorContext &context, QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
{
    d->screen = &screen;
    d->profileName = profileName;

    // Each profile has its own values; items use those of their profile.
    d->note = new QLabel(tr("These settings belong to the profile \"%1\". Items that use it read them. They are "
                            "saved in the workspace and shared with it; tokens stay on this device.")
                             .arg(profileName),
                         this);
    d->note->setObjectName(QStringLiteral("note"));
    d->note->setWordWrap(true);

    d->accountRow = new QWidget(this);
    d->accountName = new QLineEdit(d->accountRow);
    d->accountName->setObjectName(QStringLiteral("accountName"));
    auto *accountForm = new QFormLayout(d->accountRow);
    accountForm->setContentsMargins(0, 0, 0, 0);
    accountForm->addRow(tr("Account name:"), d->accountName);
    d->accountRow->hide();
    connect(d->accountName, &QLineEdit::textEdited, this, [this] { setModified(true); });

    d->editor = screen.createEditor(context, this);
    if (!d->editor) {
        auto *form = new FieldForm(screen.fields(), context, this);
        d->editor = form;
        // The text lists that the server can fill get a button under them.
        const QStringList serverFields = screen.serverChoiceFields();
        for (const Field &field : screen.fields()) {
            if (field.type != Field::Type::TextList || !serverFields.contains(field.key))
                continue;
            auto *list = new ServerListEditor(field, form);
            list->button->setEnabled(false);
            const QString key = field.key;
            connect(list->button, &QPushButton::clicked, this, [this, key] { addFromServer(key); });
            form->setFieldEditor(key, list);
            d->serverLists[key] = {list, field.label};
        }
    }
    d->editor->setObjectName(QStringLiteral("editor"));
    // A field without a stored value shows its default as real text, such as
    // GitLab's https://gitlab.com, since that is the value the module uses.
    QJsonObject shown = values;
    for (const Field &field : screen.fields()) {
        if (!shown.contains(field.key) && !field.defaultValue.isNull() && !field.defaultValue.isUndefined())
            shown.insert(field.key, field.defaultValue);
    }
    d->editor->setConfig(shown);
    connect(d->editor, &ItemEditor::changed, this, [this] { setModified(true); });

    d->message = new QLabel(this);
    d->message->setObjectName(QStringLiteral("message"));
    d->message->setWordWrap(true);
    d->message->hide();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->note);
    layout->addWidget(d->accountRow);
    layout->addWidget(d->editor, 1);
    layout->addWidget(d->message);
}

ModuleSettingsPage::~ModuleSettingsPage() = default;

const SettingsScreen &ModuleSettingsPage::screen() const { return *d->screen; }

void ModuleSettingsPage::setTarget(const Target &target) { d->target = target; }

ModuleSettingsPage::Target ModuleSettingsPage::target() const { return d->target; }

QJsonObject ModuleSettingsPage::values() const { return d->editor->config(); }

QJsonObject ModuleSettingsPage::secrets() const { return d->editor->secrets(); }

QStringList ModuleSettingsPage::removedSecrets() const { return d->editor->removedSecrets(); }

ItemEditor *ModuleSettingsPage::editor() const { return d->editor; }

void ModuleSettingsPage::setAccount(const QString &name)
{
    d->note->setText(tr("These settings belong to the account \"%1\" of the profile \"%2\". Items that choose "
                        "it read them. They are saved in the workspace and shared with it; tokens stay on this "
                        "device.")
                         .arg(name, d->profileName));
    d->accountName->setText(name);
    d->accountRow->show();
}

bool ModuleSettingsPage::isAccount() const { return !d->accountRow->isHidden(); }

QString ModuleSettingsPage::accountName() const { return d->accountName->text().trimmed(); }

bool ModuleSettingsPage::check()
{
    QStringList problems = d->editor->problems();
    if (isAccount() && accountName().isEmpty())
        problems.prepend(tr("The account needs a name."));
    d->message->setText(problems.join(QLatin1Char('\n')));
    d->message->setVisible(!problems.isEmpty());
    return problems.isEmpty();
}

bool ModuleSettingsPage::isModified() const { return d->modified; }

void ModuleSettingsPage::setModified(bool modified)
{
    if (d->modified == modified)
        return;
    d->modified = modified;
    if (modified)
        emit this->modified();
}

void ModuleSettingsPage::setServerFactory(ServerFactory factory)
{
    d->serverFactory = std::move(factory);
    for (auto &[key, list] : d->serverLists)
        list.first->button->setEnabled(static_cast<bool>(d->serverFactory));
}

void ModuleSettingsPage::setChoiceExec(std::function<bool(ServerChoiceDialog &dialog)> exec)
{
    d->choiceExec = std::move(exec);
}

void ModuleSettingsPage::addFromServer(const QString &key)
{
    const auto it = d->serverLists.find(key);
    if (it == d->serverLists.end() || !d->serverFactory)
        return;
    ServerListEditor *list = it->second.first;

    // The server of the values in the form, saved or not, so the list works
    // before the values are stored.
    std::unique_ptr<ApiClient> client
        = d->serverFactory(d->editor->config(), d->editor->secrets(), d->editor->removedSecrets());
    ServerChoiceDialog choices(it->second.second, list->entries(), this);
    if (!client) {
        choices.setChoices({}, tr("This module has no server to ask."));
    } else {
        ApiClient &server = *client;
        choices.keep(std::move(client));
        QPointer<ServerChoiceDialog> guard(&choices);
        d->screen->listServerChoices(key, server, &choices,
                                     [guard](const std::vector<ServerChoice> &values, const QString &error) {
                                         if (guard)
                                             guard->setChoices(values, error);
                                     });
    }
    if (d->choiceExec(choices))
        list->append(choices.chosen());
}

} // namespace mirante
