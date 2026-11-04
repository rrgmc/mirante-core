#include "ui/ServerChoiceDialog.h"

#include <mirante/ApiClient.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

namespace mirante {

namespace {

// The role of an item's value; its text may add the detail.
constexpr int kValueRole = Qt::UserRole;

} // namespace

struct ServerChoiceDialog::Private
{
    QSet<QString> present;   // lowercase
    QLabel *status = nullptr;
    QLineEdit *filter = nullptr;
    QListWidget *list = nullptr;
    QPushButton *add = nullptr;
    std::unique_ptr<ApiClient> client;

    void updateAdd(const ServerChoiceDialog &dialog)
    {
        add->setEnabled(!dialog.chosen().isEmpty());
    }
};

ServerChoiceDialog::ServerChoiceDialog(const QString &fieldLabel, const QStringList &present, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    setWindowTitle(tr("Add %1 from Server").arg(fieldLabel));
    resize(420, 460);
    for (const QString &value : present)
        d->present.insert(value.toLower());

    d->status = new QLabel(tr("Loading…"), this);
    d->status->setObjectName(QStringLiteral("status"));
    d->status->setWordWrap(true);

    d->filter = new QLineEdit(this);
    d->filter->setObjectName(QStringLiteral("filter"));
    d->filter->setPlaceholderText(tr("Filter"));
    d->filter->setClearButtonEnabled(true);
    d->filter->setEnabled(false);

    d->list = new QListWidget(this);
    d->list->setObjectName(QStringLiteral("choices"));
    d->list->setEnabled(false);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    d->add = buttons->button(QDialogButtonBox::Ok);
    d->add->setText(tr("Add"));
    d->add->setEnabled(false);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(d->filter, &QLineEdit::textChanged, this, [this](const QString &text) {
        const QString needle = text.trimmed();
        for (int i = 0; i < d->list->count(); ++i) {
            QListWidgetItem *item = d->list->item(i);
            item->setHidden(!needle.isEmpty() && !item->text().contains(needle, Qt::CaseInsensitive));
        }
    });
    connect(d->list, &QListWidget::itemChanged, this, [this] { d->updateAdd(*this); });

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(d->status);
    layout->addWidget(d->filter);
    layout->addWidget(d->list, 1);
    layout->addWidget(buttons);
}

ServerChoiceDialog::~ServerChoiceDialog() = default;

void ServerChoiceDialog::setChoices(const std::vector<ServerChoice> &choices, const QString &error)
{
    d->list->clear();
    if (!error.isEmpty()) {
        d->status->setText(tr("The server could not list them: %1").arg(error));
        d->status->show();
        return;
    }
    QSet<QString> seen;
    int added = 0;
    for (const ServerChoice &choice : choices) {
        const QString value = choice.value.trimmed();
        if (value.isEmpty() || seen.contains(value.toLower()))
            continue;
        seen.insert(value.toLower());
        const QString text = choice.detail.isEmpty() ? value : tr("%1 (%2)").arg(value, choice.detail);
        auto *item = new QListWidgetItem(text, d->list);
        item->setData(kValueRole, value);
        if (d->present.contains(value.toLower())) {
            // Already in the field: shown, so the list is complete, but fixed.
            item->setFlags(Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setToolTip(tr("Already in the list"));
        } else {
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            item->setCheckState(Qt::Unchecked);
            ++added;
        }
    }
    if (d->list->count() == 0) {
        d->status->setText(tr("The server has nothing to list."));
    } else if (added == 0) {
        d->status->setText(tr("Everything the server lists is already in the list."));
    } else {
        d->status->hide();
    }
    d->list->setEnabled(d->list->count() > 0);
    d->filter->setEnabled(d->list->count() > 0);
    if (d->list->count() > 0)
        d->filter->setFocus();
    d->updateAdd(*this);
}

QStringList ServerChoiceDialog::chosen() const
{
    QStringList values;
    for (int i = 0; i < d->list->count(); ++i) {
        const QListWidgetItem *item = d->list->item(i);
        const QString value = item->data(kValueRole).toString();
        if (item->checkState() == Qt::Checked && !d->present.contains(value.toLower()))
            values << value;
    }
    return values;
}

void ServerChoiceDialog::keep(std::unique_ptr<ApiClient> client) { d->client = std::move(client); }

} // namespace mirante
