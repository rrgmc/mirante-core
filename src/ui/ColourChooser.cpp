#include "ui/ColourChooser.h"

#include "core/GroupColours.h"
#include "items/PaintedIconEngine.h"

#include <QApplication>
#include <QButtonGroup>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QPainter>
#include <QToolButton>

namespace mirante {

namespace {

// The name colourDot() icons report, by which isColourDot() knows them.
constexpr const char *colourDotName = "mirante-colour-dot";

// A colour swatch: a filled circle, with a ring in the text colour around it
// while chosen.
class SwatchIconEngine : public PaintedIconEngine
{
public:
    explicit SwatchIconEngine(const QColor &colour) : m_colour(colour) {}

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        const qreal side = qMin(rect.width(), rect.height());
        const QRectF outer(rect.x() + (rect.width() - side) / 2.0, rect.y() + (rect.height() - side) / 2.0,
                           side, side);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (state == QIcon::On) {
            painter->setPen(QPen(QApplication::palette().color(QPalette::WindowText), 1.5));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(outer.adjusted(1, 1, -1, -1));
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_colour);
        const qreal inset = side * 0.22;
        const QRectF dot = outer.adjusted(inset, inset, -inset, -inset);
        painter->drawEllipse(dot);
        if (mode == QIcon::Selected) {
            painter->setPen(QPen(QApplication::palette().color(QPalette::Active, QPalette::HighlightedText), 1.5));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(dot.adjusted(-0.75, -0.75, 0.75, 0.75));
        }
        painter->restore();
    }

    QIconEngine *clone() const override { return new SwatchIconEngine(m_colour); }

    QString iconName() override { return QString::fromLatin1(colourDotName); }

private:
    QColor m_colour;
};

} // namespace

QIcon colourDot(const QColor &colour) { return QIcon(new SwatchIconEngine(colour)); }

bool isColourDot(const QIcon &icon) { return icon.name() == QLatin1String(colourDotName); }

struct ColourChooser::Private
{
    ColourChooser *q = nullptr;
    QString colour;                       // empty for none
    QButtonGroup *swatches = nullptr;     // "None", the palette, then the custom swatch
    QToolButton *none = nullptr;          // only with allowNone
    QToolButton *customSwatch = nullptr;  // a colour outside the palette; hidden without one

    QToolButton *swatch(const QString &swatchColour, const QString &objectName)
    {
        auto *button = new QToolButton(q);
        button->setObjectName(objectName);
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setIconSize(QSize(20, 20));
        button->setIcon(colourDot(QColor(swatchColour)));
        button->setToolTip(swatchColour);
        button->setProperty("colour", swatchColour);
        return button;
    }

    // Checks the swatch of the colour. A colour outside the palette gets the
    // custom swatch.
    void show()
    {
        if (colour.isEmpty()) {
            customSwatch->hide();
            if (none)
                none->setChecked(true);
            return;
        }
        const bool custom = !groupcolours::palette().contains(colour);
        if (custom) {
            customSwatch->setIcon(colourDot(QColor(colour)));
            customSwatch->setToolTip(colour);
            customSwatch->setProperty("colour", colour);
        }
        customSwatch->setVisible(custom);
        for (QAbstractButton *button : swatches->buttons()) {
            if (button != none && button->property("colour").toString() == colour && (button == customSwatch) == custom)
                button->setChecked(true);
        }
    }

    void choose(const QString &chosen)
    {
        if (chosen.isEmpty() ? !none : !groupcolours::isValid(chosen))
            return;
        colour = chosen.toLower();
        show();
        emit q->colourChosen(colour);
    }
};

ColourChooser::ColourChooser(bool allowNone, const QString &dialogTitle, QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
{
    d->q = this;
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(2);
    d->swatches = new QButtonGroup(this);
    d->swatches->setExclusive(true);
    if (allowNone) {
        d->none = new QToolButton(this);
        d->none->setObjectName(QStringLiteral("noColour"));
        d->none->setCheckable(true);
        d->none->setAutoRaise(true);
        d->none->setText(tr("None"));
        d->swatches->addButton(d->none);
        row->addWidget(d->none);
    }
    for (int i = 0; i < groupcolours::palette().size(); ++i) {
        QToolButton *button = d->swatch(groupcolours::palette().at(i), QStringLiteral("colourSwatch%1").arg(i));
        d->swatches->addButton(button);
        row->addWidget(button);
    }
    d->customSwatch = d->swatch(groupcolours::palette().front(), QStringLiteral("customColour"));
    d->swatches->addButton(d->customSwatch);
    row->addWidget(d->customSwatch);
    auto *other = new QToolButton(this);
    other->setObjectName(QStringLiteral("otherColour"));
    other->setText(tr("Other…"));
    other->setAutoRaise(true);
    row->addWidget(other);
    row->addStretch(1);

    connect(d->swatches, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *button) {
        d->choose(button == d->none ? QString() : button->property("colour").toString());
    });
    connect(other, &QToolButton::clicked, this, [this, dialogTitle] {
        const QColor initial(d->colour.isEmpty() ? groupcolours::palette().front() : d->colour);
        const QColor chosen = QColorDialog::getColor(initial, this, dialogTitle);
        if (chosen.isValid())
            d->choose(chosen.name());
    });
    d->show();
}

ColourChooser::~ColourChooser() = default;

QString ColourChooser::colour() const { return d->colour; }

void ColourChooser::setColour(const QString &colour)
{
    if (colour.isEmpty() ? !d->none : !groupcolours::isValid(colour))
        return;
    d->colour = colour.toLower();
    d->show();
}

} // namespace mirante
