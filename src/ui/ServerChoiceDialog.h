#pragma once

#include <mirante/ModuleSettings.h>

#include <QDialog>
#include <QStringList>

#include <memory>
#include <vector>

namespace mirante {

class ApiClient;

// Lists the values that a module's server offers for a settings field, such
// as the repositories a token can reach, to check and add to the field.
// "Add from Server…" in a module settings dialog opens it. It says "Loading…"
// until setChoices(). A filter above the list hides the values that do not
// contain its text. Values already in the field are checked and disabled.
// See docs/ui.md#modules and decision 0138. Implemented in ServerChoiceDialog.cpp.
class ServerChoiceDialog : public QDialog
{
    Q_OBJECT

public:
    // fieldLabel names the field in the title, such as "Repositories".
    // present are the field's current values, compared without case.
    ServerChoiceDialog(const QString &fieldLabel, const QStringList &present, QWidget *parent = nullptr);
    ~ServerChoiceDialog() override;

    // Shows the server's answer: the values, or the error when it is not empty.
    // Repeated values are shown once.
    void setChoices(const std::vector<ServerChoice> &choices, const QString &error);

    // The values the user checked that were not already present, in the
    // server's order.
    QStringList chosen() const;

    // Keeps the client of the request until the dialog goes, which ends a
    // request still in flight.
    void keep(std::unique_ptr<ApiClient> client);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante
