#include "ui/ServerChoiceDialog.h"

#include <mirante/ApiClient.h>

namespace mirante {

ServerChoiceDialog::ServerChoiceDialog(const QString &fieldLabel, const QStringList &present, QWidget *parent)
    : ChoiceListDialog(tr("Add %1 from Server").arg(fieldLabel), present, parent)
{
    setMessages(tr("The server has nothing to list."), tr("Everything the server lists is already in the list."));
    showMessage(tr("Loading…"));
}

ServerChoiceDialog::~ServerChoiceDialog() = default;

void ServerChoiceDialog::setChoices(const std::vector<ServerChoice> &choices, const QString &error)
{
    if (!error.isEmpty()) {
        showMessage(tr("The server could not list them: %1").arg(error));
        return;
    }
    std::vector<Choice> values;
    values.reserve(choices.size());
    for (const ServerChoice &choice : choices)
        values.push_back({choice.value, choice.detail});
    ChoiceListDialog::setChoices(values);
}

void ServerChoiceDialog::keep(std::unique_ptr<ApiClient> client) { m_client = std::move(client); }

} // namespace mirante
