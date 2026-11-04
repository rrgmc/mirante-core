#include <mirante/widgets/CompactDate.h>

#include <QCoreApplication>
#include <QDate>
#include <QLocale>

namespace mirante::widgets {

namespace {

constexpr qint64 minute = 60;
constexpr qint64 hour = 60 * minute;
constexpr qint64 day = 24 * hour;
// Dates up to this many days away read as a distance.
constexpr qint64 relativeDays = 6;

// Marks a text for lupdate; text() translates it.
#define N(source) QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", source)

// The text for one or for several. Both languages have only these two
// forms, and the module widgets' tests run without translations, where a
// "%n minute(s)" text would show as it is written.
QString text(const char *one, const char *several, qint64 count)
{
    return count == 1 ? QCoreApplication::translate("mirante::widgets::CompactDate", one)
                      : QCoreApplication::translate("mirante::widgets::CompactDate", several).arg(count);
}

} // namespace

QString relativeDate(const QDateTime &when, const QDateTime &now)
{
    if (!when.isValid() || !now.isValid())
        return {};
    const qint64 seconds = when.secsTo(now);
    const bool past = seconds >= 0;
    const qint64 distance = past ? seconds : -seconds;
    if (distance >= (relativeDays + 1) * day)
        return {};
    // One unit only, rounded down: "1 hour ago" for 1 h 59 min.
    if (distance < minute)
        return QCoreApplication::translate("mirante::widgets::CompactDate", "just now");
    if (distance < hour) {
        return past ? text(N("1 minute ago"), N("%1 minutes ago"), distance / minute)
                    : text(N("in 1 minute"), N("in %1 minutes"), distance / minute);
    }
    if (distance < day) {
        return past ? text(N("1 hour ago"), N("%1 hours ago"), distance / hour)
                    : text(N("in 1 hour"), N("in %1 hours"), distance / hour);
    }
    return past ? text(N("1 day ago"), N("%1 days ago"), distance / day)
                : text(N("in 1 day"), N("in %1 days"), distance / day);
}

QString compactDate(const QDateTime &when)
{
    if (!when.isValid())
        return {};
    const QString relative = relativeDate(when, QDateTime::currentDateTimeUtc());
    if (!relative.isEmpty())
        return relative;
    // Numeric, so the same in every language, but formatted by the chosen
    // language's locale, as every date shown.
    const QDateTime local = when.toLocalTime();
    return QLocale().toString(local, local.date().year() == QDate::currentDate().year()
                                         ? QStringLiteral("MM-dd HH:mm")
                                         : QStringLiteral("yyyy-MM-dd"));
}

#undef N

} // namespace mirante::widgets
