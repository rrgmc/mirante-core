#include "ui/ModuleSettingsDialog.h"

#include "ui/ServerChoiceDialog.h"

#include <mirante/ApiClient.h>
#include <mirante/widgets/FieldEditor.h>
#include <mirante/widgets/FieldForm.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
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
        m_text->setToolTip(ModuleSettingsDialog::tr("One entry per line."));
        connect(m_text, &QPlainTextEdit::textChanged, this, &FieldEditor::changed);

        button = new QPushButton(ModuleSettingsDialog::tr("Add from Server…"), this);
        button->setObjectName(QStringLiteral("addFromServer_") + field.key);
        button->setToolTip(
            ModuleSettingsDialog::tr("List what the server offers with this token, to check and add."));

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

struct ModuleSettingsDialog::Private
{
    const SettingsScreen *screen = nullptr;
    ItemEditor *editor = nullptr;
    QLabel *message = nullptr;   // why OK was refused
    // The fields with an "Add from Server…" button, by key, with their labels.
    std::map<QString, std::pair<ServerListEditor *, QString>> serverLists;
    ServerFactory serverFactory;
    std::function<bool(ServerChoiceDialog &dialog)> choiceExec = [](ServerChoiceDialog &dialog) {
        return dialog.exec() == QDialog::Accepted;
    };
};

ModuleSettingsDialog::ModuleSettingsDialog(const QString &profileName, const QString &moduleName,
                                           const SettingsScreen &screen, const QJsonObject &values,
                                           const EditorContext &context, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    d->screen = &screen;
    setWindowTitle(tr("%1: %2 %3").arg(profileName, moduleName, screen.name()));
    resize(480, 400);

    // Each profile has its own values; items use those of their profile.
    auto *note = new QLabel(tr("These settings belong to the profile \"%1\". Items that use it read them. They are "
                               "saved in the workspace and shared with it; tokens stay on this device.")
                                .arg(profileName),
                            this);
    note->setObjectName(QStringLiteral("note"));
    note->setWordWrap(true);

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

    d->message = new QLabel(this);
    d->message->setObjectName(QStringLiteral("message"));
    d->message->setWordWrap(true);
    d->message->hide();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ModuleSettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(note);
    layout->addWidget(d->editor, 1);
    layout->addWidget(d->message);
    layout->addWidget(buttons);
}

ModuleSettingsDialog::~ModuleSettingsDialog() = default;

QJsonObject ModuleSettingsDialog::values() const { return d->editor->config(); }

QJsonObject ModuleSettingsDialog::secrets() const { return d->editor->secrets(); }

ItemEditor *ModuleSettingsDialog::editor() const { return d->editor; }

void ModuleSettingsDialog::setServerFactory(ServerFactory factory)
{
    d->serverFactory = std::move(factory);
    for (auto &[key, list] : d->serverLists)
        list.first->button->setEnabled(static_cast<bool>(d->serverFactory));
}

void ModuleSettingsDialog::setChoiceExec(std::function<bool(ServerChoiceDialog &dialog)> exec)
{
    d->choiceExec = std::move(exec);
}

void ModuleSettingsDialog::addFromServer(const QString &key)
{
    const auto it = d->serverLists.find(key);
    if (it == d->serverLists.end() || !d->serverFactory)
        return;
    ServerListEditor *list = it->second.first;

    // The server of the values in the form, saved or not, so the list works
    // before the first OK.
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

void ModuleSettingsDialog::accept()
{
    const QStringList problems = d->editor->problems();
    if (!problems.isEmpty()) {
        d->message->setText(problems.join(QLatin1Char('\n')));
        d->message->show();
        return;
    }
    QDialog::accept();
}

} // namespace mirante
