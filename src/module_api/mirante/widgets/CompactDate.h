#pragma once

#include <QDateTime>
#include <QString>

namespace mirante::widgets {

// A date for a table. Up to 6 days away it reads as a distance in one unit:
// "just now", "10 minutes ago", "1 hour ago", "3 days ago", or "in 2 hours"
// for a date to come. Further away it is in local time: "10-30 14:32" in this
// year, "2025-10-30" otherwise. An invalid date gives an empty text. See
// decisions 0071 and 0143.
QString compactDate(const QDateTime &when);

// The distance part of compactDate, from now: an empty text if when is 7 days
// or more away, or either date is invalid.
QString relativeDate(const QDateTime &when, const QDateTime &now);

} // namespace mirante::widgets
