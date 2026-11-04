// Tests compactDate: recent dates as a distance, this year, another year,
// local time and an invalid date.
#include <mirante/widgets/CompactDate.h>

#include <QTest>
#include <QTimeZone>

using namespace mirante::widgets;

class TestCompactDate : public QObject
{
    Q_OBJECT

private slots:
    void thisYearShowsMonthDayAndTime()
    {
        // A date at least 7 days back in this year; early January has none.
        const QDate today = QDate::currentDate();
        const QDate date(today.year(), 1, 1);
        if (date.daysTo(today) < 8)
            QSKIP("No date of this year is 7 days back yet.");
        const QDateTime when(date, QTime(14, 32), QTimeZone::LocalTime);
        QCOMPARE(compactDate(when), QStringLiteral("01-01 14:32"));
    }

    void anotherYearShowsTheFullDate()
    {
        const int year = QDate::currentDate().year() - 1;
        const QDateTime when(QDate(year, 3, 30), QTime(14, 32), QTimeZone::LocalTime);
        QCOMPARE(compactDate(when), QStringLiteral("%1-03-30").arg(year));
    }

    void usesLocalTime()
    {
        const int year = QDate::currentDate().year() - 1;
        const QDateTime utc(QDate(year, 6, 15), QTime(12, 0), QTimeZone::UTC);
        QCOMPARE(compactDate(utc), utc.toLocalTime().toString(QStringLiteral("yyyy-MM-dd")));
    }

    void recentIsADistance()
    {
        const QDateTime now = QDateTime::currentDateTimeUtc();
        QCOMPARE(compactDate(now.addSecs(-10 * 60 - 5)), QStringLiteral("10 minutes ago"));
        QCOMPARE(compactDate(now.addDays(-3).addSecs(-60)), QStringLiteral("3 days ago"));
    }

    void relativeDate_data()
    {
        QTest::addColumn<qint64>("seconds");    // from now; negative is in the past
        QTest::addColumn<QString>("expected");
        QTest::newRow("now") << qint64(0) << QStringLiteral("just now");
        QTest::newRow("59 s ago") << qint64(-59) << QStringLiteral("just now");
        QTest::newRow("1 min ago") << qint64(-60) << QStringLiteral("1 minute ago");
        QTest::newRow("59 min ago") << qint64(-59 * 60 - 59) << QStringLiteral("59 minutes ago");
        QTest::newRow("1 h 59 min ago") << qint64(-119 * 60) << QStringLiteral("1 hour ago");
        QTest::newRow("23 h ago") << qint64(-23 * 3600) << QStringLiteral("23 hours ago");
        QTest::newRow("1 day ago") << qint64(-24 * 3600) << QStringLiteral("1 day ago");
        QTest::newRow("6 days ago") << qint64(-7 * 24 * 3600 + 1) << QStringLiteral("6 days ago");
        QTest::newRow("7 days ago") << qint64(-7 * 24 * 3600) << QString();
        QTest::newRow("in 2 h") << qint64(2 * 3600) << QStringLiteral("in 2 hours");
        QTest::newRow("in 1 day") << qint64(24 * 3600 + 5) << QStringLiteral("in 1 day");
        QTest::newRow("in 7 days") << qint64(7 * 24 * 3600) << QString();
    }

    void relativeDate()
    {
        QFETCH(qint64, seconds);
        QFETCH(QString, expected);
        const QDateTime now(QDate(2026, 11, 4), QTime(12, 0), QTimeZone::UTC);
        QCOMPARE(mirante::widgets::relativeDate(now.addSecs(seconds), now), expected);
    }

    void invalidIsEmpty()
    {
        QVERIFY(compactDate(QDateTime()).isEmpty());
        QVERIFY(mirante::widgets::relativeDate(QDateTime(), QDateTime::currentDateTimeUtc()).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestCompactDate)
#include "tst_compact_date.moc"
