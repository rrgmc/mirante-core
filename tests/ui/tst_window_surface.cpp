// A window that shows its first web view keeps its native surface. Qt WebEngine
// draws through the graphics API, and without
// platform::composeWindowsThroughTheGraphicsApi() Windows makes the window's
// surface again, which looks as if it closed and opened again.
//
// Only a real window system shows this, so the test is skipped on the
// offscreen platform that CTest uses. Run it on Windows with
// QT_QPA_PLATFORM=windows and QT_FORCE_STDERR_LOGGING=1.
#include "platform/Platform.h"

#include <QApplication>
#include <QPlatformSurfaceEvent>
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWidget>
#include <QWindow>

namespace {

// Counts the destructions of a window's native surface.
class SurfaceWatch : public QObject
{
public:
    int destroyed = 0;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::PlatformSurface
            && static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType()
                == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            ++destroyed;
        return QObject::eventFilter(watched, event);
    }
};

} // namespace

class TestWindowSurface : public QObject
{
    Q_OBJECT

private slots:
    void firstWebViewKeepsTheSurface()
    {
        if (QGuiApplication::platformName() == QLatin1String("offscreen"))
            QSKIP("Only a real window system makes a surface again.");
        QWidget window;
        auto *layout = new QVBoxLayout(&window);
        window.resize(400, 300);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const WId before = window.winId();
        SurfaceWatch watch;
        window.windowHandle()->installEventFilter(&watch);

        auto *view = new QWebEngineView(&window);
        layout->addWidget(view);
        QSignalSpy loaded(view, &QWebEngineView::loadFinished);
        view->setHtml(QStringLiteral("<p>page</p>"));
        QVERIFY(loaded.wait(20000));
        QTest::qWait(500);

        QCOMPARE(watch.destroyed, 0);
        QCOMPARE(window.winId(), before);
    }
};

// The setting must come before the application, as in main.cpp.
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    mirante::platform::composeWindowsThroughTheGraphicsApi();
    QApplication app(argc, argv);
    TestWindowSurface test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_window_surface.moc"
