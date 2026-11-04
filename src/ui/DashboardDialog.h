#pragma once

#include "core/Entities.h"

#include <QDialog>

#include <memory>

namespace mirante {

// Creates or renames a dashboard. When creating, it also offers the presets of
// docs/ui.md#layout-presets. For a root dashboard, offerColour() adds its tab
// colour. OK is refused while the name is empty. See the M3 plan, part 5.
// Implemented in DashboardDialog.cpp.
class DashboardDialog : public QDialog
{
    Q_OBJECT

public:
    DashboardDialog(const Dashboard &dashboard, bool creating, QWidget *parent = nullptr);
    ~DashboardDialog() override;

    // The dashboard with the edited name; the layout as given. The caller
    // builds the layout from presetIndex().
    Dashboard value() const;
    // The chosen index in presets::names(), or -1 when not creating.
    int presetIndex() const;

    // Adds a colour row showing colour, which may be empty for none.
    void offerColour(const QString &colour);
    // The chosen colour, or an empty string for none or if not offered.
    QString colour() const;

    // For tests.
    void setName(const QString &name);
    void setPresetIndex(int index);

    void accept() override;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante
