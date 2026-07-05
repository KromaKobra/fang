#include "ShellQuote.h"

namespace fang {
namespace ShellQuote {

QString quote(const QString& s)
{
    if (s.isEmpty())
        return QStringLiteral("''");
    QString out;
    out.reserve(s.size() + 2);
    out += QLatin1Char('\'');
    for (const QChar c : s) {
        if (c == QLatin1Char('\''))
            out += QLatin1String("'\\''");
        else
            out += c;
    }
    out += QLatin1Char('\'');
    return out;
}

QString quoteAll(const QStringList& parts)
{
    QStringList quoted;
    quoted.reserve(parts.size());
    for (const QString& p : parts)
        quoted << quote(p);
    return quoted.join(QLatin1Char(' '));
}

} // namespace ShellQuote
} // namespace fang
