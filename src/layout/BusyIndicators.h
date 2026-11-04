#pragma once

#include <QIcon>
#include <QObject>
#include <QWidget>

class QToolButton;
class QVariantAnimation;

namespace mirante {

// Turns a tool button's icon round and round while busy, such as a cell's
// refresh button while its item's status is fetched. Off, the button shows
// its icon as given. Owned by the button. Implemented in BusyIndicators.cpp.
class SpinningIcon : public QObject
{
    Q_OBJECT

public:
    SpinningIcon(QToolButton *button, const QIcon &icon);

    void setSpinning(bool spinning);
    bool isSpinning() const;

private:
    QToolButton *m_button;
    QIcon m_icon;
    QVariantAnimation *m_animation;
};

// A thin bar along the top of a cell's body that shows a moving stripe while
// busy. It keeps its height while idle, so the body below does not move.
// Implemented in BusyIndicators.cpp.
class BusyBar : public QWidget
{
    Q_OBJECT

public:
    explicit BusyBar(QWidget *parent = nullptr);

    void setBusy(bool busy);
    bool isBusy() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVariantAnimation *m_animation;
    qreal m_position = 0;   // 0 to 1, the stripe's place along the bar
};

} // namespace mirante
