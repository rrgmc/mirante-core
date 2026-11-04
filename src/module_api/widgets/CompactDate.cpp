#include <mirante/widgets/CompactDate.h>

#include <QCoreApplication>
#include <QDate>
#include <QLocale>

namespace mirante::widgets {

namespace {

const char *const context = "mirante::widgets::CompactDate";

constexpr qint64 minute = 60;
constexpr qint64 hour = 60 * minute;
constexpr qint64 day = 24 * hour;
// Dates up to this many days away read as a distance.
constexpr qint64 relativeDays = 6;

// A unit of distance and its texts, for one and for several, in the past and
// to come. Both languages have only these two forms, and the module widgets'
// tests run without translations, where a "%n minute(s)" text would show as
// it is written.
struct Unit
{
    qint64 seconds;
    const char *agoOne;
    const char *agoSeveral;
    const char *inOne;
    const char *inSeveral;
};

// The largest unit first.
const Unit units[] = {
    {day, QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "1 day ago"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "%1 days ago"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "in 1 day"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "in %1 days")},
    {hour, QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "1 hour ago"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "%1 hours ago"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "in 1 hour"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "in %1 hours")},
    {minute, QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "1 minute ago"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "%1 minutes ago"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "in 1 minute"),
     QT_TRANSLATE_NOOP("mirante::widgets::CompactDate", "in %1 minutes")},
};

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
    for (const Unit &unit : units) {
        if (distance < unit.seconds)
            continue;
        const qint64 count = distance / unit.seconds;
        if (count == 1)
            return QCoreApplication::translate(context, past ? unit.agoOne : unit.inOne);
        return QCoreApplication::translate(context, past ? unit.agoSeveral : unit.inSeveral).arg(count);
    }
    return QCoreApplication::translate("mirante::widgets::CompactDate", "just now");
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

} // namespace mirante::widgets
