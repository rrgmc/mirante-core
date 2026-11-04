#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

namespace mirante::widgets {

// Lists values to check and add to a text list, such as the repositories of a
// module's settings. A filter above the list hides the values that do not
// contain its text. Values already in the field are checked and disabled. A
// TextList field with suggestions opens it from "Add from List…", and the
// settings dialog's "Add from Server…" opens a subclass. See docs/ui.md and
// decisions 0138 and 0139. Implemented in ChoiceListDialog.cpp.
class ChoiceListDialog : public QDialog
{
    Q_OBJECT

public:
    // A value, and an optional detail shown after it in brackets.
    struct Choice
    {
        QString value;
        QString detail;
    };

    // present are the field's current values, compared without case.
    ChoiceListDialog(const QString &title, const QStringList &present, QWidget *parent = nullptr);
    ~ChoiceListDialog() override;

    // The messages shown when setChoices() has no values at all, and when it
    // has only values already present. Both have defaults.
    void setMessages(const QString &nothing, const QString &allPresent);

    // Shows a message in place of the list, such as "Loading…" or an error,
    // and empties the list.
    void showMessage(const QString &text);

    // Shows the values. Repeated values are shown once.
    void setChoices(const std::vector<Choice> &choices);

    // The values the user checked that were not already present, in the
    // order of setChoices().
    QStringList chosen() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante::widgets
