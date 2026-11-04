#include "platform/DialogScreens.h"

#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QScreen>
#include <QWindow>

#include <algorithm>

namespace mirante::platform {

namespace {

#if defined(Q_OS_WIN)
// Marks a dialog this filter placed, until it hides.
constexpr char placedProperty[] = "mirante_placedOnWindowScreen";

// Runs on a dialog's Show event before the dialog's own handler, where Qt
// places it, and before the native window appears, so the move is never seen.
class ScreenFilter : public QObject
{
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        auto *dialog = qobject_cast<QDialog *>(watched);
        if (!dialog)
            return QObject::eventFilter(watched, event);
        if (event->type() == QEvent::Show && !event->spontaneous()) {
            placeOnWindowScreen(dialog);
        } else if (event->type() == QEvent::Hide && dialog->property(placedProperty).toBool()) {
            // As Qt does after its own placement, so the next show places
            // the dialog again.
            dialog->setAttribute(Qt::WA_Moved, false);
            dialog->setProperty(placedProperty, QVariant());
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void placeOnWindowScreen(QDialog *dialog)
    {
        // A dialog that placed itself is left where it is.
        if (!dialog->isWindow() || dialog->testAttribute(Qt::WA_Moved) || !dialog->parentWidget())
            return;
        const QWidget *owner = dialog->parentWidget()->window();
        if (!owner->isVisible() || owner->isMinimized())
            return;
        // When Qt's record is right, its own placement is right too.
        QScreen *screen = QGuiApplication::screenAt(owner->frameGeometry().center());
        const QScreen *recorded = owner->screen();
        if (!screen || screen == recorded)
            return;

        qInfo("Qt has screen \"%s\" on record for the window of dialog \"%s\", which is on \"%s\". The dialog "
              "opens there.",
              qPrintable(recorded ? recorded->name() : QString()), qPrintable(dialog->windowTitle()),
              qPrintable(screen->name()));
        const QRect window = owner->frameGeometry();
        const QRect inner = owner->geometry();
        // The dialog has no frame before its first show; its window's frame
        // is the closest guess.
        const QMargins frame(inner.left() - window.left(), inner.top() - window.top(),
                             window.right() - inner.right(), window.bottom() - inner.bottom());
        if (QWindow *handle = dialog->windowHandle())
            handle->setScreen(screen);
        // move() sets WA_Moved, so QDialog::showEvent leaves the place alone.
        dialog->move(centredOnWindow(window, dialog->size(), frame, screen->availableGeometry()));
        dialog->setProperty(placedProperty, true);
    }
};
#endif

} // namespace

QPoint centredOnWindow(const QRect &window, const QSize &size, const QMargins &frame, const QRect &available)
{
    const int width = size.width() + frame.left() + frame.right();
    const int height = size.height() + frame.top() + frame.bottom();
    const QPoint centre = window.center();
    int x = centre.x() - width / 2;
    int y = centre.y() - height / 2;
    // The far edges first, so a dialog larger than the screen keeps its title
    // bar and its top-left corner on it.
    x = std::max(std::min(x, available.x() + available.width() - width), available.x());
    y = std::max(std::min(y, available.y() + available.height() - height), available.y());
    return {x, y};
}

void installDialogScreens()
{
#if defined(Q_OS_WIN)
    qApp->installEventFilter(new ScreenFilter(qApp));
#endif
}

} // namespace mirante::platform
