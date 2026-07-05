#pragma once

#include <QString>
#include <QStringList>

namespace fang {

/// POSIX shell quoting for custom-action command expansion (docs/03 §14,
/// docs/06). Security-critical: expanded paths are passed to `sh -c` and
/// must never be interpretable by the shell.
namespace ShellQuote {

/// Single-quotes `s`; embedded quotes become the '\'' pattern.
QString quote(const QString& s);
/// Each element quoted, joined with single spaces (for %F / %U).
QString quoteAll(const QStringList& parts);

} // namespace ShellQuote

} // namespace fang
