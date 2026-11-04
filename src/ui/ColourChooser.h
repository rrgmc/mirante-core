#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>

#include <memory>

class QColor;

namespace mirante {

// A filled circle in colour, for a swatch or a tab. In its On state, as on a
// checked button, a ring in the text colour goes around it.
QIcon colourDot(const QColor &colour);

// A row of colour swatches: the group colour palette, a swatch for a colour
// outside it, and "Other…", which opens QColorDialog. With allowNone it also
// offers "None" first, for no colour. Used by GroupDialog and DashboardDialog.
// Implemented in ColourChooser.cpp.
class ColourChooser : public QWidget
{
    Q_OBJECT

public:
    ColourChooser(bool allowNone, const QString &dialogTitle, QWidget *parent = nullptr);
    ~ColourChooser() override;

    // The chosen colour as "#rrggbb" in lower case, or an empty string for none.
    QString colour() const;
    // Shows colour as chosen. An empty string means none, if allowed; an
    // invalid colour is ignored.
    void setColour(const QString &colour);

signals:
    // The user chose a colour, or none.
    void colourChosen(const QString &colour);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante
