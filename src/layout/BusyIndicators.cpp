#include "layout/BusyIndicators.h"

#include "items/PaintedIconEngine.h"

#include <QPainter>
#include <QToolButton>
#include <QVariantAnimation>

namespace mirante {

namespace {

// One turn takes this long.
constexpr int turnMs = 900;
// The stripe crosses the bar in this long.
constexpr int crossMs = 1200;

// An icon turned by an angle about its centre.
class TurnedIconEngine : public PaintedIconEngine
{
public:
    TurnedIconEngine(const QIcon &icon, qreal angle) : m_icon(icon), m_angle(angle) {}

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        const QPointF centre = QRectF(rect).center();
        painter->translate(centre);
        painter->rotate(m_angle);
        painter->translate(-centre);
        m_icon.paint(painter, rect, Qt::AlignCenter, mode, state);
        painter->restore();
    }

    QIconEngine *clone() const override { return new TurnedIconEngine(m_icon, m_angle); }

private:
    QIcon m_icon;
    qreal m_angle;
};

QVariantAnimation *loop(QObject *parent, int durationMs, qreal end)
{
    auto *animation = new QVariantAnimation(parent);
    animation->setStartValue(0.0);
    animation->setEndValue(end);
    animation->setDuration(durationMs);
    animation->setLoopCount(-1);
    return animation;
}

} // namespace

SpinningIcon::SpinningIcon(QToolButton *button, const QIcon &icon)
    : QObject(button)
    , m_button(button)
    , m_icon(icon)
    , m_animation(loop(this, turnMs, 360.0))
{
    m_button->setIcon(m_icon);
    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &angle) {
        m_button->setIcon(QIcon(new TurnedIconEngine(m_icon, angle.toReal())));
    });
}

void SpinningIcon::setSpinning(bool spinning)
{
    if (spinning == isSpinning())
        return;
    if (spinning) {
        m_animation->start();
    } else {
        m_animation->stop();
        m_button->setIcon(m_icon);
    }
}

bool SpinningIcon::isSpinning() const { return m_animation->state() == QAbstractAnimation::Running; }

BusyBar::BusyBar(QWidget *parent)
    : QWidget(parent)
    , m_animation(loop(this, crossMs, 1.0))
{
    setObjectName(QStringLiteral("busyBar"));
    setFixedHeight(2);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &position) {
        m_position = position.toReal();
        update();
    });
}

void BusyBar::setBusy(bool busy)
{
    if (busy == isBusy())
        return;
    if (busy) {
        m_animation->start();
    } else {
        m_animation->stop();
        m_position = 0;
    }
    update();
}

bool BusyBar::isBusy() const { return m_animation->state() == QAbstractAnimation::Running; }

// A stripe a third of the width runs from beyond the left edge to beyond
// the right one.
void BusyBar::paintEvent(QPaintEvent *)
{
    if (!isBusy())
        return;
    QPainter painter(this);
    const qreal stripe = width() / 3.0;
    const qreal left = -stripe + m_position * (width() + stripe);
    painter.fillRect(QRectF(left, 0, stripe, height()), palette().color(QPalette::Highlight));
}

} // namespace mirante
