#include "HumanFormat.h"

#include <QLocale>

namespace fang::HumanFormat {

QString size(qint64 bytes)
{
    if (bytes < 0)
        return QString();
    return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeSIFormat);
}

QString speed(qint64 bytesPerSec)
{
    if (bytesPerSec < 0)
        return QString();
    return size(bytesPerSec) + QStringLiteral("/s");
}

QString duration(qint64 seconds)
{
    if (seconds < 0)
        return QString();
    if (seconds < 60)
        return QStringLiteral("%1 s").arg(seconds);
    if (seconds < 3600)
        return QStringLiteral("%1 min %2 s").arg(seconds / 60).arg(seconds % 60);
    return QStringLiteral("%1 h %2 min").arg(seconds / 3600).arg((seconds % 3600) / 60);
}

QString dateTime(const QDateTime& dt)
{
    if (!dt.isValid())
        return QString();
    return dt.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

} // namespace fang::HumanFormat
