#include <mirante/widgets/ChoiceListDialog.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

namespace mirante::widgets {

namespace {

// The role of an item's value; its text may add the detail.
constexpr int kValueRole = Qt::UserRole;

} // namespace

struct ChoiceListDialog::Private
{
    QSet<QString> present;   // lowercase
    QString nothing;
    QString allPresent;
    QLabel *status = nullptr;
    QLineEdit *filter = nullptr;
    QListWidget *list = nullptr;
    QPushButton *add = nullptr;

    void updateAdd(const ChoiceListDialog &dialog) { add->setEnabled(!dialog.chosen().isEmpty()); }
};

ChoiceListDialog::ChoiceListDialog(const QString &title, const QStringList &present, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    setWindowTitle(title);
    resize(420, 460);
    for (const QString &value : present)
        d->present.insert(value.toLower());
    d->nothing = tr("There is nothing to list.");
    d->allPresent = tr("Everything listed is already in the list.");

    d->status = new QLabel(this);
    d->status->setObjectName(QStringLiteral("status"));
    d->status->setWordWrap(true);
    d->status->hide();

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

ChoiceListDialog::~ChoiceListDialog() = default;

void ChoiceListDialog::setMessages(const QString &nothing, const QString &allPresent)
{
    d->nothing = nothing;
    d->allPresent = allPresent;
}

void ChoiceListDialog::showMessage(const QString &text)
{
    d->list->clear();
    d->list->setEnabled(false);
    d->filter->setEnabled(false);
    d->status->setText(text);
    d->status->show();
    d->updateAdd(*this);
}

void ChoiceListDialog::setChoices(const std::vector<Choice> &choices)
{
    d->list->clear();
    QSet<QString> seen;
    int added = 0;
    for (const Choice &choice : choices) {
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
        d->status->setText(d->nothing);
        d->status->show();
    } else if (added == 0) {
        d->status->setText(d->allPresent);
        d->status->show();
    } else {
        d->status->hide();
    }
    d->list->setEnabled(d->list->count() > 0);
    d->filter->setEnabled(d->list->count() > 0);
    if (d->list->count() > 0)
        d->filter->setFocus();
    d->updateAdd(*this);
}

QStringList ChoiceListDialog::chosen() const
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

} // namespace mirante::widgets
