// Tests platform::centredOnWindow: a dialog is centred on its window and kept
// inside the free area of that window's screen, also on a screen left of the
// primary one, at negative coordinates.
#include "platform/DialogScreens.h"

#include <QTest>

using namespace mirante;

class TestDialogScreens : public QObject
{
    Q_OBJECT

private slots:
    void centredOnWindow_data()
    {
        QTest::addColumn<QRect>("window");
        QTest::addColumn<QSize>("size");
        QTest::addColumn<QMargins>("frame");
        QTest::addColumn<QRect>("available");
        QTest::addColumn<QPoint>("expected");

        const QMargins frame(8, 31, 8, 8);
        const QRect left(-2560, 0, 2560, 1032);
        const QRect primary(0, 0, 3440, 1392);

        // The frame is 376x459. QRect::center() rounds toward zero.
        QTest::newRow("centred") << QRect(-2000, 100, 1200, 800) << QSize(360, 420) << frame << left
                                 << QPoint(-1588, 270);
        QTest::newRow("centred on the primary screen")
            << QRect(1000, 200, 1200, 800) << QSize(360, 420) << frame << primary << QPoint(1411, 370);
        // A window that reaches past the screen's far edge.
        QTest::newRow("kept inside on the right")
            << QRect(-400, 100, 800, 600) << QSize(360, 420) << frame << left << QPoint(-376, 170);
        QTest::newRow("kept inside on the left")
            << QRect(-2700, 100, 400, 600) << QSize(360, 420) << frame << left << QPoint(-2560, 170);
        QTest::newRow("kept inside at the bottom")
            << QRect(-2000, 900, 1200, 400) << QSize(360, 420) << frame << left << QPoint(-1588, 573);
        QTest::newRow("larger than the screen keeps its corner")
            << QRect(-2560, 0, 2560, 1032) << QSize(3000, 1200) << frame << left << QPoint(-2560, 0);
    }

    void centredOnWindow()
    {
        QFETCH(QRect, window);
        QFETCH(QSize, size);
        QFETCH(QMargins, frame);
        QFETCH(QRect, available);
        QFETCH(QPoint, expected);
        QCOMPARE(platform::centredOnWindow(window, size, frame, available), expected);
    }
};

QTEST_GUILESS_MAIN(TestDialogScreens)
#include "tst_dialog_screens.moc"
