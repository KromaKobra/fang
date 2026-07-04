#pragma once

#include <QDateTime>
#include <QString>

namespace fang::HumanFormat {

/// "18.4 MB" — locale-aware data size.
QString size(qint64 bytes);
/// "12.3 MB/s"
QString speed(qint64 bytesPerSec);
/// "2 min 5 s" style remaining-time text; empty for unknown (< 0).
QString duration(qint64 seconds);
/// Short date-time for list columns ("2026-07-03 14:22").
QString dateTime(const QDateTime& dt);

} // namespace fang::HumanFormat
