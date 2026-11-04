#include "ui/GroupDialog.h"

#include "core/GroupColours.h"
#include "ui/ColourChooser.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace mirante {

struct GroupDialog::Private
{
    GroupDialog *q = nullptr;
    Group group;

    QLineEdit *name = nullptr;
    QComboBox *profile = nullptr;
    QLabel *message = nullptr;             // why OK was refused
    QFormLayout *form = nullptr;

    // The new profile offered for a new group; null unless offered.
    QCheckBox *newProfile = nullptr;
    QLineEdit *newProfileName = nullptr;
    bool profileNameEdited = false;        // the name no longer follows the group's
    QStringList existingProfileNames;

    // The colour. chosenColour is empty until one is chosen in the dialog.
    QString chosenColour;
    ColourChooser *colours = nullptr;

    QString shownColour() const
    {
        return chosenColour.isEmpty() ? groupcolours::of(group) : chosenColour;
    }

    void chooseColour(const QString &colour)
    {
        if (!groupcolours::isValid(colour))
            return;
        chosenColour = colour.toLower();
        colours->setColour(chosenColour);
    }
};

GroupDialog::GroupDialog(const Group &group, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    d->q = this;
    d->group = group;
    setWindowTitle(group.name.isEmpty() ? tr("New group") : tr("Edit group"));
    resize(520, 300);

    d->name = new QLineEdit(group.name, this);
    d->name->setObjectName(QStringLiteral("name"));

    // The colour: the palette, a swatch for a colour outside it, and "Other…".
    d->colours = new ColourChooser(false, tr("Group colour"), this);
    d->colours->setColour(d->shownColour());
    connect(d->colours, &ColourChooser::colourChosen, this, [this](const QString &colour) { d->chosenColour = colour; });
    auto *colourHelp = new QLabel(tr("The title bars and cell headers of the group's tasks and items, and its "
                                     "projects in the sidebar, are tinted with this colour."),
                                  this);
    colourHelp->setWordWrap(true);

    // The profile.
    d->profile = new QComboBox(this);
    d->profile->setObjectName(QStringLiteral("profile"));
    auto *profileHelp = new QLabel(tr("Items of this group use this profile, unless an item chooses its own."), this);
    profileHelp->setWordWrap(true);

    d->message = new QLabel(this);
    d->message->setObjectName(QStringLiteral("message"));
    d->message->setWordWrap(true);
    d->message->hide();

    auto *form = new QFormLayout;
    d->form = form;
    form->addRow(tr("Name:"), d->name);
    form->addRow(tr("Colour:"), d->colours);
    form->addRow(QString(), colourHelp);
    form->addRow(tr("Profile:"), d->profile);
    form->addRow(QString(), profileHelp);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &GroupDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    // The dialog is never smaller than its content, wrapped help texts and
    // rows added by offerNewProfile() included.
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->addLayout(form);
    layout->addStretch(1);
    layout->addWidget(d->message);
    layout->addWidget(buttons);

    // Until setProfiles() is called, only the default is offered.
    setProfiles({}, tr("Default"));
    d->name->setFocus();
}

GroupDialog::~GroupDialog() = default;

Group GroupDialog::value() const
{
    Group group = d->group;
    group.name = d->name->text().trimmed();

    // The default profile is stored as no profile.
    group.profile = d->profile->currentData().toString();

    if (!d->chosenColour.isEmpty())
        group.colour = d->chosenColour;
    return group;
}

QString GroupDialog::colour() const { return d->shownColour(); }
void GroupDialog::setColour(const QString &colour) { d->chooseColour(colour); }

void GroupDialog::setName(const QString &name) { d->name->setText(name); }

void GroupDialog::setProfiles(const std::vector<Profile> &profiles, const QString &defaultName)
{
    d->profile->clear();
    d->profile->addItem(tr("The default profile (%1)").arg(defaultName), QString());
    for (const Profile &profile : profiles)
        d->profile->addItem(profile.name, profile.id);
    // A profile that is not listed, because it was deleted or has not arrived
    // yet, is kept until another is chosen.
    const QString current = d->group.profile;
    if (d->profile->findData(current) < 0)
        d->profile->addItem(tr("Unknown profile, uses the default"), current);
    d->profile->setCurrentIndex(d->profile->findData(current));
}

void GroupDialog::setProfile(const QString &id)
{
    const int index = d->profile->findData(id);
    if (index >= 0)
        d->profile->setCurrentIndex(index);
}

void GroupDialog::offerNewProfile(const QStringList &existingNames)
{
    if (d->newProfile)
        return;
    d->existingProfileNames = existingNames;
    d->newProfile = new QCheckBox(tr("Create a new profile for this group"), this);
    d->newProfile->setObjectName(QStringLiteral("createProfile"));
    d->newProfile->setChecked(false);
    d->newProfileName = new QLineEdit(d->name->text().trimmed(), this);
    d->newProfileName->setObjectName(QStringLiteral("newProfileName"));
    d->newProfileName->setEnabled(false);
    // The profile row follows the profile list, after its help text.
    d->form->addRow(QString(), d->newProfile);
    d->form->addRow(tr("Profile name:"), d->newProfileName);

    connect(d->newProfile, &QCheckBox::toggled, this, [this](bool on) {
        d->newProfileName->setEnabled(on);
        d->profile->setEnabled(!on);
        if (on)
            d->newProfileName->setFocus();
    });
    // The suggested name is the group's, until the user types another.
    connect(d->name, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!d->profileNameEdited)
            d->newProfileName->setText(text.trimmed());
    });
    connect(d->newProfileName, &QLineEdit::textEdited, this, [this] { d->profileNameEdited = true; });
}

bool GroupDialog::createsProfile() const { return d->newProfile && d->newProfile->isChecked(); }

QString GroupDialog::newProfileName() const
{
    return d->newProfileName ? d->newProfileName->text().trimmed() : QString();
}

void GroupDialog::setCreatesProfile(bool on)
{
    if (d->newProfile)
        d->newProfile->setChecked(on);
}

void GroupDialog::setNewProfileName(const QString &name)
{
    if (!d->newProfileName)
        return;
    d->newProfileName->setText(name);
    d->profileNameEdited = true;
}

void GroupDialog::accept()
{
    const auto refuse = [this](const QString &text, QWidget *focus) {
        d->message->setText(text);
        d->message->show();
        focus->setFocus();
    };
    if (d->name->text().trimmed().isEmpty()) {
        refuse(tr("Enter a name."), d->name);
        return;
    }
    if (createsProfile()) {
        const QString name = newProfileName();
        if (name.isEmpty()) {
            refuse(tr("Enter a name for the new profile."), d->newProfileName);
            return;
        }
        for (const QString &existing : std::as_const(d->existingProfileNames)) {
            if (existing.compare(name, Qt::CaseInsensitive) == 0) {
                refuse(tr("A profile called \"%1\" already exists.").arg(existing), d->newProfileName);
                return;
            }
        }
    }
    QDialog::accept();
}

} // namespace mirante
